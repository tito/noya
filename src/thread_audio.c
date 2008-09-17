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
static PaStream		*c_stream		= NULL;

extern sig_atomic_t	g_want_leave;

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
	SNDFILE			*sfp = NULL;
	SF_INFO			sinfo;
	audio_entry_t	*entry;

	assert( filename != NULL );

	entry = malloc(sizeof(audio_entry_t));
	if ( entry == NULL )
		goto noya_audio_load_clean;
	bzero(entry, sizeof(audio_entry_t));

	entry->filename = strdup(filename);
	if ( entry->filename == NULL )
		goto noya_audio_load_clean;

	entry->flags = AUDIO_ENTRY_FL_USED;

	LIST_INSERT_HEAD(&audio_entries, entry, next);

	return entry;

noya_audio_load_clean:;
	if ( entry != NULL )
	{
		if ( sfp != NULL )
			sf_close(sfp);
		if ( entry->filename != NULL )
			free(entry->filename), entry->filename = NULL;
		if ( entry->data != NULL )
			free(entry->data), entry->data = NULL;
		free(entry);
	}
	return NULL;
}

void noya_audio_play(audio_entry_t *entry)
{
	assert( entry != NULL );
	entry->flags |= AUDIO_ENTRY_FL_PLAY;
}

void noya_audio_stop(audio_entry_t *entry)
{
	assert( entry != NULL );
	entry->flags &= ~AUDIO_ENTRY_FL_PLAY;
}

void noya_audio_set_loop(audio_entry_t *entry, short isloop)
{
	assert( entry != NULL );
	if ( isloop )
		entry->flags |= AUDIO_ENTRY_FL_ISLOOP;
	else
		entry->flags &= ~AUDIO_ENTRY_FL_ISLOOP;
}

void noya_audio_set_volume(audio_entry_t *entry, float volume)
{
	assert( entry != NULL );
	entry->volume = volume;
}

short noya_audio_is_play(audio_entry_t *entry)
{
	assert( entry != NULL );
	return entry->flags & AUDIO_ENTRY_FL_PLAY;
}

/* thread functions
 */

static void noya_audio_preload(void)
{
	SNDFILE			*sfp = NULL;
	SF_INFO			sinfo;
	audio_entry_t	*entry;

	for ( entry = audio_entries.lh_first; entry != NULL; entry = entry->next.le_next )
	{
		if ( !(entry->flags & AUDIO_ENTRY_FL_USED) )
			continue;
		if ( entry->flags & AUDIO_ENTRY_FL_LOADED )
			continue;
		if ( entry->flags & AUDIO_ENTRY_FL_FAILED )
			continue;

		l_printf("Load %s", entry->filename);

		sfp = sf_open(entry->filename, SFM_READ, &sinfo);
		if ( sfp == NULL )
			goto noya_audio_preload_clean;

		l_printf("Got %s (samplerate=%d, channels=%d, frames=%d)",
			entry->filename, sinfo.samplerate,
			sinfo.channels, sinfo.frames
		);

		entry->totalframes = sf_seek(sfp, (sf_count_t) 0, SEEK_END);
		sf_seek(sfp, 0, SEEK_SET);

		entry->data = (float *)malloc(entry->totalframes * sizeof(float) * MAX_CHANNELS);
		if ( entry->data == NULL )
			goto noya_audio_preload_clean;

		entry->datalen = sf_readf_float(sfp, (float *)entry->data, entry->totalframes);

		sf_close(sfp), sfp = NULL;

		entry->flags |= AUDIO_ENTRY_FL_LOADED;

		continue;

noya_audio_preload_clean:;
		l_printf("Failed to load %s", entry->filename);
		if ( sfp != NULL )
			sf_close(sfp), sfp = NULL;
		if ( entry->data != NULL )
			free(entry->data), entry->data = NULL;
		entry->flags |= AUDIO_ENTRY_FL_FAILED;
		continue;
	}
}

static int audio_output_callback(
	const void *inputBuffer, void *outputBuffer,
	unsigned long framesPerBuffer,
	const PaStreamCallbackTimeInfo* timeInfo,
	PaStreamCallbackFlags statusFlags,
	void *userData)
{
	static audio_entry_t	*entries[MAX_SOUNDS];
	int						entries_count = -1;
	audio_entry_t			*entry;
	float					*out = (float *)outputBuffer;
	unsigned long			i, j;

	(void) timeInfo; /* Prevent unused variable warnings. */
	(void) statusFlags;
	(void) inputBuffer;
	(void) userData;


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
		{
			if ( !(entry->flags & AUDIO_ENTRY_FL_ISLOOP) )
			{
				noya_audio_stop(entry);
				continue;
			}
			entry->dataidx = 0;
		}

		entries_count++;
		entries[entries_count] = entry;

		/* prepare entry for optimize
		 */
		entry->datacur = &entry->data[entry->dataidx];
	}

	/* no entries to play ?
	 * generate empty sound
	 */
	if ( entries_count < 0 )
	{
		for ( i = 0; i < framesPerBuffer; i++ )
		{
			*out++ = 0;
			*out++ = 0;
		}
		return paContinue;
	}

	/* got entries, play !
	 */
	for ( i = 0; i < framesPerBuffer; i++ )
	{
		*out		= .0f;
		*(out+1)	= .0f;

		for ( j = 0; j < entries_count; j++ )
		{
			entry = entries[j];

			/* left
			 */
			*out		+= *(entry->datacur) * entry->volume;
			entry->datacur++;

			/* right
			 */
			*(out+1)	+= *(entry->datacur) * entry->volume;
			entry->datacur++;
		}

		out += 2;
	}

	/* advance idx 
	 */
	for ( j = 0; j < entries_count; j++ )
		entries[j]->dataidx += framesPerBuffer * 2;

	return paContinue;
}



static void *thread_audio_run(void *arg)
{
	audio_entry_t	*entry;
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
				ret = Pa_StartStream(c_stream);
				if ( ret != paNoError )
				{
					l_printf("Error while starting stream : %s", Pa_GetErrorText(ret));
					g_want_leave = 1;
					continue;
				}

				/* read some files
				 */
				noya_audio_load("audio/drums.wav", 1);
				noya_audio_load("audio/bass.wav", 2);
				noya_audio_load("audio/pad.wav", 3);

				break;

			case THREAD_STATE_RESTART:
			case THREAD_STATE_STOP:
				l_printf(" - AUDIO stop...");

				if ( c_stream != NULL )
					Pa_StopStream(c_stream);

				ret = Pa_Terminate();
				if ( ret != paNoError )
					l_printf("Error while terminate PA : %s", Pa_GetErrorText(ret));

				/* freeing all audio
				 */
				while ( !LIST_EMPTY(&audio_entries) )
				{
					entry = LIST_FIRST(&audio_entries);
					LIST_REMOVE(entry, next);
					if ( entry->filename != NULL )
						free(entry->filename);
					if ( entry->data != NULL )
						free(entry->data);
					free(entry);
				}

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

				/* preload needed audio file
				 */
				noya_audio_preload();

				usleep(1000000);

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

	LIST_INIT(&audio_entries);

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
