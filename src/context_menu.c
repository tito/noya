#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <clutter/clutter.h>
#include "noya.h"
#include "context.h"
#include "event.h"
#include "thread_input.h"
#include "actors/clutter-circle.h"

#define MAX_CURSORS		20

LOG_DECLARE("CTXMENU");

static na_ctx_t		s_context;
static ClutterColor	stage_color		= { 0x02, 0x02, 0x22, 0x00 };
static ClutterColor obj_background	= { 0xff, 0xff, 0xff, 0x99 };
static ClutterColor obj_red			= { 0xff, 0x00, 0x00, 0x99 };
static ClutterActor	*s_menu_start	= NULL;

static void menu_cursor_click_fade_complete(ClutterActor *actor, gpointer user_data)
{
	clutter_container_remove(
		CLUTTER_CONTAINER(clutter_stage_get_default()),
		actor, NULL
	);
}

static void menu_cursor_handle(unsigned short type, void *userdata, void *data)
{
	ClutterActor	*stage, *circle, *actor;
	ClutterTimeline *timeline;
	ClutterEffectTemplate *template;
	tuio_cursor_t	*c = (tuio_cursor_t *)data;
	uint			wx, wy;

	clutter_threads_enter();

	stage = clutter_stage_get_default();
	clutter_actor_get_size(stage, &wx, &wy);

	circle = clutter_circle_new_with_color(&obj_background);
	clutter_circle_set_angle_stop(CLUTTER_CIRCLE(circle), 360);
	clutter_circle_set_radius(CLUTTER_CIRCLE(circle), 5);
	clutter_actor_set_position(CLUTTER_CIRCLE(circle), c->xpos * (float)wx, c->ypos * (float)wy);
	clutter_actor_show(circle);

	timeline = clutter_timeline_new_for_duration(400);
	template = clutter_effect_template_new(timeline, CLUTTER_ALPHA_SINE_INC);
	clutter_effect_fade(template, circle, 0x00, menu_cursor_click_fade_complete, NULL);
	clutter_effect_scale(template, circle, 20, 20, NULL, NULL);

	clutter_container_add_actor(CLUTTER_CONTAINER(stage), circle);

	/**
	actor = clutter_stage_get_actor_at_pos(stage, c->xpos * (float)wx, c->ypos * (float)wy);
	l_printf("found %p", actor);
	**/

	clutter_threads_leave();
}

int context_menu_activate(void *ctx, void *userdata)
{
	ClutterActor	*stage;

	na_event_observe(NA_EV_CURSOR_NEW, menu_cursor_handle, NULL);

	clutter_threads_enter();
	stage = clutter_stage_get_default();
	clutter_stage_set_color(CLUTTER_STAGE(stage), &stage_color);

	s_menu_start = clutter_label_new_with_text("Lucida 30", "Start !");
	clutter_label_set_color(s_menu_start, &obj_red);
	clutter_container_add_actor(CLUTTER_CONTAINER(stage), s_menu_start);

	clutter_threads_leave();

	return 0;
}

int context_menu_deactivate(void *ctx, void *userdata)
{
	na_event_remove(NA_EV_CURSOR_NEW, menu_cursor_handle, NULL);

	return 0;
}

int context_menu_update(void *ctx, void *userdata)
{
	usleep(20000);
	return 0;
}

void context_menu_register()
{
	bzero(&s_context, sizeof(s_context));

	strncpy(s_context.name, "menu", sizeof(s_context.name));
	s_context.fn_activate	= context_menu_activate;
	s_context.fn_deactivate	= context_menu_deactivate;
	s_context.fn_update		= context_menu_update;

	na_ctx_register(&s_context);
}
