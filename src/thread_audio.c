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
#include "db.h"
#include "utils.h"

LOG_DECLARE("AUDIO");
MUTEX_IMPORT(audiosfx);
MUTEX_DECLARE(audiovolume);

pthread_t	thread_audio;
static na_atomic_t	c_running		= {0};
static short		c_state			= THREAD_STATE_START;
static PaStream		*c_stream		= NULL;
float				g_audio_volume_R = 0;
float				g_audio_volume_L = 0;

/* thread functions
 */

static void thread_audio_preload(void)
{
	SNDFILE			*sfp = NULL;
	SF_INFO			sinfo;
	na_audio_t		*entry;
	char			*db_filename;

	for ( entry = na_audio_entries.lh_first; entry != NULL; entry = entry->next.le_next )
	{
		if ( !(atomic_read(&entry->flags) & NA_AUDIO_FL_USED) )
			continue;
		if ( atomic_read(&entry->flags) & NA_AUDIO_FL_LOADED )
			continue;
		if ( atomic_read(&entry->flags) & NA_AUDIO_FL_FAILED )
			continue;

		l_printf("Load %s", entry->filename);

		/* try to find him in db
		 */
		db_filename = na_db_get_filename_from_title(entry->filename);
		if ( db_filename == NULL )
			goto na_audio_preload_clean;
		sfp = sf_open(db_filename, SFM_READ, &sinfo);
		free(db_filename);

		if ( sfp == NULL )
			goto na_audio_preload_clean;

		l_printf("Got %s (samplerate=%d, channels=%d, frames=%d)",
			entry->filename, sinfo.samplerate,
			sinfo.channels, (int)sinfo.frames
		);

		if ( sinfo.channels <= 0 || sinfo.channels > 2 )
		{
			l_errorf("Only mono or stereo sample are accepted !");
			goto na_audio_preload_clean;
		}
		entry->channels = sinfo.channels;

		entry->totalframes = sf_seek(sfp, (sf_count_t) 0, SEEK_END);
		sf_seek(sfp, 0, SEEK_SET);

		entry->duration = (float)sinfo.frames / (float)sinfo.samplerate;

		entry->data = (float *)malloc(entry->totalframes * sizeof(float) * sinfo.channels);
		if ( entry->data == NULL )
			goto na_audio_preload_clean;

		entry->datalen = sf_readf_float(sfp, (float *)entry->data, entry->totalframes);

		sf_close(sfp), sfp = NULL;

		atomic_read(&entry->flags) |= NA_AUDIO_FL_LOADED;

		continue;

na_audio_preload_clean:;
		l_errorf("Failed to load %s", entry->filename);
		if ( sfp != NULL )
			sf_close(sfp), sfp = NULL;
		if ( entry->data != NULL )
			free(entry->data), entry->data = NULL;
		atomic_read(&entry->flags) |= NA_AUDIO_FL_FAILED;
		continue;
	}
}

static int audio_output_callback(
	const void *inputBuffer,
	void *outputBuffer,
	unsigned long framesPerBuffer,
	const PaStreamCallbackTimeInfo* timeInfo,
	PaStreamCallbackFlags statusFlags,
	void *userData)
{
	static na_audio_t	*entries[NA_DEF_SOUNDS_MAX];
	static int			entries_count;
	static na_audio_t	*entry;
	static float		*out;
	static float		*in_L,
						*in_R;
	static unsigned long i, j;
	static float		out_L,
						out_R;

	(void) timeInfo; /* Prevent unused variable warnings. */
	(void) statusFlags;
	(void) inputBuffer;
	(void) userData;

	if ( atomic_read(&g_want_leave) )
		return paAbort;

	/* initialize values
	 */
	entries_count	= -1;
	out				= (float *)outputBuffer;
	out_L			= 0;
	out_R			= 0;

	/* check which sound are played
	 */
	for ( entry = na_audio_entries.lh_first; entry != NULL; entry = entry->next.le_next )
	{
		if ( !(atomic_read(&entry->flags) & NA_AUDIO_FL_USED) )
			continue;
		if ( !(atomic_read(&entry->flags) & NA_AUDIO_FL_LOADED) )
			continue;
		if ( !(atomic_read(&entry->flags) & NA_AUDIO_FL_PLAY) )
			continue;

		/* if we don't have enought data for this frame, don't play sound.
		 * don't reset data here, let manager reset data with current scene BPM.
		 *
		 * FIXME: try to play all rest data
		 */
		if ( (entry->dataidx / entry->channels) + (framesPerBuffer * entry->channels) >= entry->totalframes)
			continue;

		entries_count++;
		entries[entries_count] = entry;

		/* prepare entry for optimize
		 */
		entry->datacur = &entry->data[entry->dataidx];

		/* copy entry to his chunk
		 */
		assert( na_chunk_get_channels(entry->input) == 2 );
		if ( entry->channels == 1 )
		{
			in_L = na_chunk_get_channel(entry->input, 0);
			in_R = na_chunk_get_channel(entry->input, 1);

			for ( i = 0; i < framesPerBuffer; i++ )
			{
				*in_L	= *(entry->datacur) * entry->volume;
				in_L++;
				*in_R	= *(entry->datacur) * entry->volume;
				in_R++;
				entry->datacur++;
			}
		}
		else if ( entry->channels == 2 )
		{
			in_L = na_chunk_get_channel(entry->input, 0);
			in_R = na_chunk_get_channel(entry->input, 1);

			for ( i = 0; i < framesPerBuffer; i++ )
			{
				*in_L	= *(entry->datacur) * entry->volume;
				entry->datacur++;
				in_L++;
				*in_R	= *(entry->datacur) * entry->volume;
				entry->datacur++;
				in_R++;
			}
		}

		/* next idx
		 */
		entry->dataidx += framesPerBuffer * entry->channels;
		if ( entry->totalframes > 0 )
		{
			/* change this if we support more than stereo
			 */
			assert( entry->channels <= 2 );
			entry->position = (float)(entry->dataidx >> (entry->channels - 1)) /  (float)entry->totalframes;
		}
		else
			entry->position = 0;
	}

	/* first, empty out
	 */
	bzero(out, sizeof(float) * framesPerBuffer * NA_OUTPUT_CHANNELS);

	/* no entries to play ?
	 */
	if ( entries_count < 0 )
		return paContinue;

	/* ========================================================= *
	 * CRITICAL SECTION
	 * We need to ensure that processing from input to output
	 * is not modified in the time of process.
	 * Path: input -> sfx1 -> sfx2 -> ... -> {out_L,out_R}
	 * ========================================================= */

	MUTEX_LOCK(audiosfx);

	/* process sfx, as fast as we can.
	 */
	for ( j = 0; j <= entries_count; j++ )
		na_audio_sfx_process(entries[j]);

	/* and play
	 */
	for ( j = 0; j <= entries_count; j++ )
	{
		entry = entries[j];
		in_L = entry->out_L;
		in_R = entry->out_R;

		for ( i = 0; i < framesPerBuffer; i++ )
		{
			/* left
			 */
			*out		+= *in_L;
			in_L++;
			out++;

			/* right
			 */
			*out		+= *in_R;
			in_R++;
			out++;
		}

		/* rewind for next entry
		 */
		out = (float *)outputBuffer;
	}

	MUTEX_UNLOCK(audiosfx);

	/* ========================================================= *
	 * END OF CRITICAL SECTION
	 * ========================================================= */

	/* save volume output
	 */
	for ( i = 0; i < framesPerBuffer; i++ )
	{
		if ( *out > out_L )
			out_L = *out;
		out++;
		if ( *out > out_R )
			out_R = *out;
		out++;
	}

	MUTEX_LOCK(audiovolume);
	g_audio_volume_L = out_L;
	g_audio_volume_R = out_R;
	MUTEX_UNLOCK(audiovolume);

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
		if ( atomic_read(&g_want_leave) )
		{
			c_state = THREAD_STATE_STOP;
			break;
		}

		switch ( c_state )
		{
			case THREAD_STATE_START:
				l_printf(" - AUDIO start...");

				/* initialize portaudio
				 */
				ret = Pa_Initialize();
				if ( ret != paNoError )
				{
					l_errorf("unable to initialize PA : %s", Pa_GetErrorText(ret));
					sleep(5);
					continue;
				}

				/* get some configs
				 */
				cfg_samplerate	= config_lookup_int(&g_config, "noya.audio.samplerate");
				cfg_frames		= config_lookup_int(&g_config, "noya.audio.frames");

				/* open stream
				 */
				ret = Pa_OpenDefaultStream(
					&c_stream,		/* stream */
					0,				/* input channels */
					NA_OUTPUT_CHANNELS,	/* output channels */
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
					sleep(5);
					continue;
				}

				/* start stream
				 */
				ret = Pa_StartStream(c_stream);
				if ( ret != paNoError )
				{
					l_errorf("unable to start stream : %s", Pa_GetErrorText(ret));
					sleep(5);
					continue;
				}

				c_state = THREAD_STATE_RUNNING;
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
					na_audio_free(entry);
				}

				if ( c_state == THREAD_STATE_RESTART )
					c_state = THREAD_STATE_START;
				else
					goto thread_leave;
				break;

			case THREAD_STATE_RUNNING:
				if ( atomic_read(&g_want_leave) )
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
	while ( atomic_read(&c_running) )
		usleep(1000);
	return NA_OK;
}
