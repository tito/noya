#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>

#include <clutter/clutter.h>

#include "noya.h"
#include "thread_renderer.h"

MUTEX_DECLARE(m_renderer);
pthread_t	thread_renderer;

/* extern
 */
extern sig_atomic_t g_want_leave;

/* static
 */
static sig_atomic_t	c_want_leave	= 0;
static sig_atomic_t	c_running		= 0;
static short		c_state			= THREAD_STATE_START;
static ClutterColor stage_color		= { 0x33, 0x09, 0x3b, 0xff };
static ClutterActor *stage			= NULL;

/* config
 */
static int	ui_width		= 640;
static int	ui_height		= 480;

void renderer_click_handle(void)
{
	g_want_leave = 1;
	return;
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
				ui_width	= config_get_int("noya.ui.width");
				ui_height	= config_get_int("noya.ui.height");

				/* init clutter
				 */
				clutter_init(NULL, NULL);

				/* get the stage and set its size and color
				 */
				stage = clutter_stage_get_default();
				clutter_actor_set_size(stage, ui_width, ui_height);
				clutter_stage_set_color(CLUTTER_STAGE(stage), &stage_color);
				clutter_stage_set_title(CLUTTER_STAGE(stage), NOYA_TITLE);

				/* prepare signals
				 */
				g_signal_connect(stage, "button-press-event",
					G_CALLBACK(renderer_click_handle),	NULL
				);

				l_printf("Default Framerate are %d", clutter_get_default_frame_rate());
				if ( clutter_get_default_frame_rate() != NOYA_DEFAULT_FRAMERATE )
				{
					l_printf("Set Framerate to %d", NOYA_DEFAULT_FRAMERATE);
					clutter_set_default_frame_rate(NOYA_DEFAULT_FRAMERATE);
				}

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
				if ( c_want_leave )
				{
					c_state = THREAD_STATE_STOP;
					break;
				}

				/* run loop for clutter
				 */
				clutter_threads_enter();
				clutter_main();
				clutter_threads_leave();

				/* if we leave... stop app ?
				 */
				g_want_leave = 1;

				break;
		}
	}

thread_leave:;
	THREAD_LEAVE;
	return 0;
}

void _thread_renderer_enter(void)
{
	MUTEX_LOCK(m_renderer);
}

void _thread_renderer_leave(void)
{
	MUTEX_UNLOCK(m_renderer);
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
		l_printf("Cannot create RENDERER thread");
		return NOYA_ERR;
	}

	return NOYA_OK;
}

int thread_renderer_stop(void)
{
	c_want_leave = 1;
	MUTEX_LOCK(m_renderer);
	clutter_main_quit();
	MUTEX_UNLOCK(m_renderer);

	while ( c_running )
		usleep(1000);

	return NOYA_OK;
}
