#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/queue.h>
#include <assert.h>
#include <clutter/clutter.h>

#include "noya.h"
#include "event.h"
#include "thread_manager.h"
#include "thread_input.h"
#include "thread_audio.h"
#include "scene.h"
#include "dump.h"

#define TUIO_PI_TO_DEG(a)	((a - 3.1416) * 57.2957795)    //360 / 2 x PI
#define PI2					6.2832

LOG_DECLARE("MANAGER");
MUTEX_DECLARE(m_manager);
pthread_t	thread_manager;
static volatile sig_atomic_t	c_want_leave	= 0;
static volatile sig_atomic_t	c_running		= 0;
static short		c_state			= THREAD_STATE_START;
scene_t				*c_scene		= NULL;
manager_actor_list_t	manager_actors_list;
manager_cursor_list_t	manager_cursors_list;
struct timeval		st_beat;
double				t_lastbeat		= 0,
					t_beat			= 0,
					t_beatinterval	= 0,
					t_current		= 0;
long				t_bpm			= 0;

ClutterColor obj_background	= { 0xff, 0xff, 0xff, 0x99 };
ClutterColor obj_border		= { 0xff, 0xff, 0xff, 0xff };


static void manager_event_object_new(unsigned short type, void *userdata, void *data)
{
	ClutterColor	col_background, col_border;
	scene_actor_base_t	*actor;
	tuio_object_t	*o = (tuio_object_t *)data;
	manager_actor_t	*el;
	ClutterActor	*stage, *ac;
	unsigned int	wx, wy;

	assert( data != NULL );

	/* search actor from scene
	 */
	actor = noya_scene_actor_get(c_scene, o->f_id);
	if ( actor == NULL )
	{
		l_errorf("actor %d not found in scene", o->f_id);
		return;
	}

	/* check rendering module
	 */
	if ( actor->mod == NULL )
	{
		l_errorf("no object associated for actor %d", o->f_id);
		return;
	}

	/* create manager actor
	 */
	el = malloc(sizeof(struct manager_actor_s));
	if ( el == NULL )
	{
		l_errorf("no enough memory to create manager actor");
		return;
	}
	bzero(el, sizeof(struct manager_actor_s));

	el->id				= o->s_id;
	el->fid				= o->f_id;
	el->scene_actor		= actor;

	clutter_threads_enter();

	el->clutter_actor	= clutter_group_new();
	el->rotate_actor	= clutter_group_new();

	clutter_container_add_actor(CLUTTER_CONTAINER(el->clutter_actor), el->rotate_actor);

	/* use associed module for rendering
	 */
	(*actor->mod->object_prepare)(actor->data_mod, el);

	/* insert actor into list
	 */
	LIST_INSERT_HEAD(&manager_actors_list, el, next);

	/* retreive last dimensions
	 */
	stage = clutter_stage_get_default();
	clutter_actor_get_size(stage, &wx, &wy);

	/* some position
	 */
	clutter_actor_set_position(el->clutter_actor,
		o->xpos * (float)wx,
		o->ypos * (float)wy
	);
	clutter_actor_set_rotation(el->rotate_actor,
		CLUTTER_Z_AXIS,
		TUIO_PI_TO_DEG(o->angle),
		actor->width * 0.5,
		actor->height * 0.5,
		0
	);

	/* show actor
	 * FIXME not needed ?
	 */
	clutter_actor_show(el->clutter_actor);

	/* add to stage
	 */
	clutter_container_add_actor(CLUTTER_CONTAINER(stage), el->clutter_actor);

	clutter_threads_leave();
}

static void manager_event_object_del(unsigned short type, void *userdata, void *data)
{
	tuio_object_t	*o = (tuio_object_t *)data;
	audio_t	*audio;
	manager_actor_t	*it = NULL;

	for ( it = manager_actors_list.lh_first; it != NULL; it = it->next.le_next )
		if ( it->id == o->s_id )
			break;

	if ( it == NULL )
		return;

	clutter_threads_enter();

	/* unprepare object
	 */
	(*it->scene_actor->mod->object_unprepare)(it->scene_actor->data_mod);

	/* remove clutter actor
	 */
	clutter_actor_destroy(it->clutter_actor);

	clutter_threads_leave();

	/* remove from list
	 */
	LIST_REMOVE(it, next);

	/* free entry
	 */
	free(it);
}

static void manager_event_object_set(unsigned short type, void *userdata, void *data)
{
	audio_t	*audio;
	tuio_object_t	*o = (tuio_object_t *)data;
	manager_actor_t	*it;
	ClutterActor *stage;
	unsigned wx, wy;

	for ( it = manager_actors_list.lh_first; it != NULL; it = it->next.le_next )
		if ( it->id == o->s_id )
			break;

	if ( it == NULL )
		return;

	/* update control
	 */
	if ( it->scene_actor->ctl_angle )
		(*it->scene_actor->ctl_angle)(it->scene_actor->data_mod, o->angle / PI2);
	if ( it->scene_actor->ctl_x )
		(*it->scene_actor->ctl_x)(it->scene_actor->data_mod, o->xpos);
	if ( it->scene_actor->ctl_y )
		(*it->scene_actor->ctl_y)(it->scene_actor->data_mod, o->ypos);

	/* update rendering
	 */
	clutter_threads_enter();

	/* retreive last dimensions
	 */
	stage = clutter_stage_get_default();
	clutter_actor_get_size(stage, &wx, &wy);

	clutter_actor_set_position(it->clutter_actor,
		o->xpos * (float)wx,
		o->ypos * (float)wy
	);
	clutter_actor_set_rotation(it->rotate_actor,
		CLUTTER_Z_AXIS,
		TUIO_PI_TO_DEG(o->angle),
		it->scene_actor->width * 0.5,
		it->scene_actor->height * 0.5,
		0
	);

	/* update object
	 */
	(*it->scene_actor->mod->object_update)(it->scene_actor->data_mod);

	clutter_threads_leave();
}

static void manager_event_cursor_new(unsigned short type, void *userdata, void *data)
{
	tuio_cursor_t *o = (tuio_cursor_t *)data;
	manager_cursor_t *el;
	ClutterActor *stage, *ac;
	unsigned int wx, wy;

	assert( data != NULL );

	el = malloc(sizeof(struct manager_cursor_s));
	LIST_INSERT_HEAD(&manager_cursors_list, el, next);

	el->id = o->s_id;
	snprintf(el->label, sizeof(el->label), "%d", o->s_id);
	el->actor = clutter_group_new();

	clutter_threads_enter();

	stage = clutter_stage_get_default();
	clutter_actor_get_size(stage, &wx, &wy);

	/* create rectangle
	 */
	ac = clutter_rectangle_new_with_color(&obj_background);
	clutter_rectangle_set_border_color((ClutterRectangle *)ac, &obj_border);
	clutter_rectangle_set_border_width((ClutterRectangle *)ac, 2);
	clutter_actor_set_height(ac, 10);
	clutter_actor_set_width(ac, 10);

	clutter_container_add_actor(CLUTTER_CONTAINER(el->actor), ac);

	/* some position
	 */
	clutter_actor_set_position(el->actor, o->xpos * (float)wx, o->ypos * (float)wy);

	clutter_actor_show(el->actor);

	clutter_container_add_actor(CLUTTER_CONTAINER(stage), el->actor);
	clutter_threads_leave();
}

static void manager_event_cursor_del(unsigned short type, void *userdata, void *data)
{
	tuio_cursor_t	*o = (tuio_cursor_t *)data;
	manager_cursor_t	*it = NULL;

	for ( it = manager_cursors_list.lh_first; it != NULL; it = it->next.le_next )
		if ( it->id == o->s_id )
			break;

	if ( it == NULL )
		return;

	clutter_threads_enter();
	clutter_actor_destroy(it->actor);
	clutter_threads_leave();

	LIST_REMOVE(it, next);
	free(it);
}

static void manager_event_cursor_set(unsigned short type, void *userdata, void *data)
{
	tuio_cursor_t	*o = (tuio_cursor_t *)data;
	manager_cursor_t	*it;
	ClutterActor *stage;
	unsigned wx, wy;

	for ( it = manager_cursors_list.lh_first; it != NULL; it = it->next.le_next )
		if ( it->id == o->s_id )
			break;

	if ( it == NULL )
		return;

	clutter_threads_enter();

	stage = clutter_stage_get_default ();
	clutter_actor_get_size(stage, &wx, &wy);

	clutter_actor_set_position(it->actor, o->xpos * (float)wx, o->ypos * (float)wy);

	clutter_threads_leave();
}

static void *thread_manager_run(void *arg)
{
	manager_actor_t	*it = NULL;
	audio_t			*entry;

	THREAD_ENTER;

	while ( 1 )
	{
		switch ( c_state )
		{
			case THREAD_STATE_START:
				l_printf(" - MANAGER start...");
				c_state = THREAD_STATE_RUNNING;

				c_scene = noya_scene_load(g_options.scene_fn);
				if ( c_scene == NULL )
				{
					l_errorf("unable to load scene");
					g_want_leave = 1;
					c_want_leave = 1;
					break;
				}

				if ( g_options.dump )
				{
					l_printf("Dump scene !");
					noya_dump(NULL, c_scene);
					g_want_leave = 1;
					c_want_leave = 1;
					break;
				}

				LIST_INIT(&manager_actors_list);

				noya_event_observe(EV_OBJECT_NEW, manager_event_object_new, NULL);
				noya_event_observe(EV_OBJECT_SET, manager_event_object_set, NULL);
				noya_event_observe(EV_OBJECT_DEL, manager_event_object_del, NULL);

				noya_event_observe(EV_CURSOR_NEW, manager_event_cursor_new, NULL);
				noya_event_observe(EV_CURSOR_SET, manager_event_cursor_set, NULL);
				noya_event_observe(EV_CURSOR_DEL, manager_event_cursor_del, NULL);

				break;

			case THREAD_STATE_RESTART:
			case THREAD_STATE_STOP:
				l_printf(" - MANAGER stop...");

				clutter_threads_enter();
				while ( manager_actors_list.lh_first != NULL )
				{
					it = manager_actors_list.lh_first;
					LIST_REMOVE(manager_actors_list.lh_first, next);
					free(it);
				}
				clutter_threads_leave();

				if ( c_scene != NULL )
				{
					noya_scene_free(c_scene);
					c_scene = NULL;
				}

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

				/* let's do the beat !
				 */
				gettimeofday(&st_beat, NULL);
				t_beat = st_beat.tv_sec + st_beat.tv_usec * 0.000001;
				t_beatinterval = 60.0 / c_scene->bpm;

				if ( t_beat - t_lastbeat > t_beatinterval )
				{
					l_printf("BPM = %lu", t_bpm);
					t_bpm++;

					/* FIXME : how to handle lag ?
					 */
					t_lastbeat = t_beat;

					/* send bpm event
					 */
					noya_event_send(EV_BPM, &t_bpm);

					for ( entry = audio_entries.lh_first; entry != NULL; entry = entry->next.le_next )
					{
						if ( !(entry->flags & AUDIO_FL_USED) )
							continue;
						if ( !(entry->flags & AUDIO_FL_LOADED) )
							continue;
						if ( entry->flags & AUDIO_FL_FAILED )
							continue;

						if ( entry->flags & AUDIO_FL_WANTPLAY )
						{
							/* play audio
							 */
							noya_audio_play(entry);

							/* increment bpm idx
							 */
							entry->bpmduration = entry->duration / t_beatinterval;
							entry->bpmidx++;

							/* enough data for play ?
							 */
							if ( entry->bpmidx <= entry->bpmduration )
								continue;

							/* back to start of sample
							 */
							noya_audio_seek(entry, 0);

							/* if it's not a loop, stop it.
							 */
							if ( !(entry->flags & AUDIO_FL_ISLOOP) )
								noya_audio_stop(entry);
						}

						if ( entry->flags & AUDIO_FL_WANTSTOP )
						{
							noya_audio_stop(entry);
							noya_audio_seek(entry, 0);
						}
					}
				}


				usleep(10);
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
