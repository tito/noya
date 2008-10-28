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
static na_atomic_t	c_running		= {0};
static short					c_state			= THREAD_STATE_START;
static ClutterColor				stage_color		= { 0x33, 0x09, 0x3b, 0xff };
static ClutterActor				*stage			= NULL;

/* config
 */
static int						ui_width		= 640;
static int						ui_height		= 480;
na_atomic_t						g_clutter_running = {0};


#define CALIBRATE_VALUE_EX(key, delta, min, max)				\
	do {														\
		value = na_config_get_float(NA_CONFIG_DEFAULT, key);	\
		value += delta;											\
		if ( value < min )										\
			value = min;										\
		if ( value > max )										\
			value = max;										\
		l_printf("Set %s to %f", key, value);					\
		na_config_set_float(NA_CONFIG_DEFAULT, key, value);		\
	} while ( 0 );

#define CALIBRATE_VALUE(key, delta)	CALIBRATE_VALUE_EX(key, delta, 0, 1);

static gboolean renderer_key_handle(ClutterActor *actor, ClutterKeyEvent *event, gpointer data)
{
	gboolean is_fullscreen = FALSE;
	float value;

	switch ( event->keyval )
	{
		case CLUTTER_u:
			CALIBRATE_VALUE("noya.cal.x.min", 0.01);
			break;
		case CLUTTER_j:
			CALIBRATE_VALUE("noya.cal.x.min", -0.01);
			break;
		case CLUTTER_i:
			CALIBRATE_VALUE("noya.cal.x.max", 0.01);
			break;
		case CLUTTER_k:
			CALIBRATE_VALUE("noya.cal.x.max", -0.01);
			break;
		case CLUTTER_o:
			CALIBRATE_VALUE("noya.cal.y.min", 0.01);
			break;
		case CLUTTER_l:
			CALIBRATE_VALUE("noya.cal.y.min", -0.01);
			break;
		case CLUTTER_p:
			CALIBRATE_VALUE("noya.cal.y.max", 0.01);
			break;
		case CLUTTER_m:
			CALIBRATE_VALUE("noya.cal.y.max", -0.01);
			break;
		case CLUTTER_t:
			CALIBRATE_VALUE_EX("noya.cal.x.delta", 0.01, -1, 1);
			break;
		case CLUTTER_g:
			CALIBRATE_VALUE_EX("noya.cal.x.delta", -0.01, -1, 1);
			break;
		case CLUTTER_y:
			CALIBRATE_VALUE_EX("noya.cal.y.delta", 0.01, -1, 1);
			break;
		case CLUTTER_h:
			CALIBRATE_VALUE_EX("noya.cal.y.delta", -0.01, -1, 1);
			break;
		case CLUTTER_f:
			g_object_get( G_OBJECT(stage), "fullscreen", &is_fullscreen, NULL);
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
				if ( atomic_read(&g_want_leave) )
				{
					c_state = THREAD_STATE_STOP;
					break;
				}

				/* run loop for clutter
				 */
				clutter_threads_enter();
				atomic_set(&g_clutter_running, 1);
				clutter_main();
				clutter_threads_leave();

				/* if we leave... stop app ?
				 */
				na_quit();
				atomic_set(&g_clutter_running, 0);

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
	if ( atomic_read(&g_clutter_running) )
	{
		MUTEX_LOCK(renderer);
		clutter_main_quit();
		MUTEX_UNLOCK(renderer);
	}

	while ( atomic_read(&c_running) )
		usleep(1000);

	/* Show calibration result
	 */
	l_printf(
		"Calibration configuration :\n" \
		"noya.cal.x.min = %f\n"			\
		"noya.cal.x.max = %f\n"			\
		"noya.cal.y.min = %f\n"			\
		"noya.cal.y.max = %f\n"			\
		"noya.cal.x.delta = %f\n"		\
		"noya.cal.y.delta = %f",
		na_config_get_float(NA_CONFIG_DEFAULT, "noya.cal.x.min"),
		na_config_get_float(NA_CONFIG_DEFAULT, "noya.cal.x.max"),
		na_config_get_float(NA_CONFIG_DEFAULT, "noya.cal.y.min"),
		na_config_get_float(NA_CONFIG_DEFAULT, "noya.cal.y.max"),
		na_config_get_float(NA_CONFIG_DEFAULT, "noya.cal.x.delta"),
		na_config_get_float(NA_CONFIG_DEFAULT, "noya.cal.y.delta")
	);

	return NA_OK;
}
