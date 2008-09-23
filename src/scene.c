#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/queue.h>
#include <assert.h>

#include "noya.h"
#include "scene.h"

LOG_DECLARE("SCENE");

#define MAX_PATH	128

scene_t *noya_scene_load(char *name)
{
	scene_t			*scene;
	scene_actor_base_t *actor;
	config_t		config = {NULL}, config_init = {NULL};
	config_entry_t	*it;
	module_t		*module;
	char			*k_idx, *k_prop,
					filename[MAX_PATH];
	int				act_idx;

	assert( name != NULL );

	l_printf("Load scene %s", name);
	snprintf(filename, MAX_PATH, "%s/%s/%s.ini",
		config_get(CONFIG_DEFAULT, "noya.path.scenes"),
		name, name
	);

	scene = malloc(sizeof(scene_t));
	if ( scene == NULL )
		goto noya_scene_load_error;
	bzero(scene, sizeof(scene_t));

	scene->name = strdup(name);
	scene->bpm = 125;
	scene->precision = 4;

	if ( config_load(&config_init, filename) )
		goto noya_scene_load_error;

	/* invert list !
	 * FIXME use circle list ?
	 */
	for ( it = config_init.lh_first; it != NULL; it = it->next.le_next )
		config_set(&config, it->k, it->v);
	config_free(&config_init);

	for ( it = config.lh_first; it != NULL; it = it->next.le_next )
	{
		if ( strstr(it->k, "scene.") != it->k )
			continue;

		/* FIXME handle scene infos
		 */
		if ( strstr(it->k, "scene.infos.") == it->k )
			continue;

		/* Handle scene properties
		 */
		if ( strcmp(it->k, "scene.background.color") == 0 )
		{
			noya_color_read(&scene->background_color, it->v);
			continue;
		}

		if ( strstr(it->k, "scene.config.") == it->k )
		{
			/* extract index object
			 */
			k_prop = strtok(it->k + strlen("scene.object."), ".");
			if ( k_prop == NULL )
				continue;

			if ( strcmp(k_prop, "bpm") == 0 )
				scene->bpm = strtol(it->v, NULL, 10);
			else if ( strcmp(k_prop, "precision") == 0 )
				scene->precision = strtol(it->v, NULL, 10);
		}

		/* handle types
		 */
		if ( strstr(it->k, "scene.object.") == it->k )
		{
			/* extract index object
			 */
			k_idx = strtok(it->k + strlen("scene.object."), ".");
			if ( k_idx == NULL )
				continue;

			k_prop = strtok(NULL, "");
			if ( k_prop == NULL )
				continue;

			module = noya_module_get(k_idx, MODULE_TYPE_OBJECT);
			if ( module == NULL )
				continue;

			(*module->object_global_config)(k_prop, it->v);
		}

		/* handle actors
		 */
		if ( strstr(it->k, "scene.act.") == it->k )
		{
			/* extract index
			 */
			k_idx = strtok(it->k + strlen("scene.act."), ".");
			if ( k_idx == NULL )
				continue;

			/* extract prop
			 */
			k_prop = strtok(NULL, "");
			if ( k_prop == NULL )
				continue;

			if ( strcmp(k_idx, "default") == 0 )
			{
				noya_scene_prop_set(scene, &scene->def_actor, k_prop, it->v);
			}
			else
			{
				act_idx = strtol(k_idx, NULL, 10);
				actor = noya_scene_actor_get(scene, act_idx);
				if ( actor == NULL )
					actor = noya_scene_actor_new(scene, act_idx);
				if ( actor == NULL )
				{
					l_printf("Error while creating new actor %d", act_idx);
					continue;
				}
				noya_scene_prop_set(scene, actor, k_prop, it->v);
			}
			continue;
		}
	}

	config_free(&config);

	return scene;

noya_scene_load_error:;
	l_printf("Error while loading scene");

	if ( scene )
		noya_scene_free(scene);

	config_free(&config);

	return NULL;
}

scene_actor_base_t *noya_scene_actor_new(scene_t *scene, int idx)
{
	scene_actor_base_t *actor;

	actor = malloc(sizeof(scene_actor_base_t));
	if ( actor == NULL )
		return NULL;

	/* copy default actor
	 */
	memcpy(actor, &scene->def_actor, sizeof(scene->def_actor));

	actor->id = idx;

	LIST_INSERT_HEAD(&scene->actors, actor, next);
}

scene_actor_base_t *noya_scene_actor_get(scene_t *scene, int idx)
{
	scene_actor_base_t *actor;

	for ( actor = scene->actors.lh_first; actor != NULL; actor = actor->next.le_next)
	{
		if ( actor->id == idx )
			return actor;
	}
	return NULL;
}

void noya_scene_prop_set(scene_t *scene, scene_actor_base_t *actor, char *key, char *value)
{
	void	*data;
	char	filename[MAX_PATH];

	assert( actor != NULL );

	/* base object
	 */
	if ( strcmp(key, "object") == 0 )
	{
		actor->mod = noya_module_get(value, MODULE_TYPE_OBJECT);
		if ( actor->mod == NULL )
		{
			l_printf("Error: no object %s found !", value);
			return;
		}

		actor->data_mod = (*actor->mod->object_new)(scene);
		if ( actor->data_mod == NULL )
		{
			l_printf("Error: unable to create %s object", value);
			return;
		}
	}
	else if ( strcmp(key, "background.color") == 0 )
		noya_color_read(&actor->background_color, value);
	else if ( strcmp(key, "border.color") == 0 )
		noya_color_read(&actor->border_color, value);
	else if ( strcmp(key, "border.width") == 0 )
		actor->border_width = strtol(value, NULL, 10);
	else if ( strcmp(key, "width") == 0 )
		actor->width = strtol(value, NULL, 10);
	else if ( strcmp(key, "height") == 0 )
		actor->height = strtol(value, NULL, 10);
	else if ( strcmp(key, "loop") == 0 )
		actor->is_loop = strtol(value, NULL, 10) > 0 ? 1 : 0;
	else if ( strcmp(key, "ctl.angle") == 0 && actor->mod)
		actor->ctl_angle = (*actor->mod->object_get_control)(value);
	else if ( strcmp(key, "ctl.x") == 0 && actor->mod )
		actor->ctl_x = (*actor->mod->object_get_control)(value);
	else if ( strcmp(key, "ctl.y") == 0 && actor->mod )
		actor->ctl_y = (*actor->mod->object_get_control)(value);
	else if ( actor->mod )
		(*actor->mod->object_config)(actor->data_mod, key, value);
	else
		l_printf("Error : unknown key property %s", key);
	return;

noya_scene_prop_set_invalid_size:;
	l_printf("Error : invalid size for %s", key);
	return;
}

void noya_scene_free(scene_t *scene)
{
	scene_actor_base_t *actor;

	assert( scene != NULL );

	while ( !LIST_EMPTY(&scene->actors) )
	{
		actor = LIST_FIRST(&scene->actors);
		LIST_REMOVE(actor, next);
		noya_scene_actor_free(actor);
	}

	free(scene);
}

void noya_scene_actor_free(scene_actor_base_t *actor)
{
	if ( actor->mod )
	{
		if ( actor->data_mod )
			(*actor->mod->object_free)(actor->data_mod);
	}

	free(actor);
}
