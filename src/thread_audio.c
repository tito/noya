#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/queue.h>
#include <assert.h>

#include "noya.h"
#include "thread_audio.h"

LOG_DECLARE("AUDIO");
MUTEX_DECLARE(m_audio);
pthread_t	thread_audio;
static sig_atomic_t	c_want_leave	= 0;
static sig_atomic_t	c_running		= 0;
static short		c_state			= THREAD_STATE_START;

/* list of object to play
 */
LIST_HEAD(s_actor_head, audio_entry_s) audio_entries;
typedef struct audio_entry_s {
	unsigned int	id;
	LIST_ENTRY(audio_entry_s) next;
} audio_entry_t;

static void *thread_audio_run(void *arg)
{
	audio_entry_t	*it = NULL;

	THREAD_ENTER;

	while ( 1 )
	{
		switch ( c_state )
		{
			case THREAD_STATE_START:
				l_printf(" - AUDIO start...");
				c_state = THREAD_STATE_RUNNING;

				LIST_INIT(&audio_entries);

				break;

			case THREAD_STATE_RESTART:
			case THREAD_STATE_STOP:
				l_printf(" - AUDIO stop...");

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
