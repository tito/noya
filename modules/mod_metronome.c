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
	ClutterActor		*text;
} obj_t;

obj_t def_obj = {0};

void lib_init(char **name, int *type)
{
	*name = strdup(MODULE_NAME);
	*type = NA_MOD_MODULE;
}

void lib_exit(void)
{
}

void lib_object_global_config(char *key, char *value)
{
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
	assert( obj != NULL );
	free(obj);
}

void lib_object_config(obj_t *obj, char *key, char *value)
{
	return;
}

static void metronome_bpm(ushort ev_type, void *userdata, void *object)
{
	static char buffer[32];
	obj_t		*obj = (obj_t *)userdata;
	na_bpm_t	*bpm = (na_bpm_t *)object;
	assert( obj != NULL );

	snprintf(buffer, sizeof(buffer), "%d/%d", bpm->beatinmeasure, bpm->measure);
	clutter_threads_enter();
	clutter_label_set_text(obj->text, buffer);
	clutter_threads_leave();
}

/* NOTE: actor is not used with MOD_MODULE
 */
void lib_object_prepare(obj_t *obj, manager_actor_t *actor)
{
	ClutterActor *stage;
	assert( obj != NULL );

	/* RENDERING !
	 */
	stage = clutter_stage_get_default();
	obj->group = clutter_group_new();
	clutter_container_add_actor(CLUTTER_CONTAINER(stage), obj->group);

	/* create text
	 */
	obj->text = clutter_label_new_with_text("Lucida 12", "BPM");
	clutter_actor_set_position(obj->text, 10, 10);

	clutter_container_add_actor(CLUTTER_CONTAINER(obj->group), obj->text);

	na_event_observe(NA_EV_BPM, metronome_bpm, obj);
}

void lib_object_unprepare(obj_t *obj)
{
	na_event_remove(NA_EV_BPM, metronome_bpm, obj);

	/* never remove object from clutter
	 * parent remove all of this tree
	 */
	obj->group		= NULL;
	obj->text		= NULL;
}
