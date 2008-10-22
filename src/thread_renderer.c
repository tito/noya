#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>

#include <clutter/clutter.h>

#include "noya.h"
#include "thread_renderer.h"

LOG_DECLARE("RENDERER");
MUTEX_DECLARE(renderer);
pthread_t	thread_renderer;

/* static
 */
static na_atomic_t	c_running		= 0;
static short					c_state			= THREAD_STATE_START;
static ClutterColor				stage_color		= { 0x33, 0x09, 0x3b, 0xff };
static ClutterActor				*stage			= NULL;

/* config
 */
static int						ui_width		= 640;
static int						ui_height		= 480;
static na_atomic_t				clutter_running = 0;

static gboolean renderer_key_handle(
	ClutterActor	*actor,
	ClutterKeyEvent	*event,
	gpointer		data)
{
	gboolean is_fullscreen = FALSE;
	switch (event->keyval)
	{
		case CLUTTER_f:
			g_object_get (G_OBJECT (stage), "fullscreen", &is_fullscreen, NULL);
			if ( is_fullscreen )
				clutter_stage_unfullscreen(CLUTTER_STAGE(stage));
			else
				clutter_stage_fullscreen(CLUTTER_STAGE(stage));
			break;
		case CLUTTER_q:
			na_quit();
			clutter_main_quit();
			break;
		default:
			return TRUE;
	}

	return TRUE;
}

static void *thread_renderer_run(void *arg)
{
	THREAD_ENTER;

	while ( 1 )
	{
		switch ( c_state )
		{
			case THREAD_STATE_START:
				l_printf(" - RENDERER start...");
				c_state = THREAD_STATE_RUNNING;

				/* read config
				 */
				ui_width	= na_config_get_int(NA_CONFIG_DEFAULT, "noya.ui.width");
				ui_height	= na_config_get_int(NA_CONFIG_DEFAULT, "noya.ui.height");

				/* init clutter
				 */
				clutter_init(NULL, NULL);

				/* get the stage and set its size and color
				 */
				stage = clutter_stage_get_default();
				clutter_actor_set_size(stage, ui_width, ui_height);
				clutter_stage_set_color(CLUTTER_STAGE(stage), &stage_color);
				clutter_stage_set_title(CLUTTER_STAGE(stage), NA_TITLE);

				/* prepare signals
				 */
				g_signal_connect(stage, "key-press-event",
					G_CALLBACK(renderer_key_handle), NULL
				);

				/* show the stage
				 */
				clutter_actor_show (stage);

				break;

			case THREAD_STATE_RESTART:
			case THREAD_STATE_STOP:
				l_printf(" - RENDERER stop...");

				if ( c_state == THREAD_STATE_RESTART )
					c_state = THREAD_STATE_START;
				else
					goto thread_leave;
				break;

			case THREAD_STATE_RUNNING:
				if ( g_want_leave )
				{
					c_state = THREAD_STATE_STOP;
					break;
				}

				/* run loop for clutter
				 */
				clutter_threads_enter();
				clutter_running = 1;
				clutter_main();
				clutter_threads_leave();

				/* if we leave... stop app ?
				 */
				na_quit();
				clutter_running = 0;

				break;
		}
	}

thread_leave:;
	THREAD_LEAVE;
	return 0;
}

void _thread_renderer_enter(void)
{
	MUTEX_LOCK(renderer);
}

void _thread_renderer_leave(void)
{
	MUTEX_UNLOCK(renderer);
}

int thread_renderer_start(void)
{
	int ret;

	g_thread_init(NULL);

	clutter_threads_set_lock_functions(_thread_renderer_enter, _thread_renderer_leave);
	clutter_threads_init();

	ret = pthread_create(&thread_renderer, NULL, thread_renderer_run, NULL);
	if ( ret )
	{
		l_errorf("unable to create RENDERER thread");
		return NA_ERR;
	}

	return NA_OK;
}

int thread_renderer_stop(void)
{
	if ( clutter_running )
	{
		MUTEX_LOCK(renderer);
		clutter_main_quit();
		MUTEX_UNLOCK(renderer);
	}

	while ( c_running )
		usleep(1000);

	return NA_OK;
}
