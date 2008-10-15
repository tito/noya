#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/queue.h>
#include <assert.h>

#include "noya.h"
#include "config.h"
#include "scene.h"

LOG_DECLARE("SCENE");

na_scene_t *na_scene_load(char *name)
{
	na_scene_t				*scene;
	na_scene_actor_base_t	*actor;
	na_config_t				config = {NULL},
							config_init = {NULL};
	na_config_entry_t		*it;
	na_module_t				*module;
	char					*k_idx,
							*k_prop,
							filename[NA_DEF_PATH_LEN];
	int						act_idx;

	assert( name != NULL );

	l_printf("Load scene %s", name);
	snprintf(filename, sizeof(filename), "%s/%s/%s.ini",
		na_config_get(NA_CONFIG_DEFAULT, "noya.path.scenes"),
		name, name
	);

	scene = malloc(sizeof(na_scene_t));
	if ( scene == NULL )
		goto na_scene_load_error;
	bzero(scene, sizeof(na_scene_t));

	scene->name			= strdup(name);
	scene->bpm			= 125;
	scene->precision	= 4;

	if ( na_config_load(&config_init, filename) )
		goto na_scene_load_error;

	/* invert list !
	 * FIXME use circle list ?
	 */
	for ( it = config_init.lh_first; it != NULL; it = it->next.le_next )
		na_config_set(&config, it->k, it->v);
	na_config_free(&config_init);

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
			na_color_read(&scene->background_color, it->v);
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

		/* handle object
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

			module = na_module_get(k_idx, NA_MOD_OBJECT);
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
				na_scene_prop_set(scene, &scene->def_actor, k_prop, it->v);
			}
			else
			{
				act_idx = strtol(k_idx, NULL, 10);
				actor = na_scene_actor_get(scene, act_idx);
				if ( actor == NULL )
					actor = na_scene_actor_new(scene, act_idx);
				if ( actor == NULL )
				{
					l_errorf("unable to create actor %d", act_idx);
					continue;
				}
				na_scene_prop_set(scene, actor, k_prop, it->v);
			}
			continue;
		}
	}

	na_config_free(&config);

	return scene;

na_scene_load_error:;
	l_errorf("unable to load scene");

	if ( scene )
		na_scene_free(scene);

	na_config_free(&config);

	return NULL;
}

na_scene_actor_base_t *na_scene_actor_new(na_scene_t *scene, int idx)
{
	na_scene_actor_base_t *actor;

	actor = malloc(sizeof(na_scene_actor_base_t));
	if ( actor == NULL )
		return NULL;

	/* copy default actor
	 */
	memcpy(actor, &scene->def_actor, sizeof(scene->def_actor));

	actor->id = idx;

	LIST_INSERT_HEAD(&scene->actors, actor, next);
}

na_scene_actor_base_t *na_scene_actor_get(na_scene_t *scene, int idx)
{
	na_scene_actor_base_t *actor;

	for ( actor = scene->actors.lh_first; actor != NULL; actor = actor->next.le_next)
	{
		if ( actor->id == idx )
			return actor;
	}
	return NULL;
}

void na_scene_prop_set(na_scene_t *scene, na_scene_actor_base_t *actor, char *key, char *value)
{
	void	*data;

	assert( actor != NULL );

	/* base object
	 */
	if ( strcmp(key, "object") == 0 )
	{
		actor->mod = na_module_get(value, NA_MOD_OBJECT);
		if ( actor->mod == NULL )
		{
			l_errorf("no object %s found !", value);
			return;
		}

		actor->data_mod = (*actor->mod->object_new)(scene);
		if ( actor->data_mod == NULL )
		{
			l_errorf("unable to create %s object", value);
			return;
		}
	}
	else if ( strcmp(key, "background.color") == 0 )
		na_color_read(&actor->background_color, value);
	else if ( strcmp(key, "border.color") == 0 )
		na_color_read(&actor->border_color, value);
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
		l_errorf("unknown key property %s", key);
	return;

na_scene_prop_set_invalid_size:;
	l_errorf("invalid size for %s", key);
	return;
}

void na_scene_free(na_scene_t *scene)
{
	na_scene_actor_base_t *actor;

	assert( scene != NULL );

	while ( !LIST_EMPTY(&scene->actors) )
	{
		actor = LIST_FIRST(&scene->actors);
		LIST_REMOVE(actor, next);
		na_scene_actor_free(actor);
	}

	free(scene);
}

void na_scene_actor_free(na_scene_actor_base_t *actor)
{
	if ( actor->mod )
	{
		if ( actor->data_mod )
			(*actor->mod->object_free)(actor->data_mod);
	}

	free(actor);
}
