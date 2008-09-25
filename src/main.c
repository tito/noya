#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "noya.h"

#include "module.h"
#include "thread_input.h"
#include "thread_renderer.h"
#include "thread_audio.h"
#include "thread_manager.h"

LOG_DECLARE("MAIN");
MUTEX_DECLARE(g_thread_mutex);
volatile sig_atomic_t	g_threads		= 0;
volatile sig_atomic_t	g_want_leave	= 0;
options_t				g_options		= {0};

/* services declaration
 */
typedef struct
{
	int (*fn_start)(void);
	int (*fn_stop)(void);
	int	is_started;
} service_t;
service_t services[] = {
	{	thread_audio_start,		thread_audio_stop,		0	},
	{	thread_input_start,		thread_input_stop,		0	},
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

/* show usage
 */
void usage(void)
{
	printf("Usage: noya [OPTIONS]...\n");
	printf("  -d                         Dump scene and quit\n");
	printf("  -h                         Show usage\n");
	printf("  -s <name>                  Load a scene\n");
	printf("\n");

	exit(EXIT_SUCCESS);
}

/* read options from command line
 */
void options_read(int argc, char **argv)
{
	int opt;

	while ( (opt = getopt(argc, argv, "s:d")) != -1 )
	{
		switch ( opt )
		{
			case 'd':
				g_options.dump = 1;
				break;
			case 's':
				g_options.scene_fn = strdup(optarg);
				break;
			default:
				usage();
				break;
		}
	}

	/* don't start noya if we don't have scene
	 */
	if ( g_options.scene_fn == NULL )
	{
		usage();
		return;
	}
}

/* clean options
 */
void options_free(void)
{
	if ( g_options.scene_fn != NULL )
		free(g_options.scene_fn);
}

/* start point !
 */
int main(int argc, char **argv)
{
	service_t	*service;

	printf("%s\n\n", NOYA_BANNER);

	/* read options
	 */
	options_read(argc, argv);

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

	/* load modules
	 */
	noya_event_init();
	noya_modules_init();

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

	l_printf("Free modules...");
	noya_modules_free();

	l_printf("Free events...");
	noya_event_free();

	options_free();

	l_printf("Done.");
	return 0;
}
