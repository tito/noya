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
#include "audio.h"
#include "thread_audio.h"

#define BUFFER_LEN		2048
#define MAX_CHANNELS	2
#define MAX_SOUNDS		128

LOG_DECLARE("AUDIO");
MUTEX_DECLARE(m_audio);
pthread_t	thread_audio;
static na_atomic_t	c_want_leave	= 0;
static na_atomic_t	c_running		= 0;
static short		c_state			= THREAD_STATE_START;
static PaStream		*c_stream		= NULL;

/* thread functions
 */

static void thread_audio_preload(void)
{
	SNDFILE			*sfp = NULL;
	SF_INFO			sinfo;
	na_audio_t	*entry;

	for ( entry = na_audio_entries.lh_first; entry != NULL; entry = entry->next.le_next )
	{
		if ( !(entry->flags & NA_AUDIO_FL_USED) )
			continue;
		if ( entry->flags & NA_AUDIO_FL_LOADED )
			continue;
		if ( entry->flags & NA_AUDIO_FL_FAILED )
			continue;

		l_printf("Load %s", entry->filename);

		sfp = sf_open(entry->filename, SFM_READ, &sinfo);
		if ( sfp == NULL )
			goto na_audio_preload_clean;

		l_printf("Got %s (samplerate=%d, channels=%d, frames=%d)",
			entry->filename, sinfo.samplerate,
			sinfo.channels, sinfo.frames
		);

		if ( sinfo.channels <= 0 || sinfo.channels > 2 )
		{
			l_errorf("Only mono or stereo sample are accepted !");
			goto na_audio_preload_clean;
		}
		entry->channels = sinfo.channels;

		entry->totalframes = sf_seek(sfp, (sf_count_t) 0, SEEK_END);
		sf_seek(sfp, 0, SEEK_SET);

		entry->duration = sinfo.frames / sinfo.samplerate;

		entry->data = (float *)malloc(entry->totalframes * sizeof(float) * MAX_CHANNELS);
		if ( entry->data == NULL )
			goto na_audio_preload_clean;

		entry->datalen = sf_readf_float(sfp, (float *)entry->data, entry->totalframes);

		sf_close(sfp), sfp = NULL;

		entry->flags |= NA_AUDIO_FL_LOADED;

		continue;

na_audio_preload_clean:;
		l_printf("Failed to load %s", entry->filename);
		if ( sfp != NULL )
			sf_close(sfp), sfp = NULL;
		if ( entry->data != NULL )
			free(entry->data), entry->data = NULL;
		entry->flags |= NA_AUDIO_FL_FAILED;
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
	static na_audio_t	*entries[MAX_SOUNDS];
	int						entries_count = -1;
	na_audio_t					*entry;
	float					*out = (float *)outputBuffer,
							*out_s;
	unsigned long			i, j;

	(void) timeInfo; /* Prevent unused variable warnings. */
	(void) statusFlags;
	(void) inputBuffer;
	(void) userData;

	if ( c_want_leave )
		return paComplete;


	/* check which sound are played
	 */
	for ( entry = na_audio_entries.lh_first; entry != NULL; entry = entry->next.le_next )
	{
		if ( !(entry->flags & NA_AUDIO_FL_USED) )
			continue;
		if ( !(entry->flags & NA_AUDIO_FL_LOADED) )
			continue;
		if ( !(entry->flags & NA_AUDIO_FL_PLAY) )
			continue;

		/* if we don't have enought data for this frame, don't play sound.
		 * don't reset data here, let manager reset data with current scene BPM.
		 *
		 * FIXME: try to play all rest data
		 */
		if ( (entry->dataidx / 2) + (framesPerBuffer * 2) >= entry->totalframes)
			continue;

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
	out_s = out;

	/* first, empty out
	 */
	bzero(out, sizeof(float) * framesPerBuffer * 2);

	/* and play
	 */
	for ( j = 0; j <= entries_count; j++ )
	{
		entry = entries[j];

		/* stereo audio ?
		 */
		if ( entry->channels == 2 )
		{
			for ( i = 0; i < framesPerBuffer; i++ )
			{
				/* left
				 */
				*out		+= *(entry->datacur) * entry->volume;
				entry->datacur++;

				/* right
				 */
				*(out+1)	+= *(entry->datacur) * entry->volume;
				entry->datacur++;

				out += 2;
			}
		}

		/* mono audio ?
		 */
		else if ( entry->channels == 1 )
		{
			for ( i = 0; i < framesPerBuffer; i++ )
			{
				/* left & right
				 */
				*out		+= *(entry->datacur) * entry->volume;
				*(out+1)	+= *(entry->datacur) * entry->volume;
				entry->datacur++;

				out += 2;
			}
		}

		/* rewind for next entry
		 */
		out = out_s;
	}

	/* advance idx 
	 */
	for ( j = 0; j <= entries_count; j++ )
	{
		entry = entries[j];
		entry->dataidx += framesPerBuffer * entry->channels;
		if ( entry->totalframes > 0 )
			entry->position = (entry->dataidx * 0.5) /  entry->totalframes;
		else
			entry->position = 0;
	}

	return paContinue;
}



static void *thread_audio_run(void *arg)
{
	na_audio_t	*entry;
	uint	ret,
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
					l_errorf("unable to initialize PA : %s", Pa_GetErrorText(ret));
					g_want_leave = 1;
					c_want_leave = 1;
					continue;
				}

				/* get some configs
				 */
				cfg_samplerate = na_config_get_int(NA_CONFIG_DEFAULT, "noya.audio.samplerate");
				cfg_frames = na_config_get_int(NA_CONFIG_DEFAULT, "noya.audio.frames");

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
					l_errorf("unable to open default stream : %s", Pa_GetErrorText(ret));
					g_want_leave = 1;
					c_want_leave = 1;
					continue;
				}

				/* start stream
				 */
				ret = Pa_StartStream(c_stream);
				if ( ret != paNoError )
				{
					l_errorf("unable to start stream : %s", Pa_GetErrorText(ret));
					g_want_leave = 1;
					continue;
				}

				break;

			case THREAD_STATE_RESTART:
			case THREAD_STATE_STOP:
				l_printf(" - AUDIO stop...");

				if ( c_stream != NULL )
					Pa_StopStream(c_stream);

				ret = Pa_Terminate();
				if ( ret != paNoError )
					l_errorf("unable to terminate PA : %s", Pa_GetErrorText(ret));

				/* freeing all audio
				 */
				while ( !LIST_EMPTY(&na_audio_entries) )
				{
					entry = LIST_FIRST(&na_audio_entries);
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
				thread_audio_preload();

				usleep(200000);

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

	LIST_INIT(&na_audio_entries);

	ret = pthread_create(&thread_audio, NULL, thread_audio_run, NULL);
	if ( ret )
	{
		l_errorf("unable to create AUDIO thread");
		return NA_ERR;
	}

	return NA_OK;
}

int thread_audio_stop(void)
{
	c_want_leave = 1;
	while ( c_running )
		usleep(1000);
	return NA_OK;
}
