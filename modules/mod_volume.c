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

#define MODULE_NAME "mod_volume"

LOG_DECLARE("MOD_SCENE_VOLUME");
MUTEX_IMPORT(audiovolume);

extern na_atomic_t g_audio_volume_L;
extern na_atomic_t g_audio_volume_R;

typedef struct
{
	na_scene_t			*scene;

	/* rendering issues
	 */
	ClutterActor		*group;

	ClutterActor		**volbox;
	uint				volcount;
	int					volidx;

	/* configuration
	 */
	uint				boxsx,
						boxsy,
						boxdx,
						boxdy,
						boxmx,
						boxmy,
						boxborderwidth;

	ClutterColor		vol_background,
						vol_backgroundhi,
						vol_backgroundhili,
						vol_border;
} obj_t;

obj_t def_obj = {0};

ClutterColor vol_backgroundhili	= { 0xff, 0xff, 0xff, 0x99 };
ClutterColor vol_backgroundhi	= { 0xff, 0x66, 0x66, 0xaa };
ClutterColor vol_background		= { 0x66, 0x66, 0x66, 0xaa };
ClutterColor vol_border			= { 0xff, 0xff, 0xff, 0xaa };

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
	obj->volcount	= 20;
	obj->volbox		= malloc(obj->volcount * sizeof(ClutterActor *) * 2);
	if ( obj->volbox == NULL )
	{
		free(obj);
		return NULL;
	}
	bzero(obj->volbox, obj->volcount * sizeof(ClutterActor *));

	/* default values
	 */
	obj->boxsx					= 10;
	obj->boxsy					= 10;
	obj->boxmx					= 2;
	obj->boxmy					= 5;
	obj->boxdx					= 140;
	obj->boxdy					= 10;
	obj->boxborderwidth			= 0;
	obj->volidx					= -1;
	obj->vol_background			= vol_background;
	obj->vol_backgroundhi		= vol_backgroundhi;
	obj->vol_backgroundhili		= vol_backgroundhili;
	obj->vol_border				= vol_border;

	return obj;
}

void lib_object_free(obj_t *obj)
{
	assert( obj != NULL );

	free(obj->volbox);
	free(obj);
}

void lib_object_config(obj_t *obj, char *key, config_setting_t *value)
{
	/* no config yet.
	 */
	return;
}

static gboolean lib_object_renderer_update(gpointer data)
{
	float	out_L = CLUTTER_FIXED_TO_FLOAT(atomic_read(&g_audio_volume_L)) * 20,
			out_R = CLUTTER_FIXED_TO_FLOAT(atomic_read(&g_audio_volume_R)) * 20;
	int		x, y, i = 0;
	obj_t	*obj = (obj_t *)data;

	assert( obj != NULL );

	i = 0;
	for ( y = 0; y < 2; y++ )
	{
		for ( x = 0; x < obj->volcount; x++ )
		{
			if ( (y == 0 && out_L > x) || (y == 1 && out_L > x) )
				clutter_rectangle_set_color(CLUTTER_RECTANGLE(obj->volbox[i]), &obj->vol_backgroundhili);
			else
				clutter_rectangle_set_color(CLUTTER_RECTANGLE(obj->volbox[i]), &obj->vol_background);

			i++;
		}
	}
}

/* NOTE: actor is not used with MOD_MODULE
 */
void lib_object_prepare(obj_t *obj, manager_actor_t *actor)
{
	ClutterActor *stage;
	uint		x, y, i;

	assert( obj != NULL );

	/* RENDERING !
	 */
	stage = clutter_stage_get_default();
	obj->group = clutter_group_new();
	clutter_container_add_actor(CLUTTER_CONTAINER(stage), obj->group);

	/* create box
	 */
	i = 0;
	for ( y = 0; y < 2; y++ )
	{
		for ( x = 0; x < obj->volcount; x++ )
		{
			obj->volbox[i] = clutter_rectangle_new();
			clutter_actor_set_width(obj->volbox[i], obj->boxsx);
			clutter_actor_set_height(obj->volbox[i], obj->boxsy);
			clutter_actor_set_x(obj->volbox[i], obj->boxdx + (x * (obj->boxsx + obj->boxmx)));
			clutter_actor_set_y(obj->volbox[i], obj->boxdy + (y * (obj->boxsy + obj->boxmy)));
			clutter_rectangle_set_color(CLUTTER_RECTANGLE(obj->volbox[i]), &obj->vol_background);
			clutter_rectangle_set_border_color(CLUTTER_RECTANGLE(obj->volbox[i]), &obj->vol_border);
			clutter_rectangle_set_border_width(CLUTTER_RECTANGLE(obj->volbox[i]), obj->boxborderwidth);
			clutter_container_add_actor(CLUTTER_CONTAINER(obj->group), obj->volbox[i]);
			i++;
		}
	}

	clutter_threads_add_timeout(25, lib_object_renderer_update, obj);
}

void lib_object_unprepare(obj_t *obj)
{
	/* never remove object from clutter
	 * parent remove all of this tree
	 */
	obj->group		= NULL;
}
