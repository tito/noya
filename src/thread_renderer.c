#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <assert.h>

#include <clutter/clutter.h>

#include "noya.h"
#include "event.h"
#include "context.h"
#include "thread_renderer.h"

LOG_DECLARE("RENDERER");
MUTEX_DECLARE(renderer);
MUTEX_IMPORT(context);
pthread_t	thread_renderer;

/* static
 */
static na_atomic_t	c_running		= {0};
static short					c_state			= THREAD_STATE_START;
static ClutterActor				*stage			= NULL;

/* config
 */
static int						ui_width		= 640;
static int						ui_height		= 480;
na_atomic_t						g_clutter_running = {0};


#define CALIBRATE_VALUE_EX(key, delta, min, max)				\
	do {														\
		value = config_lookup_float(&g_config, key);			\
		l_printf("%s = %f", key, value);						\
		value += delta;											\
		if ( value < min )										\
			value = min;										\
		if ( value > max )										\
			value = max;										\
		l_printf("Set %s to %f", key, value);					\
	} while ( 0 );
// TODO use new library libconfig
//		na_config_set_float(NA_CONFIG_DEFAULT, key, value);		\

#define CALIBRATE_VALUE(key, delta)	CALIBRATE_VALUE_EX(key, delta, 0, 1);

static gboolean renderer_button_handle(ClutterActor *actor, ClutterButtonEvent *event, gpointer data)
{
	na_event_send(NA_EV_BUTTONPRESS, event);
	return FALSE;
}

static gboolean renderer_key_handle(ClutterActor *actor, ClutterKeyEvent *event, gpointer data)
{
	gboolean is_fullscreen = FALSE;
	float value;

	switch ( event->keyval )
	{
		case CLUTTER_u:
			CALIBRATE_VALUE("noya.tuio.calibration.xmin", 0.01);
			break;
		case CLUTTER_j:
			CALIBRATE_VALUE("noya.tuio.calibration.xmin", -0.01);
			break;
		case CLUTTER_i:
			CALIBRATE_VALUE("noya.tuio.calibration.xmax", 0.01);
			break;
		case CLUTTER_k:
			CALIBRATE_VALUE("noya.tuio.calibration.xmax", -0.01);
			break;
		case CLUTTER_o:
			CALIBRATE_VALUE("noya.tuio.calibration.ymin", 0.01);
			break;
		case CLUTTER_l:
			CALIBRATE_VALUE("noya.tuio.calibration.ymin", -0.01);
			break;
		case CLUTTER_p:
			CALIBRATE_VALUE("noya.tuio.calibration.ymax", 0.01);
			break;
		case CLUTTER_m:
			CALIBRATE_VALUE("noya.tuio.calibration.ymax", -0.01);
			break;
		case CLUTTER_t:
			CALIBRATE_VALUE_EX("noya.tuio.calibration.xdelta", 0.01, -1, 1);
			break;
		case CLUTTER_g:
			CALIBRATE_VALUE_EX("noya.tuio.calibration.xdelta", -0.01, -1, 1);
			break;
		case CLUTTER_y:
			CALIBRATE_VALUE_EX("noya.tuio.calibration.ydelta", 0.01, -1, 1);
			break;
		case CLUTTER_h:
			CALIBRATE_VALUE_EX("noya.tuio.calibration.ydelta", -0.01, -1, 1);
			break;
		case CLUTTER_f:
			g_object_get( G_OBJECT(stage), "fullscreen", &is_fullscreen, NULL);
			if ( is_fullscreen )
				clutter_stage_unfullscreen(CLUTTER_STAGE(stage));
			else
				clutter_stage_fullscreen(CLUTTER_STAGE(stage));

			g_object_get(G_OBJECT(stage), "fullscreen", &is_fullscreen, NULL);
			na_event_send(NA_EV_WINDOW_UPDATE, NULL);
			break;
		case CLUTTER_q:
			na_quit();
			clutter_main_quit();
			break;
		default:
			na_event_send(NA_EV_KEYPRESS, event);
			return TRUE;
	}

	return TRUE;
}

gboolean thread_renderer_switch(gpointer data)
{
	int ts;

	if ( atomic_read(&g_want_leave) )
		return FALSE;

	MUTEX_LOCK(context);

	na_ctx_do_switch();
	ts = na_ctx_do_update();
	if ( ts > 0 )
		clutter_threads_add_timeout(ts, thread_renderer_switch, NULL);

	MUTEX_UNLOCK(context);

	return ts > 0 ? FALSE : TRUE;
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
				ui_width	= config_lookup_int(&g_config, "noya.ui.width");
				if ( ui_width <= 0 )
				{
					l_errorf("invalid noya.ui.width configuration (%d)", ui_width);
					na_quit();
					c_state = THREAD_STATE_STOP;
					continue;
				}
				ui_height	= config_lookup_int(&g_config, "noya.ui.height");
				if ( ui_height <= 0 )
				{
					l_errorf("invalid noya.ui.height configuration (%d)", ui_height);
					na_quit();
					c_state = THREAD_STATE_STOP;
					continue;
				}

				/* init clutter
				 */
				clutter_init(NULL, NULL);

				/* get the stage and set its size and color
				 */
				stage = clutter_stage_get_default();
				clutter_actor_set_size(stage, ui_width, ui_height);
				//clutter_stage_set_color(CLUTTER_STAGE(stage), &stage_color);
				clutter_stage_set_title(CLUTTER_STAGE(stage), NA_TITLE);

				/* prepare signals
				 */
				g_signal_connect(stage, "key-press-event",
					G_CALLBACK(renderer_key_handle), NULL
				);
				g_signal_connect(stage, "button-press-event",
					G_CALLBACK(renderer_button_handle), NULL
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

				/* set menu context
				 */
				assert( na_ctx_resolve("menu") != NULL );
				na_ctx_switch(na_ctx_resolve("menu"));

				/* context switch
				 */
				clutter_threads_add_timeout(250, thread_renderer_switch, NULL);

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
#if 0
	l_printf(
		"Calibration configuration :\n" \
		"noya.tuio.calibration.x.min = %f\n"			\
		"noya.tuio.calibration.x.max = %f\n"			\
		"noya.tuio.calibration.y.min = %f\n"			\
		"noya.tuio.calibration.y.max = %f\n"			\
		"noya.tuio.calibration.x.delta = %f\n"		\
		"noya.tuio.calibration.y.delta = %f",
		na_config_get_float(NA_CONFIG_DEFAULT, "noya.tuio.calibration.x.min"),
		na_config_get_float(NA_CONFIG_DEFAULT, "noya.tuio.calibration.x.max"),
		na_config_get_float(NA_CONFIG_DEFAULT, "noya.tuio.calibration.y.min"),
		na_config_get_float(NA_CONFIG_DEFAULT, "noya.tuio.calibration.y.max"),
		na_config_get_float(NA_CONFIG_DEFAULT, "noya.tuio.calibration.x.delta"),
		na_config_get_float(NA_CONFIG_DEFAULT, "noya.tuio.calibration.y.delta")
	);
#endif

	return NA_OK;
}
