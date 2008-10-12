#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "../src/noya.h"
#include "../src/audio.h"
#include "../src/module.h"
#include "../src/config.h"
#include "../src/utils.h"
#include "../src/thread_manager.h"

#define MODULE_NAME "obj_samplelist"

LOG_DECLARE("MOD_OBJ_SAMPLELIST");

typedef struct
{
	/* audio attached for sample
	 */
	char			*filename;		/*< audio filename */
	na_audio_t		*audio;			/*< audio entry (for audio manager) */

} obj_entry_t;

typedef struct
{
	na_scene_t			*scene;
	manager_actor_t	*actor;

	/* entries
	 */
	obj_entry_t		*entries;
	int				entry_count;
	int				idx;

	/* rendering issues
	 */
	ClutterActor	*group_cube;
	float			last_rotation;

	float			cfg_volume;
	float			*volume;

	float			*position;

	/* widget that we accept
	 */
	na_module_t		*top;
	void			*data_top;
	char			*value_top;
	na_module_t		*bottom;
	void			*data_bottom;
	char			*value_bottom;
	na_module_t		*left;
	void			*data_left;
	char			*value_left;
	na_module_t		*right;
	void			*data_right;
	char			*value_right;
} obj_t;

obj_t def_obj = {0};

static void *object_resolve_value(obj_t *obj, char *value)
{
	assert( obj != NULL );
	assert( value != NULL );
/**
	if ( strcmp(value, "volume") == 0 )
		return obj->volume;
	if ( strcmp(value, "position") == 0 )
		return obj->position;
		**/
	return NULL;
}

void lib_init(char **name, int *type)
{
	*name = strdup(MODULE_NAME);
	*type = NA_MOD_OBJECT;
}

void lib_exit(void)
{
}

void lib_object_global_config(char *key, char *value)
{
	char		*k_pos, *k_prop;
	na_module_t	**module;
	void		**data;
	char		**w_value;

	k_pos = strtok(key, ".");
	if ( k_pos == NULL )
		return;

	k_prop = strtok(NULL, ".");
	if ( k_prop == NULL )
		return;

	module = NULL;
	if ( strcmp(k_pos, "top") == 0 )
	{
		module	= &def_obj.top;
		data	= &def_obj.data_top;
		w_value	= &def_obj.value_top;
	}
	else if ( strcmp(k_pos, "bottom") == 0 )
	{
		module	= &def_obj.bottom;
		data	= &def_obj.data_bottom;
		w_value	= &def_obj.value_bottom;
	}
	else if ( strcmp(k_pos, "left") == 0 )
	{
		module	= &def_obj.left;
		data	= &def_obj.data_left;
		w_value	= &def_obj.value_left;
	}
	else if ( strcmp(k_pos, "right") == 0 )
	{
		module	= &def_obj.right;
		data	= &def_obj.data_right;
		w_value	= &def_obj.value_right;
	}
	else
		return;

	/* first command must be a type !
	 */
	if ( strcmp(k_prop, "widget") == 0 )
	{
		*module = na_module_get(value, NA_MOD_WIDGET);

		if ( *module == NULL )
		{
			l_printf("Widget %s not found", value);
			return;
		}
		*data = (*(*module)->widget_new)(NULL);
		return;
	}
	else if ( strcmp(k_prop, "value") == 0 )
	{
		*w_value = strdup(value);
	}
	else
	{
		if ( *module == NULL )
		{
			l_printf("Error : %s invalid configuration for %s module", key, MODULE_NAME);
			return;
		}

		(*(*module)->widget_config)(*data, k_prop, value);
	}
}

obj_t *lib_object_new(na_scene_t *scene)
{
	obj_t *obj;

	obj = malloc(sizeof(obj_t));
	if ( obj == NULL )
		return NULL;

	bzero(obj, sizeof(obj_t));

	obj->scene = scene;

	obj->top = def_obj.top;
	if ( obj->top )
		obj->data_top = (*obj->top->widget_clone)(def_obj.data_top, scene);
	obj->bottom = def_obj.bottom;
	if ( obj->bottom )
		obj->data_bottom = (*obj->bottom->widget_clone)(def_obj.data_bottom, scene);
	obj->left = def_obj.left;
	if ( obj->left )
		obj->data_left = (*obj->left->widget_clone)(def_obj.data_left, scene);
	obj->right = def_obj.right;
	if ( obj->right )
		obj->data_right = (*obj->right->widget_clone)(def_obj.data_right, scene);

	return obj;
}

void lib_object_free(obj_t *obj)
{
	int i;

	assert( obj != NULL );

	for ( i = 0; i < obj->entry_count; i++ )
	{
		if ( obj->entries[i].filename != NULL )
			free(obj->entries[i].filename);
		if ( obj->entries[i].audio != NULL )
			na_audio_free(obj->entries[i].audio);
	}

	if ( obj->top )
		(*obj->top->widget_free)(obj->data_top);
	if ( obj->bottom )
		(*obj->bottom->widget_free)(obj->data_bottom);
	if ( obj->left )
		(*obj->left->widget_free)(obj->data_left);
	if ( obj->right )
		(*obj->right->widget_free)(obj->data_right);

	free(obj);
}

void lib_object_config(obj_t *obj, char *key, char *value)
{
	char	filename[512];
	char	*k_idx,
			*k_prop;
	int		idx;
	obj_entry_t *entry;

	if ( strcmp(key, "count") == 0 )
	{
		if ( obj->entry_count > 0 )
		{
			l_printf("Error : %s already specified", key);
			return;
		}

		obj->entry_count = strtol(value, NULL, 10);
		obj->entries = malloc(sizeof(obj_entry_t) * obj->entry_count);
		if ( obj->entries == NULL )
		{
			l_printf("Error : unable to allocate memory for %s", key);
			obj->entry_count = 0;
			return;
		}

		bzero(obj->entries, sizeof(obj_entry_t) * obj->entry_count);
	}
	else if ( strcmp(key, "volume") == 0 )
	{
		obj->cfg_volume = atof(value);
		l_printf("Set volume to %f (from %s)", obj->cfg_volume, value);
	}
	else
	{
		if ( obj->entries == NULL )
		{
			l_printf("Error : no entries allocated, provide a count property before.");
			return;
		}

		k_idx = strtok(key, ".");
		if ( k_idx == NULL )
		{
			l_printf("Error : unable to extract index for %s", key);
			return;
		}

		idx = strtol(k_idx, NULL, 10);
		if ( idx < 0 || idx >= obj->entry_count )
		{
			l_printf("Error : invalid index for %s", key);
			return;
		}

		k_prop = strtok(NULL, "");
		if ( k_prop == NULL )
		{
			l_printf("Error : unable to extract property for %s", key);
			return;
		}

		if ( strcmp(k_prop, "file") == 0 )
		{
			snprintf(filename, sizeof(filename), "%s/%s/%s",
				na_config_get(NA_CONFIG_DEFAULT, "noya.path.scenes"),
				obj->scene->name,
				value
			);
			entry = &obj->entries[idx];

			entry->filename = strdup(value);
			entry->audio = na_audio_load(filename);
			na_audio_set_volume(entry->audio, obj->cfg_volume);
		}
		else
			l_printf("Error: invalid configuration %s", key);
	}
}

void lib_object_prepare(obj_t *obj, manager_actor_t *actor)
{
	ClutterColor	obj_background,
					obj_border;
	ClutterActor	*ac;
	char			number[5];

	assert( obj != NULL );
	assert( actor != NULL );

	/* save actor
	 */
	obj->actor = actor;

	/* link widget for value
	 */
	if ( obj->top )
		(*obj->top->widget_set_data)(obj->data_top, object_resolve_value(obj, def_obj.value_top));
	if ( obj->bottom )
		(*obj->bottom->widget_set_data)(obj->data_bottom, object_resolve_value(obj, def_obj.value_bottom));
	if ( obj->left )
		(*obj->left->widget_set_data)(obj->data_left, object_resolve_value(obj, def_obj.value_left));
	if ( obj->right )
		(*obj->right->widget_set_data)(obj->data_right, object_resolve_value(obj, def_obj.value_right));

	/* do first thing : play audio !
	 */
	na_audio_wantplay(obj->entries[obj->idx].audio);
	na_audio_set_loop(obj->entries[obj->idx].audio, actor->scene_actor->is_loop);

	/* RENDERING !
	 */

	obj->group_cube = clutter_group_new();
	clutter_container_add_actor(CLUTTER_CONTAINER(actor->rotate_actor), obj->group_cube);

	/* create object
	 */
	memcpy(&obj_background, &actor->scene_actor->background_color, sizeof(obj_background));
	memcpy(&obj_border, &actor->scene_actor->border_color, sizeof(obj_border));
	ac = clutter_rectangle_new_with_color(&obj_background);
	clutter_rectangle_set_border_color((ClutterRectangle *)ac, &obj_border);
	clutter_rectangle_set_border_width((ClutterRectangle *)ac, actor->scene_actor->border_width);
	clutter_actor_set_width(ac, actor->scene_actor->width);
	clutter_actor_set_height(ac, actor->scene_actor->height);

	clutter_container_add_actor(CLUTTER_CONTAINER(obj->group_cube), ac);

	/* create text
	 */
	snprintf(number, sizeof(number), "%d", actor->tuio->f_id);
	ac = clutter_label_new_with_text("Lucida 12", number);
	clutter_actor_set_position(ac, actor->scene_actor->width / 2 -6, actor->scene_actor->height / 2 -10);

	clutter_container_add_actor(CLUTTER_CONTAINER(obj->group_cube), ac);


	/* prepare widget too
	 */
	if ( obj->top )
		(*obj->top->widget_prepare)(obj->data_top, actor);
	if ( obj->bottom )
		(*obj->bottom->widget_prepare)(obj->data_bottom, actor);
	if ( obj->left )
		(*obj->left->widget_prepare)(obj->data_left, actor);
	if ( obj->right )
		(*obj->right->widget_prepare)(obj->data_right, actor);

}

void lib_object_unprepare(obj_t *obj)
{
	/* never remove object from clutter
	 * parent remove all of this tree
	 */
	obj->actor		= NULL;
	obj->group_cube	= NULL;

	/* stop audio
	 */
	na_audio_stop(obj->entries[obj->idx].audio);
	na_audio_seek(obj->entries[obj->idx].audio, 0);

	if ( obj->top )
		(*obj->top->widget_unprepare)(obj->data_top);
	if ( obj->bottom )
		(*obj->bottom->widget_unprepare)(obj->data_bottom);
	if ( obj->left )
		(*obj->left->widget_unprepare)(obj->data_left);
	if ( obj->right )
		(*obj->right->widget_unprepare)(obj->data_right);
}

void lib_object_update(obj_t *obj)
{
	assert( obj != NULL );

	if ( obj->top )
		(*obj->top->widget_update)(obj->data_top);
	if ( obj->bottom )
		(*obj->bottom->widget_update)(obj->data_bottom);
	if ( obj->left )
		(*obj->left->widget_update)(obj->data_left);
	if ( obj->right )
		(*obj->right->widget_update)(obj->data_right);
}

void ctl_volume(obj_t *obj, float value)
{
	assert( obj != NULL );
	na_audio_set_volume(obj->entries[obj->idx].audio, value);
}

void ctl_sampleidx(obj_t *obj, float value)
{
	int		last_idx, sens;
	float	amplitude;

	assert( obj != NULL );

	amplitude = fabs(value - obj->last_rotation);
	sens = (value - obj->last_rotation) < 0 ? 0 : 1;

	if ( amplitude < 0.1 )
		return;
	obj->last_rotation = value;

	last_idx = obj->idx;
	if ( sens )
		obj->idx++;
	else
		obj->idx--;

	if ( obj->idx >= obj->entry_count )
		obj->idx = obj->entry_count - 1;
	if ( obj->idx < 0 )
		obj->idx = 0;

	if ( last_idx != obj->idx )
	{
		na_audio_wantstop(obj->entries[last_idx].audio);
		na_audio_wantplay(obj->entries[obj->idx].audio);
		na_audio_set_volume(obj->entries[obj->idx].audio, 0.7); //obj->cfg_volume);
		na_audio_set_loop(obj->entries[obj->idx].audio, 1);
	}
}

_fn_control lib_object_get_control(char *name)
{
	if ( strcmp(name, "volume") == 0 )
		return ctl_volume;
	if ( strcmp(name, "sampleidx") == 0 )
		return ctl_sampleidx;
	return NULL;
}
