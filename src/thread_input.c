#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <lo/lo.h>

#include "noya.h"
#include "event.h"
#include "thread_input.h"

LOG_DECLARE("INPUT");
MUTEX_DECLARE(input);
pthread_t	thread_input;
static na_atomic_t	c_running		= {0};
static short		c_state			= THREAD_STATE_START;

static tuio_object_t	t_objs[TUIO_OBJECT_MAX];				/*< object info */
static uint				t_objs_alive[TUIO_OBJECT_MAX],			/*< actual object alive */
						t_objs_alive_o[TUIO_OBJECT_MAX];		/*< old alive object (needed for event) */
static uint				t_objs_alive_count				= 0,	/*< actual counter of object alive */
						t_objs_alive_count_o			= 0;
static tuio_cursor_t	t_curs[TUIO_CURSOR_MAX];				/*< cursor info */
static uint				t_curs_alive[TUIO_CURSOR_MAX],			/*< actual cursor alive */
						t_curs_alive_o[TUIO_CURSOR_MAX];		/*< old alive cursor (needed for event) */
static uint				t_curs_alive_count				= 0,	/*< actual counter of cursor alive */
						t_curs_alive_count_o			= 0;	/*< old counter of cursor alive (needed for event) */

static void _lo_error(int num, const char *msg, const char *path)
{
    l_errorf("liblo server error %d in path %s: %s\n", num, path, msg);
}

static inline tuio_object_t *_tuio_object_get_by_idx(uint id)
{
	uint	idx;

	for ( idx = 0; idx < TUIO_OBJECT_MAX; idx++ )
	{
		if ( t_objs[idx].flags != 0 && t_objs[idx].s_id == id )
			return & t_objs[idx];
	}

	return NULL;
}

#define TUIO_CALIBRATION_X		0x00
#define TUIO_CALIBRATION_Y		0x01
static float _tuio_calibrate(uint type, float value)
{
	float min, max, delta, d;

	switch ( type )
	{
		case TUIO_CALIBRATION_X:
			min = config_lookup_float(&g_config, "noya.tuio.calibration.xmin");
			max = config_lookup_float(&g_config, "noya.tuio.calibration.xmax");
			delta = config_lookup_float(&g_config, "noya.tuio.calibration.xdelta");
			break;

		case TUIO_CALIBRATION_Y:
			min = config_lookup_float(&g_config, "noya.tuio.calibration.ymin");
			max = config_lookup_float(&g_config, "noya.tuio.calibration.ymax");
			delta = config_lookup_float(&g_config, "noya.tuio.calibration.ydelta");
			break;
	}

	d = max - min;
	if ( d <= 0 )
		d = 0.000001;
	if ( d >= 1 )
		d = 1;

	if ( value < min )
		value = min;
	if ( value > max )
		value = max;

	return ((value - min) / d) + delta;
}


static int _lo_tuio_object_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 void *data, void *user_data)
{
	uint	i;
	tuio_object_t	*o;

	if ( argc <= 0 )
		return 0;

	/* object set
	 */
	if ( strcmp("set", (char *)argv[0]) == 0 && argc == 11 )
	{
		MUTEX_LOCK(input);
		o			= & t_objs[argv[2]->i];
		o->s_id		= argv[1]->i32;
		o->f_id		= argv[2]->i32;
		o->xpos		= _tuio_calibrate(TUIO_CALIBRATION_X, argv[3]->f);
		o->ypos		= _tuio_calibrate(TUIO_CALIBRATION_Y, argv[4]->f);
		o->angle	= argv[5]->f;
		o->xspeed	= argv[6]->f;
		o->yspeed	= argv[7]->f;
		o->rspeed	= argv[8]->f;
		o->maccel	= argv[9]->f;
		o->raccel	= argv[10]->f;

		/* send new object event
		 */
		if ( (o->flags & TUIO_OBJECT_FL_INIT) == TUIO_OBJECT_FL_INIT )
			na_event_send(NA_EV_OBJECT_SET, o, sizeof(tuio_object_t));
		else
			na_event_send(NA_EV_OBJECT_NEW, o, sizeof(tuio_object_t));

		o->flags	|= TUIO_OBJECT_FL_INIT;
		MUTEX_UNLOCK(input);
	}

	/* object alive list
	 */
	else if ( strcmp("alive", (char *)argv[0]) == 0 )
	{
		MUTEX_LOCK(input);

		/* copy actual to old
		 */
		t_objs_alive_count_o = t_objs_alive_count;
		memcpy(t_objs_alive_o, t_objs_alive, sizeof(tuio_object_t) * t_objs_alive_count_o);

		/* deactivate old
		 */
		for ( i = 0; i < t_objs_alive_count_o; i++ )
		{
			o = _tuio_object_get_by_idx(t_objs_alive_o[i]);
			if ( o == NULL )
				continue;
			o->flags |= TUIO_OBJECT_FL_TRYFREE;
			o->flags ^= TUIO_OBJECT_FL_USED;
		}

		/* activate current
		 */
		t_objs_alive_count = argc - 1;
		for ( i = 0; i < t_objs_alive_count; i++ )
		{
			t_objs_alive[i] = argv[i+1]->i32;
			o = _tuio_object_get_by_idx(argv[i+1]->i32);
			if ( o == NULL )
				continue;
			o->flags |= TUIO_OBJECT_FL_USED;
		}

		/* deactivate try free
		 */
		for ( i = 0; i < t_objs_alive_count_o; i++ )
		{
			o = _tuio_object_get_by_idx(t_objs_alive_o[i]);
			if ( o == NULL )
				continue;

			/* object is still used, don't blur it.
			 */
			if ( o->flags & TUIO_CURSOR_FL_USED )
			{
				o->flags ^= TUIO_OBJECT_FL_TRYFREE;
				continue;
			}

			if ( o->flags & TUIO_OBJECT_FL_TRYFREE )
			{
				/* send end object event
				 */
				o->flags = 0;
				na_event_send(NA_EV_OBJECT_DEL, o, sizeof(tuio_object_t));
			}
		}

		MUTEX_UNLOCK(input);
	}

	/* TODO handle other object like : source, fseq...
	 */

	return 1;
}

static inline tuio_cursor_t *_tuio_cursor_get_free_slot(void)
{
	uint	idx;

	for ( idx = 0; idx < TUIO_CURSOR_MAX; idx++ )
		if ( t_curs[idx].flags == 0 )
			return & t_curs[idx];

	return NULL;
}

static inline tuio_cursor_t *_tuio_cursor_get_by_id(uint id)
{
	uint	idx;

	for ( idx = 0; idx < TUIO_CURSOR_MAX; idx++ )
	{
		if ( t_curs[idx].flags != 0 && t_curs[idx].s_id == id )
			return & t_curs[idx];
	}

	return NULL;
}

static int _lo_tuio_cursor_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 void *data, void *user_data)
{
	uint	i;
	tuio_cursor_t	*c;

	if ( argc <= 0 )
		return 0;

	/* cursor set
	 */
	if ( strcmp("set", (char *)argv[0]) == 0 && argc >= 7 )
	{
		MUTEX_LOCK(input);
		c = _tuio_cursor_get_by_id(argv[1]->i32);
		if ( c == NULL )
		{
			c = _tuio_cursor_get_free_slot();
			if ( c == NULL )
				return -1;
			c->flags	= TUIO_CURSOR_FL_INIT;
		}
		c->s_id		= argv[1]->i32;
		c->xpos		= _tuio_calibrate(TUIO_CALIBRATION_X, argv[2]->f);
		c->ypos		= _tuio_calibrate(TUIO_CALIBRATION_Y, argv[3]->f);
		c->mov++;

		/* send cursor new event
		 */
		if ( c->flags == TUIO_CURSOR_FL_INIT )
			na_event_send(NA_EV_CURSOR_NEW, (void *)c, sizeof(tuio_cursor_t));
		else
			na_event_send(NA_EV_CURSOR_SET, (void *)c, sizeof(tuio_cursor_t));
		MUTEX_UNLOCK(input);
	}

	/* object alive list
	 */
	else if ( strcmp("alive", (char *)argv[0]) == 0 )
	{
		MUTEX_LOCK(input);

		/* copy actual to old
		 */
		t_curs_alive_count_o = t_curs_alive_count;
		memcpy(t_curs_alive_o, t_curs_alive, sizeof(tuio_cursor_t) * t_curs_alive_count_o);

		/* deactivate old alive
		 */
		for ( i = 0; i < t_curs_alive_count_o; i++ )
		{
			c = _tuio_cursor_get_by_id(t_curs_alive_o[i]);
			if ( c == NULL )
				continue;
			c->flags |= TUIO_CURSOR_FL_TRYFREE;
		}

		/* activate current one
		 */
		t_curs_alive_count = argc - 1;
		for ( i = 0; i < t_curs_alive_count; i++ )
		{
			t_curs_alive[i]	= argv[i+1]->i32;
			c = _tuio_cursor_get_by_id(t_curs_alive[i]);
			if ( c == NULL )
				continue;
			c->flags = TUIO_CURSOR_FL_USED;
		}

		/* deactivate try free
		 */
		for ( i = 0; i < t_curs_alive_count_o; i++ )
		{
			c = _tuio_cursor_get_by_id(t_curs_alive_o[i]);
			if ( c == NULL )
				continue;
			if ( c->flags == TUIO_CURSOR_FL_USED )
				continue;


			/* send click cursor event ! */
			if ( c->mov < TUIO_CURSOR_THRESHOLD_CLICK )
				na_event_send(NA_EV_CURSOR_CLICK, (void *)c, sizeof(tuio_cursor_t));

			/* send blur cursor event ! */
			na_event_send(NA_EV_CURSOR_DEL, (void *)c, sizeof(tuio_cursor_t));

			/* delete cursor
			 */
			c->s_id = 0, c->flags = 0, c->mov = 0;
		}

		MUTEX_UNLOCK(input);
	}

	return 1;
}

static void *thread_input_run(void *arg)
{
	const char			*port;
    int					lo_fd;
    fd_set				rfds;
    int					retval;
	lo_server			s;
	struct timeval		ts;


	THREAD_ENTER;

	while ( 1 )
	{
		switch ( c_state )
		{
			case THREAD_STATE_START:
				l_printf(" - INPUT start...");
				c_state = THREAD_STATE_RUNNING;

				/* get osc port
				 */
				port = config_lookup_string(&g_config, "noya.tuio.port");
				if ( port == NULL )
				{
					l_errorf("invalid tuio port (%s), cannot listening !", port);
					na_quit();
					goto thread_leave;
				}

				/* instanciate service
				 */
				s = lo_server_new(port, _lo_error);

				/* register callback
				 */
				lo_server_add_method(s, TUIO_CURSOR_OSCPATH, NULL, _lo_tuio_cursor_handler, NULL);
				lo_server_add_method(s, TUIO_OBJECT_OSCPATH, NULL, _lo_tuio_object_handler, NULL);

				/* get socket
				 */
				lo_fd = lo_server_get_socket_fd(s);
				if ( lo_fd <= 0 )
				{
					l_errorf("lo_server_get_socket_fd() error...");
					c_state = THREAD_STATE_RESTART;
					usleep(100);
					continue;
				}

				break;

			case THREAD_STATE_RESTART:
			case THREAD_STATE_STOP:
				l_printf(" - INPUT stop...");

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

				FD_ZERO(&rfds);
				FD_SET(lo_fd, &rfds);

				ts.tv_sec	= 0;
				ts.tv_usec	= 500;

				retval = select(lo_fd + 1, &rfds, NULL, NULL, &ts);

				if ( retval == -1 )
				{
					l_errorf("select() error...");
					c_state = THREAD_STATE_RESTART;
					usleep(100);
					continue;
				}
				else if ( retval > 0 )
				{
					if ( FD_ISSET(lo_fd, &rfds) )
						lo_server_recv_noblock(s, 0);
				}

				break;
		}
	}

thread_leave:;
	THREAD_LEAVE;

	return 0;
}

int thread_input_start(void)
{
	int ret;

	memset(t_objs, 0, TUIO_OBJECT_MAX * sizeof(tuio_object_t));
	memset(t_curs, 0, TUIO_CURSOR_MAX * sizeof(tuio_cursor_t));

	ret = pthread_create(&thread_input, NULL, thread_input_run, NULL);
	if ( ret )
	{
		l_errorf("unable to create INPUT thread");
		return NA_ERR;
	}

	return NA_OK;
}

int thread_input_stop(void)
{
	while ( atomic_read(&c_running) )
		usleep(1000);
	return NA_OK;
}
