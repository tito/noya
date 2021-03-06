#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "noya.h"
#include "event.h"
#include "audio.h"
#include "module.h"
#include "config.h"
#include "utils.h"
#include "thread_manager.h"

#define MODULE_NAME "obj_sequencer"

LOG_DECLARE("MOD_OBJ_SEQUENCER");

typedef struct
{
	unsigned int	idx;			/*< index */
	unsigned int	bpm;			/*< bpm to play */
	unsigned int	bpmidx;			/*< current index for this entry */
	char			*filename;		/*< audio filename */
	na_audio_t		*audio;			/*< audio entry (for audio manager) */
} obj_entry_t;

typedef struct
{
	na_scene_t		*scene;
	manager_actor_t	*actor;

	/* rendering issues
	 */
	ClutterActor	*group_cube;

	/* entries
	 */
	short			entry_count;
	obj_entry_t		*entries;

	unsigned int	bpmidx;
	unsigned int	entryidx;
	unsigned int	bpmmax;
	obj_entry_t		**bpmentries;

	double			volume;
} obj_t;

obj_t def_obj = {0};

#if 0
static void *object_resolve_value(obj_t *obj, char *value)
{
	assert( obj != NULL );
	assert( value != NULL );

	if ( strcmp(value, "bpmidx") == 0 )
		return &obj->bpmidx;
	if ( strcmp(value, "entryidx") == 0 )
		return &obj->entryidx;
	return NULL;
}
#endif

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
	l_printf("Error : no global configuration available for %s module", MODULE_NAME);
	return;
}

obj_t *lib_object_new(na_scene_t *scene)
{
	obj_t *obj;

	obj = malloc(sizeof(obj_t));
	if ( obj == NULL )
		return NULL;

	bzero(obj, sizeof(obj_t));

	obj->scene = scene;

	return obj;
}

void lib_object_free(obj_t *obj)
{
	int			idx;
	obj_entry_t *entry;

	assert( obj != NULL );

	for ( idx = 0; idx < obj->entry_count; idx++ )
	{
		entry		= &obj->entries[idx];
		if ( entry->filename )
			free(entry->filename);
		if ( entry->audio )
			na_audio_free(entry->audio);
	}

	if ( obj->bpmentries )
		free(obj->bpmentries);
	if ( obj->entries )
		free(obj->entries);

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
		obj->volume = (float)strtod(value, NULL);
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
			na_audio_set_volume(entry->audio, obj->volume);
		}
		else if ( strcmp(k_prop, "bpm") == 0 )
		{
			entry = &obj->entries[idx];
			entry->bpm = strtol(value, NULL, 10);
		}
		else
			l_printf("Error: invalid configuration %s", key);
	}
}

static void lib_object_ev_bpm(unsigned short type, obj_t *obj, void *data)
{
	unsigned int	oldidx = obj->bpmidx;
	obj_entry_t		*entry, *oldentry;

	/* increment idx
	 */
	obj->bpmidx++;
	if ( obj->bpmidx >= obj->bpmmax )
		obj->bpmidx = 0;

	/* get entries
	 */
	oldentry	= obj->bpmentries[oldidx];
	entry		= obj->bpmentries[obj->bpmidx];

	/* need to stop old entry ?
	 */
	if ( entry != oldentry )
	{
		na_audio_stop(oldentry->audio);
		na_audio_seek(oldentry->audio, 0);

		/* copy volume to new entry
		 * FIXME find a way to copy all params
		 */
		entry->audio->volume = oldentry->audio->volume;
	}

	/* start current entry
	 */
	if ( !na_audio_is_play(entry->audio) )
		na_audio_wantplay(entry->audio);

	l_printf("SET entry=%s, flags=%d", entry->filename, entry->audio->flags);

	/* update index
	 */
	obj->entryidx = entry->idx;
}

void lib_object_prepare(obj_t *obj, manager_actor_t *actor)
{
	ClutterColor	obj_background,
					obj_border;
	ClutterActor	*ac;
	char			number[5];
	int				idx, idx2, bpmidx, bpmmax;
	obj_entry_t		*entry;

	assert( obj != NULL );
	assert( actor != NULL );

	/* save actor
	 */
	obj->actor = actor;

	/* do first thing : some calculation
	 */
	bpmmax = 0;
	for ( idx = 0; idx < obj->entry_count; idx++ )
	{
		entry		= &obj->entries[idx];
		entry->idx	= idx;
		bpmmax		+= entry->bpm;

		/* force loop
		 */
		na_audio_set_loop(entry->audio, 1);
	}

	if ( obj->bpmmax != bpmmax )
	{
		obj->bpmmax = bpmmax;

		/* remove old bpm table if exist
		 */
		if ( obj->bpmentries != NULL )
			free(obj->bpmentries);

		/* create bpm table
		 */
		obj->bpmentries = malloc(sizeof(obj_entry_t *) * obj->bpmmax);
		if ( obj->bpmentries == NULL )
		{
			l_printf("Error: unable to create bpm table !");
			return;
		}

		bpmidx = 0;
		for ( idx = 0; idx < obj->entry_count; idx++ )
		{
			entry = &obj->entries[idx];
			l_printf("entry %d, bpmmax = %d, bpm = %d", idx, obj->bpmmax, entry->bpm);
			for ( idx2 = 0; idx2 < entry->bpm ; idx2++ )
			{
				obj->bpmentries[bpmidx] = entry;
				l_printf("bpmidx = %d", bpmidx);
				bpmidx++;
			}
		}
	}

	/* observe bpm
	 */
	na_event_observe(NA_EV_BPM, (na_event_callback)lib_object_ev_bpm, obj);

	/* RENDERING !
	 */

	obj->group_cube = clutter_group_new();

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

	clutter_container_add_actor(CLUTTER_CONTAINER(actor->rotate_actor), obj->group_cube);
}

void lib_object_unprepare(obj_t *obj)
{
	obj_entry_t *entry;

	/* never remove object from clutter
	 * parent remove all of this tree
	 */
	obj->actor		= NULL;
	obj->group_cube	= NULL;

	/* remove bpm event
	 */
	na_event_remove(NA_EV_BPM, (na_event_callback)lib_object_ev_bpm, obj);

	/* stop audio
	 */
	if ( obj->bpmentries != NULL )
	{
		entry = obj->bpmentries[obj->bpmidx];
		na_audio_stop(entry->audio);
		na_audio_seek(entry->audio, 0);
	}

	obj->bpmidx		= 0;
	obj->entryidx	= 0;
}

void lib_object_update(obj_t *obj)
{
	assert( obj != NULL );
}

void ctl_volume(void *data, float value)
{
	obj_entry_t *entry;
	obj_t		*obj = (obj_t *)data;

	assert( obj != NULL );

	if ( obj->bpmentries == NULL )
		return;

	entry = obj->bpmentries[obj->bpmidx];
	na_audio_set_volume(entry->audio, value);
}

_fn_control lib_object_get_control(char *name)
{
	if ( strcmp(name, "volume") == 0 )
		return ctl_volume;
	return NULL;
}
