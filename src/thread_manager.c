#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/queue.h>
#include <assert.h>
#include <clutter/clutter.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>

#include "noya.h"
#include "event.h"
#include "thread_manager.h"
#include "thread_input.h"
#include "thread_audio.h"
#include "scene.h"
#include "utils.h"
#include "context.h"

LOG_DECLARE("MANAGER");
pthread_t				thread_manager;
static na_atomic_t		c_running		= {0};
static short			c_state			= THREAD_STATE_START;
extern na_atomic_t		g_clutter_running;

static void *thread_manager_run(void *arg)
{
	na_ctx_t	*ctxmenu;

	THREAD_ENTER;

	while ( 1 )
	{
		switch ( c_state )
		{
			case THREAD_STATE_START:

				if ( !atomic_read(&g_clutter_running) )
				{
					usleep(100);
					continue;
				}

				l_printf(" - MANAGER start...");
				c_state = THREAD_STATE_RUNNING;

				ctxmenu = na_ctx_resolve("noya");
				if ( ctxmenu == NULL )
				{
					l_errorf("cannot switch to menu, no context menu found");
					na_quit();
					break;
				}
				na_ctx_switch(ctxmenu);

				break;

			case THREAD_STATE_RESTART:
			case THREAD_STATE_STOP:
				l_printf(" - MANAGER stop...");

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

				na_ctx_manager_switch();
				na_ctx_manager_update();

				break;
		}
	}

thread_leave:;
	THREAD_LEAVE;
	return 0;
}

int thread_manager_start(void)
{
	int ret;

	ret = pthread_create(&thread_manager, NULL, thread_manager_run, NULL);
	if ( ret )
	{
		l_errorf("unable to create MANAGER thread");
		return NA_ERR;
	}

	return NA_OK;
}

int thread_manager_stop(void)
{
	while ( atomic_read(&c_running) )
		usleep(1000);
	return NA_OK;
}
