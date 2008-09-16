#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/queue.h>
#include <assert.h>

#include <portaudio.h>
#include <sndfile.h>

#include "noya.h"
#include "thread_audio.h"

#define BUFFER_LEN		2048
#define MAX_CHANNELS	2
#define MAX_SOUNDS		128

LOG_DECLARE("AUDIO");
MUTEX_DECLARE(m_audio);
pthread_t	thread_audio;
static sig_atomic_t	c_want_leave	= 0;
static sig_atomic_t	c_running		= 0;
static short		c_state			= THREAD_STATE_START;
static PaStream		*c_stream;

extern sig_atomic_t	g_want_leave;

/* list of object to play
 */
LIST_HEAD(s_actor_head, audio_entry_s) audio_entries;
typedef struct audio_entry_s
{
#define AUDIO_ENTRY_FL_USED		0x01
#define	AUDIO_ENTRY_FL_LOADED	0x02
#define AUDIO_ENTRY_FL_PLAY		0x04
	sig_atomic_t	flags;

	unsigned int	id;
	char			*filename;

	/* sound file
	 */
	SNDFILE			*sfp;
	SF_INFO			sinfo;

	/* data
	 */
	unsigned int	totalframes;
	float			*data;
	float			datalen;
	int				dataidx;

	LIST_ENTRY(audio_entry_s) next;
} audio_entry_t;

/* move noya_audio_* functions in another thread
 */
audio_entry_t *noya_audio_get_by_filename(char *filename)
{
	audio_entry_t	*it;

	assert( filename != NULL );

	for ( it = audio_entries.lh_first; it != NULL; it = it->next.le_next )
	{
		if ( !(it->flags & AUDIO_ENTRY_FL_USED) )
			continue;
		if ( strcmp(filename, it->filename) == 0 )
			return it;
	}

	return NULL;
}

audio_entry_t *noya_audio_get_by_id(unsigned int id)
{
	audio_entry_t	*it;

	for ( it = audio_entries.lh_first; it != NULL; it = it->next.le_next )
	{
		if ( !(it->flags & AUDIO_ENTRY_FL_USED) )
			continue;
		if ( it->id == id )
			return it;
	}

	return NULL;
}

audio_entry_t *noya_audio_load(char *filename, unsigned int id)
{
	audio_entry_t	*entry;

	assert( filename != NULL );

	entry = malloc(sizeof(audio_entry_t));
	if ( entry == NULL )
		goto noya_audio_load_clean;
	bzero(entry, sizeof(audio_entry_t));

	entry->filename = strdup(filename);
	if ( entry->filename == NULL )
		goto noya_audio_load_clean;

	entry->sfp = sf_open(entry->filename, SFM_READ, &entry->sinfo);
	if ( entry->sfp == NULL )
		goto noya_audio_load_clean;

	entry->flags = AUDIO_ENTRY_FL_USED;

	l_printf("Read %s (samplerate=%d, channels=%d, frames=%d)",
		entry->filename, entry->sinfo.samplerate,
		entry->sinfo.channels, entry->sinfo.frames
	);

	entry->totalframes = sf_seek(entry->sfp, (sf_count_t) 0, SEEK_END);
	sf_seek(entry->sfp, -1 * entry->totalframes, SEEK_CUR);

	entry->data = (float *)malloc(entry->totalframes * sizeof(float) * MAX_CHANNELS);
	if ( entry->data == NULL )
		goto noya_audio_load_clean;

	entry->datalen = sf_readf_float(entry->sfp, (float *)entry->data, entry->totalframes);
	l_printf("Loaded %s (frames=%d, bytes=%d)",
		entry->filename, entry->datalen,
		entry->datalen * MAX_CHANNELS * sizeof(float)
	);

	entry->flags |= AUDIO_ENTRY_FL_LOADED;

	return entry;

noya_audio_load_clean:;
	if ( entry != NULL )
	{
		if ( entry->filename != NULL )
			free(entry->filename), entry->filename = NULL;
		if ( entry->data != NULL )
			free(entry->data), entry->data = NULL;
		free(entry);
	}
	return NULL;
}

static int audio_output_callback(
	const void *inputBuffer, void *outputBuffer,
	unsigned long framesPerBuffer,
	const PaStreamCallbackTimeInfo* timeInfo,
	PaStreamCallbackFlags statusFlags,
	void *userData)
{
	static audio_entry_t	entries[MAX_SOUNDS];
	static int				entries_count = 0;
	audio_entry_t			*entry;
	float					*out = (float *)outputBuffer;
	unsigned long			i, j;

	(void) timeInfo; /* Prevent unused variable warnings. */
	(void) statusFlags;
	(void) inputBuffer;
	(void) userData;
	int readcount;

	/* check which sound are played
	 */
	for ( entry = audio_entries.lh_first; entry != NULL; entry = entry->next.le_next )
	{
		if ( !(entry->flags & AUDIO_ENTRY_FL_USED) )
			continue;
		if ( !(entry->flags & AUDIO_ENTRY_FL_LOADED) )
			continue;
		if ( !(entry->flags & AUDIO_ENTRY_FL_PLAY) )
			continue;

		if ( (entry->dataidx / 2) + (framesPerBuffer * 2) >= entry->totalframes)
			continue;

		entries[entries_count++] = entry;
	}

	for ( i = 0; i < framesPerBuffer; i++ )
	{
		for ( j = 0, entry = entries; j < entries_count; j++, entry++ )
		{
			*out		= entry->data[entry->dataidx + i * 2];
			*(out + 1)	= entry->data[entry->dataidx + 1 + i * 2];
		}
		out += 2;
	}

	for ( j = 0, entry = entries; j < entries_count; j++, entry++ )
		entry->dataidx += framesPerBuffer * 2;

	return paContinue;
}



static void *thread_audio_run(void *arg)
{
	audio_entry_t	*it;
	unsigned int	ret,
					cfg_samplerate,
					cfg_frames;

	THREAD_ENTER;

	while ( 1 )
	{
		switch ( c_state )
		{
			case THREAD_STATE_START:
				l_printf(" - AUDIO start...");
				c_state = THREAD_STATE_RUNNING;

				LIST_INIT(&audio_entries);

				/* initialize portaudio
				 */
				ret = Pa_Initialize();
				if ( ret != paNoError )
				{
					l_printf("Error while initialize PA : %s", Pa_GetErrorText(ret));
					g_want_leave = 1;
					continue;
				}

				/* get some configs
				 */
				cfg_samplerate = config_get_int("noya.audio.samplerate");
				cfg_frames = config_get_int("noya.audio.frames");

				/* open stream
				 */
				ret = Pa_OpenDefaultStream(
					&c_stream,		/* stream */
					0,				/* input channels */
					2,				/* output channels (stereo output) */
					paFloat32,		/* 32 bit floating point output */
					cfg_samplerate,
					cfg_frames,		/* frames per buffer, i.e. the number
									   of sample frames that PortAudio will
									   request from the callback. Many apps
									   may want to use
									   paFramesPerBufferUnspecified, which
									   tells PortAudio to pick the best,
									   possibly changing, buffer size.*/
					audio_output_callback,	/* callback */
					NULL			/* user data */
				);
				if ( ret != paNoError )
				{
					l_printf("Error while opening default stream : %s", Pa_GetErrorText(ret));
					g_want_leave = 1;
					continue;
				}

				/* start stream
				 */
				ret = Pa_StartStream( c_stream );
				if ( ret != paNoError )
				{
					l_printf("Error while starting stream : %s", Pa_GetErrorText(ret));
					g_want_leave = 1;
					continue;
				}

				break;

			case THREAD_STATE_RESTART:
			case THREAD_STATE_STOP:
				l_printf(" - AUDIO stop...");

				ret = Pa_Terminate();
				if ( ret != paNoError )
					l_printf("Error while terminate PA : %s", Pa_GetErrorText(ret));

				if ( c_state == THREAD_STATE_RESTART )
					c_state = THREAD_STATE_START;
				else
					goto thread_leave;
				break;

			case THREAD_STATE_RUNNING:
				if ( c_want_leave )
				{
					c_state = THREAD_STATE_STOP;
					break;
				}

				usleep(100000);

				break;
		}
	}

thread_leave:;
	THREAD_LEAVE;
	return 0;
}

int thread_audio_start(void)
{
	int ret;

	ret = pthread_create(&thread_audio, NULL, thread_audio_run, NULL);
	if ( ret )
	{
		l_printf("Cannot create AUDIO thread");
		return NOYA_ERR;
	}

	return NOYA_OK;
}

int thread_audio_stop(void)
{
	c_want_leave = 1;
	while ( c_running )
		usleep(1000);
	return NOYA_OK;
}
