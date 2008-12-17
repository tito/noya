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

#include "actors/clutter-round-rectangle.h"

#define MODULE_NAME "mod_volume"

LOG_DECLARE("MOD_SCENE_VOLUME");
MUTEX_IMPORT(audiovolume);

static char *mod_settings[] =
{
	"borderwidth",
	"count",
	"sx",
	"sy",
	"mx",
	"my",
	"dx",
	"dy",
	"px",
	"py",
	NULL
};

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
						boxpx,
						boxpy,
						boxborderwidth;

	ClutterColor		vol_background,
						vol_backgroundlo,
						vol_backgroundhi,
						vol_borderhi,
						vol_border;
} obj_t;

obj_t def_obj = {0};

ClutterColor vol_background			= { 0x43, 0x85, 0x8d, 0xff };
ClutterColor vol_backgroundlo		= { 0x6f, 0xed, 0xf1, 0xff };
ClutterColor vol_backgroundhi		= { 0xff, 0xff, 0xff, 0xff };
ClutterColor vol_borderhi			= { 0x71, 0xf3, 0xf7, 0xff };
ClutterColor vol_border				= { 0x2e, 0x5d, 0x63, 0xff };

void lib_init(char **name, int *type, char ***settings)
{
	*name		= strdup(MODULE_NAME);
	*type		= NA_MOD_MODULE;
	*settings	= mod_settings;
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
	obj->volcount	= 40;
	obj->volbox		= malloc(obj->volcount * sizeof(ClutterActor *) * 2);
	if ( obj->volbox == NULL )
	{
		free(obj);
		return NULL;
	}
	bzero(obj->volbox, obj->volcount * sizeof(ClutterActor *));

	/* default values
	 */
	obj->boxsx					= 4;
	obj->boxsy					= 8;
	obj->boxmx					= 1;
	obj->boxmy					= 1;
	obj->boxdx					= 140;
	obj->boxdy					= 10;
	obj->boxpx					= 4;
	obj->boxpy					= 4;
	obj->boxborderwidth			= 0;
	obj->volidx					= -1;
	obj->vol_background			= vol_background;
	obj->vol_backgroundlo		= vol_backgroundlo;
	obj->vol_backgroundhi		= vol_backgroundhi;
	obj->vol_borderhi			= vol_borderhi;
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
	int i;
	if ( strcmp(key, "borderwidth") == 0 )
		obj->boxborderwidth = config_setting_get_int(value);
	else if ( strcmp(key, "count") == 0 )
	{
		i = config_setting_get_int(value);
		if ( i > 0 && obj->volcount != i )
		{
			obj->volcount	= i;
			if ( obj->volbox )
				free(obj->volbox);
			obj->volbox		= malloc(obj->volcount * sizeof(ClutterActor *) * 2);
			if ( obj->volbox == NULL )
				return;
			bzero(obj->volbox, obj->volcount * sizeof(ClutterActor *));
		}
	}
	else if ( strcmp(key, "sx") == 0 )
		obj->boxsx = config_setting_get_int(value);
	else if ( strcmp(key, "sy") == 0 )
		obj->boxsy = config_setting_get_int(value);
	else if ( strcmp(key, "mx") == 0 )
		obj->boxmx = config_setting_get_int(value);
	else if ( strcmp(key, "my") == 0 )
		obj->boxmy = config_setting_get_int(value);
	else if ( strcmp(key, "dx") == 0 )
		obj->boxdx = config_setting_get_int(value);
	else if ( strcmp(key, "dy") == 0 )
		obj->boxdy = config_setting_get_int(value);
	else if ( strcmp(key, "px") == 0 )
		obj->boxpx = config_setting_get_int(value);
	else if ( strcmp(key, "py") == 0 )
		obj->boxpy = config_setting_get_int(value);
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
			if ( (y == 0 && out_L > x) || (y == 1 && out_R > x) )
				clutter_rectangle_set_color(CLUTTER_RECTANGLE(obj->volbox[i]), &obj->vol_backgroundhi);
			else
				clutter_rectangle_set_color(CLUTTER_RECTANGLE(obj->volbox[i]), &obj->vol_backgroundlo);

			i++;
		}
	}

	return TRUE;
}

/* NOTE: actor is not used with MOD_MODULE
 */
void lib_object_prepare(obj_t *obj, manager_actor_t *actor)
{
	ClutterActor *stage, *rec;
	uint		x, y, i;

	assert( obj != NULL );

	/* RENDERING !
	 */
	stage = clutter_stage_get_default();
	obj->group = clutter_group_new();
	clutter_container_add_actor(CLUTTER_CONTAINER(stage), obj->group);

	rec = clutter_round_rectangle_new();
	clutter_actor_set_x(rec, obj->boxdx);
	clutter_actor_set_y(rec, obj->boxdy);
	clutter_actor_set_width(rec, obj->volcount * obj->boxsx + (obj->volcount - 1) * obj->boxmx + 2 * obj->boxpx);
	clutter_actor_set_height(rec, 2 * (obj->boxsy) + 2 * obj->boxpy + obj->boxmy);
	clutter_round_rectangle_set_color(CLUTTER_ROUND_RECTANGLE(rec), &obj->vol_background);
	clutter_round_rectangle_set_border_color(CLUTTER_ROUND_RECTANGLE(rec), &obj->vol_borderhi);
	clutter_round_rectangle_set_border_width(CLUTTER_ROUND_RECTANGLE(rec), obj->boxborderwidth);
	clutter_container_add_actor(CLUTTER_CONTAINER(obj->group), rec);


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
			clutter_actor_set_x(obj->volbox[i], obj->boxdx + obj->boxpx + (x * (obj->boxsx + obj->boxmx)));
			clutter_actor_set_y(obj->volbox[i], obj->boxdy + obj->boxpy + (y * (obj->boxsy + obj->boxmy)));
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
