#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "noya.h"

#include "thread_input.h"
#include "thread_renderer.h"
#include "thread_audio.h"
#include "thread_manager.h"

LOG_DECLARE("MAIN");
MUTEX_DECLARE(g_thread_mutex);
sig_atomic_t		g_threads		= 0;
sig_atomic_t		g_want_leave	= 0;

/* services declaration
 */
typedef struct
{
	int (*fn_start)(void);
	int (*fn_stop)(void);
	int	is_started;
} service_t;
service_t services[] = {
	{	thread_input_start,		thread_input_stop,		0	},
	{	thread_audio_start,		thread_audio_stop,		0	},
	{	thread_renderer_start,	thread_renderer_stop,	0	},
	{	thread_manager_start,	thread_manager_stop,	0	},
	{	NULL,					NULL,					0	}
};

/* handle leave signal (control+c...)
 */
void signal_leave(int sig)
{
	/* XXX Check if we need to get a lock or not ?
	 * XXX Assure it's an atomic operation
	 */
	l_printf("Catch signal %d, leave", sig);
	if ( g_want_leave )
	{
		l_printf("Signal suck ? Ok, leave...");
		exit(-1);
	}
	g_want_leave = 1;
}


/* start point !
 */
int main(int argc, char **argv)
{
	service_t	*service;

	l_printf(NOYA_BANNER);

	/* prepare signals
	 */
	signal(SIGINT, signal_leave);

	/* read config
	 */
	if ( config_init(NOYA_CONFIG_FN) )
	{
		l_printf("Error while reading %s : %s", NOYA_CONFIG_FN, strerror(errno));
		return -1;
	}

	/* start INPUT service
	 */
	l_printf("Starting services...");
	service = services;
	while ( service->fn_start )
	{
		if ( service->fn_start() != NOYA_OK )
			goto cleaning;
		service->is_started = 1;
		service++;
	}

	/* loop until we want to leave...
	 */
	while ( g_want_leave == 0 )
		sleep(1);

cleaning:;

	l_printf("Stopping services...");
	service = services;
	while ( service->fn_start )
	{
		service->fn_stop();
		service->is_started = 0;
		service++;
	}

	while ( g_threads > 0 )
		usleep(100);

	l_printf("Done.");
	return 0;
}
