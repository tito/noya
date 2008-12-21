#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <locale.h>

#include "noya.h"

#include "module.h"
#include "db.h"
#include "thread_input.h"
#include "thread_renderer.h"
#include "thread_audio.h"
#include "context.h"

LOG_DECLARE("MAIN");
MUTEX_DECLARE(global);
na_atomic_t				g_threads		= {0};
na_atomic_t				g_want_leave	= {0};
na_options_t			g_options		= {0};
config_t				g_config		= {0};

/* contexts import
 */
void	context_menu_register(void);
void	context_noya_register(void);
void	context_marmelade_register(void);

/* services declaration
 */
typedef struct
{
	int (*fn_start)(void);
	int (*fn_stop)(void);
	int	is_started;
} na_service_t;
na_service_t services[] = {
	{	thread_audio_start,		thread_audio_stop,		0	},
	{	thread_input_start,		thread_input_stop,		0	},
	{	thread_renderer_start,	thread_renderer_stop,	0	},
	{	NULL,					NULL,					0	}
};

/* handle leave signal (control+c...)
 */
void signal_leave(int sig)
{
	l_printf("Catch signal %d, leave", sig);
	if ( atomic_read(&g_want_leave) )
	{
		l_printf("Signal suck ? Ok, leave...");
		exit(-1);
	}
	na_quit();
}

/* show usage
 */
void usage(void)
{
	printf("Usage: noya [OPTIONS]...\n");
	printf("  -h                         Show usage\n");
	printf("  -c <context>               Switch to <context> at start\n");
	printf("  -s <name>                  Load a scene\n");
	printf("  -i <directory>             Import waves from directory\n");
	printf("  -V                         Show version and quit\n");
	printf("\n");

	exit(EXIT_SUCCESS);
}

/* leave application
 */
void na_quit(void)
{
	static sig_atomic_t showprint = 1;
	if ( showprint )
	{
		l_printf("Stopping services...");
		showprint = 0;
	}

	na_event_stop();
	atomic_set(&g_want_leave, 1);
}

/* read options from command line
 */
void na_options_read(int argc, char **argv)
{
	int opt;

	while ( (opt = getopt(argc, argv, "c:s:di:V")) != -1 )
	{
		switch ( opt )
		{
			case 'c':
				g_options.default_context = strdup(optarg);
				break;
			case 's':
				g_options.scene_fn = strdup(optarg);
				break;
			case 'i':
				g_options.import_dir = strdup(optarg);
				break;
			case 'V':
				exit(-1);
				break;
			default:
				usage();
				break;
		}
	}

	/* import directory ?
	 */
	if ( g_options.import_dir != NULL )
	{
		int stat_ok		= 0,
			stat_exist	= 0,
			stat_err	= 0;
		l_printf("import directory %s", g_options.import_dir);
		na_db_init();
		na_db_import_directory(g_options.import_dir, &stat_ok, &stat_exist, &stat_err);
		na_db_free();
		l_printf("DONE: %d new sounds imported, %d exists, %d errors",
			stat_ok, stat_exist, stat_err
		);
		exit(0);
	}

}

/* clean options
 */
void na_options_free(void)
{
	if ( g_options.scene_fn != NULL )
		free(g_options.scene_fn);
}

/* start point !
 */
int main(int argc, char **argv)
{
	na_service_t	*service;

	printf("%s\n%s\n\n", NA_BANNER, NA_WEBSITE);

	/* read options
	 */
	na_options_read(argc, argv);

	/* prepare signals
	 */
	signal(SIGINT, signal_leave);

	/* read config
	 */
	config_init(&g_config);
	if ( !config_read_file(&g_config, NA_CONFIG_FN) )
	{
		l_errorf("unable to read config file %s", NA_CONFIG_FN);
		l_errorf("line %d: %s",
				config_error_line(&g_config),
				config_error_text(&g_config)
			);
		return -1;
	}

	/* load modules
	 */
	na_db_init();
	na_event_init();
	na_modules_init();

	/* register contexts
	 */
	l_printf("Register contexts...");
	context_menu_register();
	context_noya_register();
	context_marmelade_register();

	/* start INPUT service
	 */
	l_printf("Starting services...");
	service = services;
	while ( service->fn_start )
	{
		if ( service->fn_start() != NA_OK )
			goto cleaning;
		service->is_started = 1;
		service++;
	}

	/* loop until we want to leave...
	 */
	while ( atomic_read(&g_want_leave) == 0 )
		sleep(1);

	/* stop events
	 */
	na_event_stop();

cleaning:;

	service = services;
	while ( service->fn_start )
	{
		service->fn_stop();
		service->is_started = 0;
		service++;
	}

	while ( atomic_read(&g_threads) > 0 )
		usleep(100);

	l_printf("Free modules...");
	na_modules_free();

	l_printf("Free events...");
	na_event_free();

	l_printf("Free db...");
	na_db_free();

	config_destroy(&g_config);
	na_options_free();

	l_printf("Done.");
	return 0;
}
