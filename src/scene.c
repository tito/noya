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

/* settings for Noya actors.
 * NOTE: order are important, do not move !
 */
static char *g_na_settings[] = {
	"object",
	"width",
	"height",
	"background_color",
	"border_color",
	"ctl_x",
	"ctl_y",
	"ctl_angle",
	"loop",
	NULL
};

static void na_scene_prop_set(na_scene_t *scene, na_scene_actor_t *actor, char *key, config_setting_t *value)
{
	const char *s_value;

	assert( actor != NULL );

	/* base object
	 */
	if ( strcmp(key, "object") == 0 )
	{
		s_value = config_setting_get_string(value);
		actor->mod = na_module_get(s_value, NA_MOD_OBJECT);
		if ( actor->mod == NULL )
		{
			l_errorf("no object %s found !", s_value);
			return;
		}

		actor->data_mod = (*actor->mod->object_new)(scene);
		if ( actor->data_mod == NULL )
		{
			l_errorf("unable to create %s object", s_value);
			return;
		}
	}
	else if ( strcmp(key, "background_color") == 0 )
		na_color_read(&actor->background_color, config_setting_get_string(value));
	else if ( strcmp(key, "border_color") == 0 )
		na_color_read(&actor->border_color, config_setting_get_string(value));
	else if ( strcmp(key, "border_width") == 0 )
		actor->border_width = config_setting_get_int(value);
	else if ( strcmp(key, "width") == 0 )
		actor->width = config_setting_get_int(value);
	else if ( strcmp(key, "height") == 0 )
		actor->height = config_setting_get_int(value);
	else if ( strcmp(key, "loop") == 0 )
		actor->is_loop = config_setting_get_int(value);
	else if ( strcmp(key, "ctl_angle") == 0 && actor->mod)
		actor->ctl_angle = (*actor->mod->object_get_control)((void*)config_setting_get_string(value));
	else if ( strcmp(key, "ctl_x") == 0 && actor->mod )
		actor->ctl_x = (*actor->mod->object_get_control)((void*)config_setting_get_string(value));
	else if ( strcmp(key, "ctl_y") == 0 && actor->mod )
		actor->ctl_y = (*actor->mod->object_get_control)((void*)config_setting_get_string(value));
	else
		l_errorf("unknown key property %s", key);
	return;
}

na_scene_t *na_scene_load(char *name)
{
	na_scene_t				*scene;
	na_scene_actor_t		*actor;
	config_t				config = {NULL};
	const char				*k_prop;
	char					filename[NA_DEF_PATH_LEN],
							path[NA_DEF_PATH_LEN];
	int						act_idx, i, found;
	config_setting_t		*setting,
							*setting_actors,
							*setting_actor,
							*k, *classes;
	char					**k_setting;

	assert( name != NULL );

	setting = config_lookup(&g_config, "noya.path.scenes");
	if ( setting == NULL )
	{
		l_errorf("no noya.path.scenes configuration !");
		return NULL;
	}

	if ( config_setting_length(setting) == 0 )
	{
		l_errorf("empty noya.path.scene configuration !");
		return NULL;
	}

	/* try to find scene
	 */

	l_printf("Load scene %s", name);

	for ( i = 0, found = 0; i < config_setting_length(setting); i++ )
	{
		snprintf(path, sizeof(path), "%s/%s",
			config_setting_get_string_elem(setting, i),
			name
		);
		snprintf(filename, sizeof(filename), "%s/%s.cfg",
			path, name
		);
		l_printf("Check %s", filename);
		if ( fileexist(filename) )
		{
			found = 1;
			break;
		}
	}

	if ( !found )
	{
		l_errorf("unable to found the scene : %s", name);
		return NULL;
	}

	/* load configuration
	 */
	config_init(&config);
	if ( !config_read_file(&config, filename) )
	{
		l_errorf("unable to read scene file %s", filename);
		l_errorf("line %d: %s",
				config_error_line(&g_config),
				config_error_text(&g_config)
			);
		return NULL;
	}

	/* load scene
	 */
	scene = malloc(sizeof(na_scene_t));
	if ( scene == NULL )
		goto na_scene_load_error;
	bzero(scene, sizeof(na_scene_t));

	scene->name			= strdup(name);
	scene->path			= strdup(path);

	setting = config_lookup_with_default(&g_config, &config, "scene.bpm");
	if ( setting )
		scene->bpm			= config_setting_get_int(setting);
	if ( scene->bpm <= 0 )
		scene->bpm			= 125;

	setting = config_lookup_with_default(&g_config, &config, "scene.precision");
	if ( setting )
		scene->precision	= config_setting_get_int(setting);
	if ( scene->precision <= 0 )
		scene->precision	= 4;

	setting = config_lookup_with_default(&g_config, &config, "scene.measure");
	if ( setting )
		scene->measure		= config_setting_get_int(setting);
	if ( scene->measure <= 0 )
		scene->measure		= 4;

	classes = config_lookup(&g_config, "noya.scene.classes");

	setting_actors = config_lookup(&config, "scene.actors");
	if ( setting_actors == NULL )
	{
		l_errorf("no actors in scene ?");
		goto na_scene_load_error;
	}

	for ( i = 0; i < config_setting_length(setting_actors); i++ )
	{
		setting_actor = config_setting_get_elem(setting_actors, i);
		if ( setting_actor == NULL )
			continue;

		/* create actor
		 */
		k = config_setting_get_member(setting_actor, "id");
		if ( k == NULL )
		{
			l_errorf("no id defined for actor at index %d", i);
			continue;
		}

		act_idx = config_setting_get_int(k);
		actor = na_scene_actor_get(scene, act_idx);
		if ( actor == NULL )
		{
			actor = na_scene_actor_new(scene, act_idx);
			if ( actor == NULL )
			{
				l_errorf("unable to create actor %d", act_idx);
				continue;
			}
		}
		else
		{
			l_printf("Warning: actor %d already defined !", act_idx);
		}

		/* define class
		 */
		k = config_setting_get_member(setting_actor, "class");
		if ( k != NULL )
		{
			k_prop = config_setting_get_string(k);
			if ( k_prop )
			{
				if ( actor->class )
					free(actor->class), actor->class = NULL;
				actor->class = strdup(k_prop);
			}
		}
		else
		{
			if ( actor->class )
				free(actor->class), actor->class = NULL;
			actor->class = strdup("default");
		}

		/* read all properties
		 */
		k_setting = (char **)g_na_settings;
		while ( *k_setting != NULL )
		{
			k = config_setting_get_member_with_class(
				classes, actor->class, setting_actor, *k_setting
			);
			if ( k != NULL )
				na_scene_prop_set(scene, actor, *k_setting, k);
			k_setting++;
		}

		/* read module properties
		 */
		if ( actor->mod == NULL || actor->mod->settings == NULL )
			continue;

		k_setting = (char **)actor->mod->settings;
		while ( *k_setting != NULL )
		{
			k = config_setting_get_member_with_class(
				classes, actor->class, setting_actor, *k_setting
			);
			if ( k != NULL )
				(*actor->mod->object_config)(actor->data_mod, *k_setting, k);
			k_setting++;
		}
	}

	config_destroy(&config);

	return scene;

na_scene_load_error:;
	l_errorf("unable to load scene");

	if ( scene )
		na_scene_free(scene);

	config_destroy(&config);

	return NULL;
}

na_scene_actor_t *na_scene_actor_new(na_scene_t *scene, int idx)
{
	na_scene_actor_t *actor;

	actor = malloc(sizeof(na_scene_actor_t));
	if ( actor == NULL )
		return NULL;
	bzero(actor, sizeof(na_scene_actor_t));

	actor->id = idx;

	LIST_INSERT_HEAD(&scene->actors, actor, next);

	return actor;
}

na_scene_actor_t *na_scene_actor_get(na_scene_t *scene, int idx)
{
	na_scene_actor_t *actor;

	for ( actor = scene->actors.lh_first; actor != NULL; actor = actor->next.le_next)
	{
		if ( actor->id == idx )
			return actor;
	}
	return NULL;
}

void na_scene_free(na_scene_t *scene)
{
	na_scene_actor_t *actor;

	assert( scene != NULL );

	while ( !LIST_EMPTY(&scene->actors) )
	{
		actor = LIST_FIRST(&scene->actors);
		LIST_REMOVE(actor, next);
		na_scene_actor_free(actor);
	}

	if ( scene->name )
		free(scene->name);
	if ( scene->path )
		free(scene->path);
	free(scene);
}

void na_scene_actor_free(na_scene_actor_t *actor)
{
	if ( actor->mod )
	{
		if ( actor->data_mod )
			(*actor->mod->object_free)(actor->data_mod);
	}

	free(actor);
}
