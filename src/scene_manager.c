#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/queue.h>
#include <assert.h>

#include "noya.h"
#include "scene_manager.h"

LOG_DECLARE("SCENE");

#define MAX_PATH	128

static void noya_scene_color_read(color_t *color, char *in)
{
	char	buf[2] = {0};
	char	*dst = (char *)color;

	if ( strlen(in) < 8 )
	{
		l_printf("Invalid color : %s", in);
		return;
	}

	buf[0] = in[0], buf[1] = in[1];
	dst[0] = strtol(buf, NULL, 16);
	buf[0] = in[2], buf[1] = in[3];
	dst[1] = strtol(buf, NULL, 16);
	buf[0] = in[4], buf[1] = in[5];
	dst[2] = strtol(buf, NULL, 16);
	buf[0] = in[6], buf[1] = in[7];
	dst[3] = strtol(buf, NULL, 16);
}

scene_t *noya_scene_load(char *name)
{
	scene_t			*scene;
	scene_actor_base_t *actor;
	config_t		config = {NULL}, config_init = {NULL};
	config_entry_t	*it;
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
			noya_scene_color_read(&scene->background_color, it->v);
			continue;
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

static short noya_scene_ctl_get_id(char *ctl)
{
	if ( strcmp(ctl, "volume") == 0 )
		return SCENE_CONTROL_VOLUME;
	else if ( strcmp(ctl, "none") == 0 )
		return SCENE_CONTROL_NONE;
	l_printf("Invalid control %s", ctl);
	return SCENE_CONTROL_NONE;
}

void noya_scene_prop_set(scene_t *scene, scene_actor_base_t *actor, char *key, char *value)
{
	void	*data;
	char	filename[MAX_PATH];

	assert( actor != NULL );

	/* base type
	 */
	if ( strcmp(key, "type") == 0 )
	{
		if ( actor->type != SCENE_ACTOR_TYPE_BASE )
		{
			l_printf("Error : actor type already specified");
			return;
		}

		if ( strcmp(value, "sample") == 0 )
		{
			LIST_REMOVE(actor, next);

			data = realloc(actor, sizeof(scene_actor_sample_t));
			if ( data == NULL )
			{
				l_printf("Can't realloc actor for %s", value);
				LIST_INSERT_HEAD(&scene->actors, actor, next);
				return;
			}
			actor = data;
			actor->type = SCENE_ACTOR_TYPE_SAMPLE;
			LIST_INSERT_HEAD(&scene->actors, actor, next);
		}
		else
		{
			l_printf("Unknown actor type %s", value);
			return;
		}
	}
	else if ( strcmp(key, "background.color") == 0 )
		noya_scene_color_read(&actor->background_color, value);
	else if ( strcmp(key, "border.color") == 0 )
		noya_scene_color_read(&actor->border_color, value);
	else if ( strcmp(key, "border.width") == 0 )
		actor->border_width = strtol(value, NULL, 10);
	else if ( strcmp(key, "width") == 0 )
		actor->width = strtol(value, NULL, 10);
	else if ( strcmp(key, "height") == 0 )
		actor->height = strtol(value, NULL, 10);
	else if ( strcmp(key, "loop") == 0 )
		actor->is_loop = strtol(value, NULL, 10) > 0 ? 1 : 0;
	else if ( strcmp(key, "ctl.angle") == 0 )
		actor->ctl_angle = noya_scene_ctl_get_id(value);
	else if ( strcmp(key, "ctl.x") == 0 )
		actor->ctl_x = noya_scene_ctl_get_id(value);
	else if ( strcmp(key, "ctl.y") == 0 )
		actor->ctl_y = noya_scene_ctl_get_id(value);

	/* sample type
	 */
	else if ( actor->type == SCENE_ACTOR_TYPE_SAMPLE )
	{
		if ( strcmp(key, "file") == 0 )
		{
			snprintf(filename, sizeof(filename), "%s/%s/%s",
				config_get(CONFIG_DEFAULT, "noya.path.scenes"),
				scene->name,
				value
			);
			SCENE_ACTOR_SAMPLE(actor)->filename = strdup(value);
			SCENE_ACTOR_SAMPLE(actor)->audio = noya_audio_load(filename, actor->id);
		}
	}

	/* others
	 */
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
	scene_actor_sample_t *p;
	switch ( actor->type )
	{
		case SCENE_ACTOR_TYPE_BASE:
			/* nothing to free here */
			break;

		case SCENE_ACTOR_TYPE_SAMPLE:
			p = SCENE_ACTOR_SAMPLE(actor);
			if ( p->filename != NULL )
				free(p->filename);
			if ( p->audio != NULL )
				free(p->audio);
			break;

		default:
			/* should never append */
			assert( 0 );
			break;
	}

	free(actor);
}
