#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <dlfcn.h>
#include <math.h>
#include <locale.h>

#include "noya.h"
#include "audio.h"
#include "module.h"
#include "config.h"
#include "utils.h"
#include "event.h"
#include "thread_manager.h"

#include <ladspa.h>

#define MODULE_NAME				"obj_ladspa"
#define DEFAULT_LADSPA_PATH		"/usr/lib/ladspa/"

LOG_DECLARE("MOD_OBJ_LADSPA");

struct obj_s;

typedef struct obj_entry_s
{
	struct obj_s			*parent;	/* used from event */

	uint					id;			/* actor id */
	manager_actor_t			*actor;
	na_audio_t				*audio;

	na_audio_sfx_t			*sfx;		/* buffer for sfx */
	LADSPA_Handle			*pl_ladspa_handle; /* effect instance for sfx */

	LIST_ENTRY(obj_entry_s) next;
} obj_entry_t;

typedef struct
{
	obj_entry_t *lh_first;
} obj_entry_list_t;

typedef struct obj_s
{
	na_scene_t			*scene;
	manager_actor_t		*actor;

	/* ladspa issue
	 */
	char				*pl_name;
	char				*pl_filename;
	void				*pl_handle;
	LADSPA_Descriptor_Function pl_ladspa_fn;
	LADSPA_Descriptor	*pl_ladspa_desc;

	int					pl_input_count;
	int					pl_output_count;

	/* list of connected object
	 */
	obj_entry_list_t	entries;

	/* rendering issues
	 */
	ClutterActor		*group_cube;

	/* min/max distance value to attach
	 */
	float			dist_min;
	float			dist_max;

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

static obj_t def_obj = {0};

static inline float __dist(float xa, float ya, float xb, float yb)
{
#define pow2(a) ((a)*(a))
	return sqrtf(pow2(xb-xa) + pow2(yb-ya));
}

static obj_entry_t *__find_entry_by_id(obj_entry_list_t *list, uint id)
{
	obj_entry_t *it;

	assert( list != NULL );

	LIST_FOREACH(it, list, next)
	{
		if ( it->id == id )
			return it;
	}
	return NULL;
}

static obj_entry_t *__entry_new(obj_entry_list_t *list)
{
	obj_entry_t *entry;

	assert( list != NULL );

	entry = malloc(sizeof(obj_entry_t));
	if ( entry == NULL )
		return NULL;
	bzero(entry, sizeof(obj_entry_t));

	LIST_INSERT_HEAD(list, entry, next);

	return entry;
}

static void __entry_free(obj_entry_list_t *list, obj_entry_t *entry)
{
	assert( entry != NULL );
	LIST_REMOVE(entry, next);
	free(entry);
}

static void __event_dispatch(ushort ev_type, void *userdata, void *object)
{
	obj_entry_t *dobj = (obj_entry_t *)userdata;

	assert( userdata != NULL );
	assert( dobj != NULL );
	assert( dobj->sfx != NULL );

	switch ( ev_type )
	{
		case NA_EV_AUDIO_PLAY:
			l_printf("Event AUDIO_PLAY : connect sfx");
			na_audio_sfx_connect((na_audio_t *)object, dobj->sfx);
			dobj->audio = object;
			break;
		case NA_EV_AUDIO_STOP:
			l_printf("Event AUDIO_PLAY : disconnect sfx");
			na_audio_sfx_disconnect((na_audio_t *)object, dobj->sfx);
			dobj->audio = object;
			break;
		case NA_EV_ACTOR_PREPARE:
			dobj->actor = (manager_actor_t *)object;
			/* TODO create a visual between actors
			 */
			break;
		case NA_EV_ACTOR_UNPREPARE:
			dobj->actor = NULL;
			/* TODO remove visual between actors 
			 */
			break;
	}
}

static void __audio_sfx_connect(void *userdata, na_chunk_t *in, na_chunk_t *out)
{
	uint					i,
							in_audio = 0,
							out_audio = 0;
	obj_entry_t				*entry = (obj_entry_t *)userdata;
	LADSPA_Data				dummy_port;
	LADSPA_PortDescriptor	port;

	/* We only support :
	 * mono/stereo input chunk
	 * mono/stereo input plugin
	 * -> 1-1, 1-2, 2-1, 2-2
	 */

	if ( na_chunk_get_channels(in) > NA_OUTPUT_CHANNELS )
		return;

	/* create an instance of ladspa plugin for this object
	 */
	entry->pl_ladspa_handle = entry->parent->pl_ladspa_desc->instantiate(
		entry->parent->pl_ladspa_desc, NA_DEF_SAMPLERATE
	);
	if ( entry->pl_ladspa_handle == NULL )
	{
		l_errorf("Unable to have a plugin instance !");
		return;
	}

	/* we cannot handle differents channels between IO from chunk and plugin
	 */
	if ( na_chunk_get_channels(in)  != entry->parent->pl_input_count ||
		 na_chunk_get_channels(out) != entry->parent->pl_output_count )
	{
		l_errorf("I/O channels not corresponding : Chunk [%d,%d] vs Effect [%d,%d]",
			na_chunk_get_channels(in), na_chunk_get_channels(out),
			entry->parent->pl_input_count, entry->parent->pl_output_count
		);
		return;
	}

	for ( i = 0; i < entry->parent->pl_ladspa_desc->PortCount; i++ )
	{
		port = entry->parent->pl_ladspa_desc->PortDescriptors[i];

		if ( ! LADSPA_IS_PORT_AUDIO(port) )
		{
			l_printf("call ladspa connect_port DUMMY %d", i);
			entry->parent->pl_ladspa_desc->connect_port(
				entry->pl_ladspa_handle, i,
				&dummy_port
			);
		}
		else if ( LADSPA_IS_PORT_INPUT(port) )
		{
			l_printf("call ladspa connect_port IN %d, %p", i,
				na_chunk_get_channel(in, in_audio)
			);
			entry->parent->pl_ladspa_desc->connect_port(
				entry->pl_ladspa_handle, i,
				na_chunk_get_channel(in, in_audio)
			);
			in_audio++;
		}
		else if ( LADSPA_IS_PORT_OUTPUT(port) )
		{
			l_printf("call ladspa connect_port OUT %d, %p", i,
				na_chunk_get_channel(out, out_audio)
			);
			entry->parent->pl_ladspa_desc->connect_port(
				entry->pl_ladspa_handle, i,
				na_chunk_get_channel(out, out_audio)
			);
			out_audio++;
		}
	}

	if ( entry->parent->pl_ladspa_desc->activate );
	{
		l_printf("call ladspa activate");
		entry->parent->pl_ladspa_desc->activate(entry->pl_ladspa_handle);
	}
}

static void __audio_sfx_disconnect(void *userdata)
{
	obj_entry_t *entry = (obj_entry_t *)userdata;

	if ( entry->pl_ladspa_handle == NULL )
		return;

	if ( entry->parent->pl_ladspa_desc->deactivate )
	{
		l_printf("call ladspa deactivate");
		entry->parent->pl_ladspa_desc->deactivate(entry->pl_ladspa_handle);
	}

	if ( entry->parent->pl_ladspa_desc->cleanup )
	{
		l_printf("call ladspa cleanup");
		entry->parent->pl_ladspa_desc->cleanup(entry->pl_ladspa_handle);
	}

	entry->pl_ladspa_handle = NULL;
}

static void __audio_sfx_process(void *userdata)
{
	obj_entry_t *entry = (obj_entry_t *)userdata;

	entry->parent->pl_ladspa_desc->run(
		entry->pl_ladspa_handle,
		entry->sfx->out->size
	);
}

static void __scene_update(ushort type, void *userdata, void *data)
{
	obj_t					*obj = (obj_t *)userdata;
	manager_actor_list_t	*list;
	manager_actor_t			*it;
	obj_entry_t				*dobj;
	float					dist;

	assert( obj != NULL );

	list = na_manager_get_actors();
	if ( list == NULL )
		return;

	for ( it = list->lh_first; it != NULL; it = it->next.le_next )
	{
		if ( it == obj->actor )
			continue;

		assert( it->tuio != NULL );

		dobj = __find_entry_by_id(&obj->entries, it->id);

		dist = __dist(
			it->tuio->xpos,
			it->tuio->ypos,
			obj->actor->tuio->xpos,
			obj->actor->tuio->ypos
		);

#ifdef DEBUG_INTERNAL
		l_printf("[%f,%f] -> [%f, %f] = dist %f / min %f / max %f",
				obj->actor->tuio->xpos, obj->actor->tuio->ypos,
				it->tuio->xpos, it->tuio->ypos,
				dist, obj->dist_min, obj->dist_max
		);
#endif

		if ( dist < obj->dist_min )
		{
			if ( dobj != NULL )
				continue;

			/* test is we can connect to this object
			 */
			l_printf("Try to attach on object %d", it->id);

			if ( it->scene_actor->mod == NULL || it->scene_actor->data_mod == NULL )
			{
				l_errorf("No module attach to object %d, skip", it->id);
				continue;
			}

			if ( !(it->scene_actor->mod->type & NA_MOD_EVENT) )
			{
				l_errorf("No event interface on object %d, skip", it->id);
				continue;
			}

			/* create attached object
			 */

			dobj = __entry_new(&obj->entries);
			if ( dobj == NULL )
			{
				l_errorf("Cannot allocate new link !");
				continue;
			}

			dobj->id		= it->id;
			dobj->parent	= obj;

			/* create sfx
			 */
			dobj->sfx = na_audio_sfx_new(
				obj->pl_input_count,
				obj->pl_output_count,
				__audio_sfx_connect,
				__audio_sfx_disconnect,
				__audio_sfx_process,
				dobj
			);
			if ( dobj->sfx == NULL )
			{
				l_errorf("Unable to create a sfx !");
				__entry_free(&obj->entries, dobj);
				continue;
			}

			/* subscribe to event
			 */
			(*it->scene_actor->mod->event_observe)(it->scene_actor->data_mod,
					NA_EV_ACTOR_PREPARE, __event_dispatch, dobj);
			(*it->scene_actor->mod->event_observe)(it->scene_actor->data_mod,
					NA_EV_ACTOR_UNPREPARE, __event_dispatch, dobj);
			(*it->scene_actor->mod->event_observe)(it->scene_actor->data_mod,
					NA_EV_AUDIO_PLAY, __event_dispatch, dobj);
			(*it->scene_actor->mod->event_observe)(it->scene_actor->data_mod,
					NA_EV_AUDIO_STOP, __event_dispatch, dobj);

			l_printf("Object %d attached to %d", obj->actor->id, dobj->id);
		}
		else if ( dist > obj->dist_max )
		{
			if ( dobj == NULL )
				continue;

			/* TODO write detach object code.
			 */

			__event_dispatch(NA_EV_AUDIO_STOP, dobj, dobj->audio);
			__audio_sfx_disconnect(dobj);

			(*it->scene_actor->mod->event_remove)(it->scene_actor->data_mod,
					NA_EV_ACTOR_PREPARE, __event_dispatch, dobj);
			(*it->scene_actor->mod->event_remove)(it->scene_actor->data_mod,
					NA_EV_ACTOR_UNPREPARE, __event_dispatch, dobj);
			(*it->scene_actor->mod->event_remove)(it->scene_actor->data_mod,
					NA_EV_AUDIO_PLAY, __event_dispatch, dobj);
			(*it->scene_actor->mod->event_remove)(it->scene_actor->data_mod,
					NA_EV_AUDIO_STOP, __event_dispatch, dobj);

			l_printf("Object %d detached to %d", obj->actor->id, dobj->id);

			__entry_free(&obj->entries, dobj);

		}

	}

	return;
}

/* code taken from ladspa > applyplugin.c
 */
static unsigned long __ladspa_count_port(
		const LADSPA_Descriptor		*psDescriptor,
		const LADSPA_PortDescriptor	iType)
{
	unsigned long lCount;
	unsigned long lIndex;

	lCount = 0;
	for (lIndex = 0; lIndex < psDescriptor->PortCount; lIndex++)
		if ((psDescriptor->PortDescriptors[lIndex] & iType) == iType)
			lCount++;

	return lCount;
}

static void __load_ladspa(obj_t *obj)
{
	l_printf("Load ladspa on %p", obj);

	/* load descriptor
	 */
	obj->pl_ladspa_fn = (LADSPA_Descriptor_Function)dlsym(obj->pl_handle, "ladspa_descriptor");
	if ( obj->pl_ladspa_fn == NULL )
	{
		l_errorf("Unable to found ladspa_descriptor() in %s", obj->pl_name);
		goto load_cleanup;
	}

	obj->pl_ladspa_desc = (LADSPA_Descriptor *)obj->pl_ladspa_fn(0);
	if ( obj->pl_ladspa_desc == NULL )
	{
		l_errorf("Unable to get a ladspa descriptor in %s", obj->pl_name);
		goto load_cleanup;
	}

	if ( !LADSPA_IS_HARD_RT_CAPABLE(obj->pl_ladspa_desc->Properties) )
	{
		l_errorf("Unable to use %s, not Hard-RT capable.", obj->pl_name);
		goto load_cleanup;
	}

	/* count input / output
	 */
	obj->pl_input_count = __ladspa_count_port(obj->pl_ladspa_desc, LADSPA_PORT_AUDIO | LADSPA_PORT_INPUT);
	obj->pl_output_count = __ladspa_count_port(obj->pl_ladspa_desc, LADSPA_PORT_AUDIO | LADSPA_PORT_OUTPUT);

	l_printf("Ladspa plugin have %d IN, %d OUT", obj->pl_input_count, obj->pl_output_count);

	return;

load_cleanup:;
	if ( obj->pl_handle )
		dlclose(obj->pl_handle);
	obj->pl_handle = NULL;
	return;
}

static void *object_resolve_value(obj_t *obj, char *value)
{
	assert( obj != NULL );
	assert( value != NULL );
/**
	if ( strcmp(value, "volume") == 0 )
		return &obj->audio->volume;
	if ( strcmp(value, "position") == 0 )
		return &obj->audio->position;
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
	assert( obj != NULL );

	/* unload ladspa
	 */
	if ( obj->pl_handle )
		dlclose(obj->pl_handle);
	if ( obj->pl_filename )
		free(obj->pl_filename);
	if ( obj->pl_name )
		free(obj->pl_name);

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
	char filename[1024];
	char *ladspa_path;

	if ( strcmp(key, "plugin") == 0 )
	{
		ladspa_path = na_config_get(NA_CONFIG_DEFAULT, "noya.path.ladspa");
		if ( ladspa_path == NULL )
		{
			l_errorf("No noya.path.ladspa config found, fallback to default path");
			ladspa_path = getenv("LADSPA_PATH");
		}
		if ( ladspa_path == NULL )
		{
			l_errorf("No LADSPA_PATH env found, fallback to default path");
			ladspa_path = DEFAULT_LADSPA_PATH;
		}

		snprintf(filename, sizeof(filename), "%s/%s.so", ladspa_path, value);

		obj->pl_name		= strdup(value);
		obj->pl_filename	= strdup(filename);

		/* open plugin
		 */
		obj->pl_handle = dlopen(obj->pl_filename, RTLD_LOCAL | RTLD_NOW);
		if ( obj->pl_handle == NULL )
		{
			l_printf("Cannot open LADSPA %s", obj->pl_name);
			return;
		}

		__load_ladspa(obj);
	}
	else if ( strcmp(key, "distmin") == 0 )
	{
		obj->dist_min = (float)strtod(value, NULL);
		l_printf("obj->distmin = %f", obj->dist_min);
	}
	else if ( strcmp(key, "distmax") == 0 )
		obj->dist_max = (float)strtod(value, NULL);
	else
		l_printf("Invalid configuration %s", key);
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

	/* attach to scene update event
	 */
	na_event_observe(NA_EV_SCENE_UPDATE, __scene_update, obj);
}

void lib_object_unprepare(obj_t *obj)
{
	/* un-attach to scene update event
	 */
	na_event_remove(NA_EV_SCENE_UPDATE, __scene_update, obj);

	/* never remove object from clutter
	 * parent remove all of this tree
	 */
	obj->actor		= NULL;
	obj->group_cube	= NULL;

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

#if 0
void ctl_volume(obj_t *obj, float value)
{
	assert( obj != NULL );
}
#endif
_fn_control lib_object_get_control(char *name)
{
//	if ( strcmp(name, "volume") == 0 )
//		return ctl_volume;
	return NULL;
}
