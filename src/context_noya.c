#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/queue.h>
#include <assert.h>
#include <clutter/clutter.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>

#include <string.h>
#include "noya.h"
#include "event.h"
#include "thread_manager.h"
#include "thread_input.h"
#include "thread_audio.h"
#include "scene.h"
#include "utils.h"
#include "context.h"
#include "rtc.h"

LOG_DECLARE("NOYA");
MUTEX_IMPORT(context);

static na_ctx_t		s_context;

static na_atomic_t		c_running		= {0};
static na_atomic_t		c_scene_changed	= {0};
static ClutterColor		stage_color		= { 0x00, 0x00, 0x00, 0xff };
static ClutterColor		obj_background	= { 0xff, 0xff, 0xff, 0x99 };
static ClutterColor		obj_border		= { 0xff, 0xff, 0xff, 0xff };
na_scene_t				*c_scene		= NULL;
manager_actor_list_t	manager_actors_list;
manager_cursor_list_t	manager_cursors_list;


manager_actor_list_t *na_manager_get_actors(void)
{
	return &manager_actors_list;
}

manager_actor_t *na_manager_actor_get_by_fid(uint fid)
{
	manager_actor_t	*it;
	for ( it = manager_actors_list.lh_first; it != NULL; it = it->next.le_next )
		if ( it->tuio->f_id == fid )
			return it;
	return NULL;
}

static void manager_event_object_new(unsigned short type, void *userdata, void *data)
{
	na_scene_actor_t		*actor;
	tuio_object_t			*o = (tuio_object_t *)data;
	manager_actor_t			*el;
	ClutterActor			*stage;
	uint					wx,
							wy;

	assert( data != NULL );

	/* search actor from scene
	 */
	actor = na_scene_actor_get(c_scene, o->f_id);
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
	el->tuio			= o;
	el->scene_actor		= actor;

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

	atomic_set(&c_scene_changed, 1);
}

static void manager_event_object_del(unsigned short type, void *userdata, void *data)
{
	tuio_object_t	*o = (tuio_object_t *)data;
	manager_actor_t	*it = NULL;

	for ( it = manager_actors_list.lh_first; it != NULL; it = it->next.le_next )
		if ( it->id == o->s_id )
			break;

	if ( it == NULL )
		return;

	/* unprepare object
	 */
	(*it->scene_actor->mod->object_unprepare)(it->scene_actor->data_mod);

	/* remove clutter actor
	 */
	clutter_actor_destroy(it->clutter_actor);

	/* remove from list
	 */
	LIST_REMOVE(it, next);

	/* free entry
	 */
	free(it);

	atomic_set(&c_scene_changed, 1);
}

static void manager_event_object_set(unsigned short type, void *userdata, void *data)
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

	/* update control
	 */
	if ( it->scene_actor->ctl_angle )
		(*it->scene_actor->ctl_angle)(it->scene_actor->data_mod, o->angle / PI2);
	if ( it->scene_actor->ctl_x )
		(*it->scene_actor->ctl_x)(it->scene_actor->data_mod, o->xpos);
	if ( it->scene_actor->ctl_y )
		(*it->scene_actor->ctl_y)(it->scene_actor->data_mod, o->ypos);

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

	atomic_set(&c_scene_changed, 1);
}

static void manager_event_cursor_new(unsigned short type, void *userdata, void *data)
{
	tuio_cursor_t *o = (tuio_cursor_t *)data;
	manager_cursor_t *el;
	ClutterActor *stage, *ac;
	uint wx, wy;

	assert( data != NULL );

	el = malloc(sizeof(struct manager_cursor_s));
	LIST_INSERT_HEAD(&manager_cursors_list, el, next);

	el->id = o->s_id;
	snprintf(el->label, sizeof(el->label), "%d", o->s_id);

	el->actor = clutter_group_new();

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

	clutter_actor_destroy(it->actor);

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

	stage = clutter_stage_get_default ();
	clutter_actor_get_size(stage, &wx, &wy);

	clutter_actor_set_position(it->actor, o->xpos * (float)wx, o->ypos * (float)wy);
}

void thread_manager_mod_init(na_module_t *module, void *userdata)
{
	char				**k_setting;
	config_setting_t	*k;
	char				tmpbuffer[256];

	assert( module != NULL );
	if ( module->object_new )
		module->data = (*module->object_new)(userdata);
	if ( module->data == NULL )
	{
		l_errorf("failed to initialize static module %s", module->name);
		return;
	}

	/* search and inject configuration
	 * noya.modules.<modulename>.<key> = <value>
	 */
	k_setting = module->settings;
	if ( k_setting )
	{
		while ( *k_setting != NULL )
		{
			snprintf(tmpbuffer, sizeof(tmpbuffer), "noya.modules.%s.%s",
				module->name, *k_setting);
			k = config_lookup(&g_config, tmpbuffer);
			if ( k != NULL )
				(*module->object_config)(module->data, *k_setting, k);
			k_setting++;
		}
	}

	if ( module->object_prepare )
		(*module->object_prepare)(module->data, NULL);
}

void thread_manager_mod_deinit(na_module_t *module, void *userdata)
{
	assert( module != NULL );
	if ( module->data == NULL )
		return;

	if ( module->object_unprepare )
		(*module->object_unprepare)(module->data);
	if ( module->object_free )
		(*module->object_free)(module->data);
	module->data = NULL;
}

static gboolean manager_renderer_update(gpointer data)
{
	manager_actor_t	*it;

	/* scene changed, send event
	*/
	if ( atomic_read(&c_scene_changed) )
	{
		na_event_send(NA_EV_SCENE_UPDATE, NULL, 0);
		atomic_set(&c_scene_changed, 0);
	}


	if ( !atomic_read(&c_running ) )
		return FALSE;

	for ( it = manager_actors_list.lh_first; it != NULL; it = it->next.le_next )
	{
		(*it->scene_actor->mod->object_update)(it->scene_actor->data_mod);
		clutter_actor_queue_redraw(clutter_stage_get_default());
	}

	return TRUE;
}

int context_noya_activate(void *ctx, void *userdata)
{
	ClutterActor	*stage;

	atomic_set(&c_running, 1);

	stage = clutter_stage_get_default();
	clutter_stage_set_color(CLUTTER_STAGE(stage), &stage_color);

	c_scene = na_scene_load(g_options.scene_fn);
	if ( c_scene == NULL )
	{
		l_errorf("unable to load scene");
		na_quit();
		return -1;
	}

	/* initialize actors
	 */
	LIST_INIT(&manager_actors_list);

	/* initialize modules
	 */
	na_module_yield(NA_MOD_MODULE, thread_manager_mod_init, c_scene);

	/* listening events
	 */
	na_event_observe(NA_EV_OBJECT_NEW, manager_event_object_new, NULL);
	na_event_observe(NA_EV_OBJECT_SET, manager_event_object_set, NULL);
	na_event_observe(NA_EV_OBJECT_DEL, manager_event_object_del, NULL);

	na_event_observe(NA_EV_CURSOR_NEW, manager_event_cursor_new, NULL);
	na_event_observe(NA_EV_CURSOR_SET, manager_event_cursor_set, NULL);
	na_event_observe(NA_EV_CURSOR_DEL, manager_event_cursor_del, NULL);

	clutter_threads_add_timeout(25, manager_renderer_update, NULL);

	return 0;
}

int context_noya_deactivate(void *ctx, void *userdata)
{
	manager_actor_t	*it = NULL;

	atomic_set(&c_running, 0);

	/* remove events
	 */
	na_event_remove(NA_EV_OBJECT_NEW, manager_event_object_new, NULL);
	na_event_remove(NA_EV_OBJECT_SET, manager_event_object_set, NULL);
	na_event_remove(NA_EV_OBJECT_DEL, manager_event_object_del, NULL);
	na_event_remove(NA_EV_CURSOR_NEW, manager_event_cursor_new, NULL);
	na_event_remove(NA_EV_CURSOR_SET, manager_event_cursor_set, NULL);
	na_event_remove(NA_EV_CURSOR_DEL, manager_event_cursor_del, NULL);

	/* deinitialize modules
	 */
	na_module_yield(NA_MOD_MODULE, thread_manager_mod_deinit, NULL);

	while ( manager_actors_list.lh_first != NULL )
	{
		it = manager_actors_list.lh_first;
		LIST_REMOVE(manager_actors_list.lh_first, next);
		free(it);
	}

	if ( c_scene != NULL )
	{
		na_scene_free(c_scene);
		c_scene = NULL;
	}

	return 0;
}

int context_noya_update(void *ctx, void *userdata)
{
	return 0;
}

void context_noya_register()
{
	bzero(&s_context, sizeof(s_context));

	strncpy(s_context.name, "noya", sizeof(s_context.name));
	s_context.fn_activate	= context_noya_activate;
	s_context.fn_deactivate	= context_noya_deactivate;
	s_context.fn_update		= context_noya_update;

	MUTEX_LOCK(context);
	na_ctx_register(&s_context);
	MUTEX_UNLOCK(context);
}
