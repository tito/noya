#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/queue.h>
#include <assert.h>
#include <clutter/clutter.h>

#include "noya.h"
#include "thread_manager.h"
#include "thread_input.h"

#define TUIO_PI_TO_DEG(a)	((a - 3.1416) * 57.2957795)    //360 / 2 x PI

MUTEX_DECLARE(m_manager);
pthread_t	thread_manager;
static sig_atomic_t	c_want_leave	= 0;
static sig_atomic_t	c_running		= 0;
static short		c_state			= THREAD_STATE_START;

ClutterColor obj_background	= { 0xff, 0xff, 0xff, 0x99 };
ClutterColor obj_border		= { 0xff, 0xff, 0xff, 0xff };

/* list of object in scene
 */
LIST_HEAD(s_actor_head, manager_actor_s) manager_actors_list;
typedef struct manager_actor_s {
	unsigned int	id;
	ClutterActor	*actor;
	char			label[10];
	LIST_ENTRY(manager_actor_s) next;
} manager_actor_t;

static void manager_event_object_new(unsigned short type, void *data)
{
	tuio_object_t *o = (tuio_object_t *)data;
	manager_actor_t *el;
	ClutterActor *stage, *ac;
	unsigned int wx, wy;

	assert( data != NULL );

	stage = clutter_stage_get_default ();
	clutter_actor_get_size(stage, &wx, &wy);

	el = malloc(sizeof(struct manager_actor_s));
	LIST_INSERT_HEAD(&manager_actors_list, el, next);

	el->id = o->s_id;
	snprintf(el->label, sizeof(el->label), "%d", o->f_id);
	el->actor = clutter_group_new();

	/* create rectangle
	 */
	ac = clutter_rectangle_new_with_color(&obj_background);
	clutter_rectangle_set_border_color((ClutterRectangle *)ac, &obj_border);
	clutter_rectangle_set_border_width((ClutterRectangle *)ac, 4);
	clutter_actor_set_height(ac, 60);
	clutter_actor_set_width(ac, 60);

	clutter_container_add_actor(CLUTTER_CONTAINER(el->actor), ac);

	/* create text
	 */
	ac = clutter_label_new_with_text ("Mono 12", el->label);
	clutter_actor_set_position(ac, 30-6, 30-6);

	clutter_container_add_actor(CLUTTER_CONTAINER(el->actor), ac);

	/* some position
	 */
	clutter_actor_set_position(el->actor, o->xpos * (float)wx, o->ypos * (float)wy);
	clutter_actor_set_rotation(el->actor, CLUTTER_Z_AXIS, TUIO_PI_TO_DEG(o->angle), 30, 30, 0);

	clutter_actor_show(el->actor);

	clutter_threads_enter();
	clutter_container_add_actor(CLUTTER_CONTAINER(stage), el->actor);
	clutter_threads_leave();
}

static void manager_event_object_del(unsigned short type, void *data)
{
	tuio_object_t	*o = (tuio_object_t *)data;
	manager_actor_t	*it = NULL;

	for ( it = manager_actors_list.lh_first; it != NULL; it = it->next.le_next )
		if ( it->id == o->s_id )
			break;

	if ( it == NULL )
		return;

	clutter_actor_destroy(it->actor);
	LIST_REMOVE(it, next);
	free(it);
}

static void manager_event_object_set(unsigned short type, void *data)
{
	tuio_object_t	*o = (tuio_object_t *)data;
	manager_actor_t	*it;
	ClutterActor *stage;
	unsigned wx, wy;

	for ( it = manager_actors_list.lh_first; it != NULL; it = it->next.le_next )
		if ( it->id == o->s_id )
			break;

	if ( it == NULL )
		return;

	stage = clutter_stage_get_default ();
	clutter_actor_get_size(stage, &wx, &wy);

	clutter_threads_enter();
	clutter_actor_set_position(it->actor, o->xpos * (float)wx, o->ypos * (float)wy);
	clutter_actor_set_rotation(it->actor, CLUTTER_Z_AXIS, TUIO_PI_TO_DEG(o->angle), 30, 30, 0);
	clutter_threads_leave();
}

static void *thread_manager_run(void *arg)
{
	manager_actor_t	*it = NULL;

	THREAD_ENTER;

	while ( 1 )
	{
		switch ( c_state )
		{
			case THREAD_STATE_START:
				l_printf(" - MANAGER start...");
				c_state = THREAD_STATE_RUNNING;

				LIST_INIT(&manager_actors_list);

				event_observe(EV_OBJECT_NEW, manager_event_object_new);
				event_observe(EV_OBJECT_SET, manager_event_object_set);
				event_observe(EV_OBJECT_DEL, manager_event_object_del);

				break;

			case THREAD_STATE_RESTART:
			case THREAD_STATE_STOP:
				l_printf(" - MANAGER stop...");

				clutter_threads_enter();
				while ( manager_actors_list.lh_first != NULL )
				{
					it = manager_actors_list.lh_first;
					//clutter_actor_destroy(it->actor);
					LIST_REMOVE(manager_actors_list.lh_first, next);
					free(it);
				}
				clutter_threads_leave();

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

				/* TODO write running
				 */
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
		l_printf("Cannot create MANAGER thread");
		return NOYA_ERR;
	}

	return NOYA_OK;
}

int thread_manager_stop(void)
{
	c_want_leave = 1;
	while ( c_running )
		usleep(1000);
	return NOYA_OK;
}
