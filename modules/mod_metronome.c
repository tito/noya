#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "noya.h"
#include "audio.h"
#include "module.h"
#include "config.h"
#include "utils.h"
#include "thread_manager.h"
#include "event.h"

#define MODULE_NAME "mod_metronome"

LOG_DECLARE("MOD_SCENE_METRONOME");

typedef struct
{
	na_scene_t			*scene;

	/* rendering issues
	 */
	ClutterActor		*group;

	ClutterActor		**beatbox;
	uint				beatcount;
	int					beatidx;

	/* configuration
	 */
	uint				boxsize,
						boxdx,
						boxdy,
						boxmx,
						boxborderwidth;

	ClutterColor		beat_background,
						beat_backgroundhi,
						beat_backgroundhili,
						beat_border;
} obj_t;

obj_t def_obj = {0};

ClutterColor beat_background	= { 0xff, 0xff, 0xff, 0x99 };
ClutterColor beat_backgroundhi	= { 0xff, 0x66, 0x66, 0xaa };
ClutterColor beat_backgroundhili= { 0x66, 0x66, 0x66, 0xaa };
ClutterColor beat_border		= { 0xff, 0xff, 0xff, 0xaa };

void lib_init(char **name, int *type, char ***settings)
{
	*name = strdup(MODULE_NAME);
	*type = NA_MOD_MODULE;
	*settings = NULL;
}

void lib_exit(void)
{
}

obj_t *lib_object_new(na_scene_t *scene)
{
	obj_t *obj;

	obj = malloc(sizeof(obj_t));
	if ( obj == NULL )
		return NULL;
	bzero(obj, sizeof(obj_t));

	obj->scene		= scene;
	obj->beatcount	= scene->measure;
	obj->beatbox	= malloc(obj->beatcount * sizeof(ClutterActor *));
	if ( obj->beatbox == NULL )
	{
		free(obj);
		return NULL;
	}
	bzero(obj->beatbox, obj->beatcount * sizeof(ClutterActor *));

	/* default values
	 */
	obj->boxsize				= 10;
	obj->boxmx					= 5;
	obj->boxdx					= 10;
	obj->boxdy					= 10;
	obj->boxborderwidth			= 0;
	obj->beatidx				= -1;
	obj->beat_background		= beat_background;
	obj->beat_backgroundhi		= beat_backgroundhi;
	obj->beat_backgroundhili	= beat_backgroundhili;
	obj->beat_border			= beat_border;

	return obj;
}

void lib_object_free(obj_t *obj)
{
	assert( obj != NULL );

	free(obj->beatbox);
	free(obj);
}

void lib_object_config(obj_t *obj, char *key, config_setting_t *value)
{
	/* no config yet.
	 */
	return;
}

static void metronome_bpm(ushort ev_type, void *userdata, void *object)
{
	obj_t		*obj = (obj_t *)userdata;
	na_bpm_t	*bpm = (na_bpm_t *)object;

	assert( obj != NULL );

	clutter_threads_enter();

	if ( obj->beatidx >= 0 )
		clutter_rectangle_set_color(CLUTTER_RECTANGLE(obj->beatbox[obj->beatidx]), &obj->beat_background);
	obj->beatidx = bpm->beatinmeasure - 1;
	if ( obj->beatidx == 0 )
		clutter_rectangle_set_color(CLUTTER_RECTANGLE(obj->beatbox[obj->beatidx]), &obj->beat_backgroundhi);
	else
		clutter_rectangle_set_color(CLUTTER_RECTANGLE(obj->beatbox[obj->beatidx]), &obj->beat_backgroundhili);

	clutter_threads_leave();
}

/* NOTE: actor is not used with MOD_MODULE
 */
void lib_object_prepare(obj_t *obj, manager_actor_t *actor)
{
	ClutterActor *stage;
	uint		i;

	assert( obj != NULL );

	/* RENDERING !
	 */
	stage = clutter_stage_get_default();
	obj->group = clutter_group_new();
	clutter_container_add_actor(CLUTTER_CONTAINER(stage), obj->group);

	/* create box
	 */
	for ( i = 0; i < obj->beatcount; i++ )
	{
		obj->beatbox[i] = clutter_rectangle_new();
		clutter_actor_set_width(obj->beatbox[i], obj->boxsize);
		clutter_actor_set_height(obj->beatbox[i], obj->boxsize);
		clutter_actor_set_x(obj->beatbox[i], obj->boxdx + (i *  (obj->boxsize + obj->boxmx)));
		clutter_actor_set_y(obj->beatbox[i], obj->boxdy);
		clutter_rectangle_set_color(CLUTTER_RECTANGLE(obj->beatbox[i]), &obj->beat_background);
		clutter_rectangle_set_border_color(CLUTTER_RECTANGLE(obj->beatbox[i]), &obj->beat_border);
		clutter_rectangle_set_border_width(CLUTTER_RECTANGLE(obj->beatbox[i]), obj->boxborderwidth);
		clutter_container_add_actor(CLUTTER_CONTAINER(obj->group), obj->beatbox[i]);
	}

	na_event_observe(NA_EV_BPM, metronome_bpm, obj);
}

void lib_object_unprepare(obj_t *obj)
{
	na_event_remove(NA_EV_BPM, metronome_bpm, obj);

	/* never remove object from clutter
	 * parent remove all of this tree
	 */
	obj->group		= NULL;
}
