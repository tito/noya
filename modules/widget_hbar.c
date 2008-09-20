#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "../src/noya.h"
#include "../src/module.h"
#include "../src/audio.h"
#include "../src/thread_manager.h"

#define MODULE_NAME "hbar"

LOG_DECLARE("MOD_WIDGET_HBAR");

typedef struct
{
	scene_t			*scene;
	manager_actor_t	*actor;

	ClutterActor	*group_hbar;
	ClutterActor	*hbar_gauge;

	float			*value;
} widget_t;

void lib_init(char **name, int *type)
{
	*name = strdup(MODULE_NAME);
	*type = MODULE_TYPE_WIDGET;
}

void lib_exit(void)
{
}

void *lib_widget_new(scene_t *scene)
{
	widget_t *widget;

	widget = malloc(sizeof(widget_t));
	if ( widget == NULL )
		return NULL;
	bzero(widget, sizeof(widget_t));

	widget->scene = scene;

	return widget;
}

void lib_widget_free(widget_t *widget)
{
	assert( widget != NULL );

	free(widget);
}

void *lib_widget_clone(widget_t *widget, scene_t *scene)
{
	widget_t *clone;

	assert( widget != NULL );
	assert( scene != NULL );

	clone = malloc(sizeof(widget_t));
	if ( clone == NULL )
		return NULL;

	memcpy(clone, widget, sizeof(widget));

	clone->scene = scene;

	return clone;
}

void lib_widget_config(widget_t *widget, char *key, char *value)
{
	assert( widget != NULL );
	assert( key != NULL );
	assert( value != NULL );
}

void lib_widget_prepare(widget_t *widget, manager_actor_t *actor)
{
	ClutterColor	obj_background,
					obj_border;
	ClutterActor	*ac;

	noya_color_copy(&obj_background, &actor->scene_actor->background_color);
	noya_color_copy(&obj_border, &actor->scene_actor->border_color);

	/* save actor
	 */
	widget->actor = actor;

	/* create hbar object
	 */
	widget->group_hbar = clutter_group_new();
	clutter_container_add_actor(CLUTTER_CONTAINER(actor->clutter_actor), widget->group_hbar);

	ac = clutter_rectangle_new_with_color(&obj_background);
	clutter_actor_set_width(ac, actor->scene_actor->width);
	clutter_actor_set_height(ac, 10);
	clutter_actor_set_y(ac, actor->scene_actor->height + 16);
	clutter_container_add_actor(CLUTTER_CONTAINER(widget->group_hbar), ac);

	ac = clutter_rectangle_new_with_color(&obj_border);
	clutter_actor_set_width(ac, actor->scene_actor->width - 4);
	clutter_actor_set_height(ac, 6);
	clutter_actor_set_x(ac, 2);
	clutter_actor_set_y(ac, actor->scene_actor->height + 18);
	clutter_container_add_actor(CLUTTER_CONTAINER(widget->group_hbar), ac);
	widget->hbar_gauge = ac;
}

void lib_widget_unprepare(widget_t *widget)
{
	assert( widget != NULL );
	widget->group_hbar = NULL;
	widget->value = NULL;
}

void lib_widget_update(widget_t *widget)
{
	assert( widget != NULL );
	clutter_actor_set_width(widget->hbar_gauge, widget->actor->scene_actor->width * (*widget->value));
}

void lib_widget_set_data(widget_t *widget, float *value)
{
	assert( widget != NULL );
	widget->value = value;
}
