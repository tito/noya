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
MUTEX_IMPORT(context);

static na_ctx_t		s_context;
static ClutterColor	stage_color		= { 0x02, 0x02, 0x22, 0x00 };
static ClutterColor obj_background	= { 0xff, 0xff, 0xff, 0x99 };
static ClutterColor color_white		= { 0xff, 0xff, 0xff, 0x99 };
static ClutterColor color_white_sh	= { 0xff, 0xff, 0xff, 0x33 };
static ClutterActor	*s_menu_start	= NULL,
					*s_menu_options	= NULL,
					*s_menu_quit	= NULL;
static ClutterTexture *tex_bg		= NULL;

static void menu_cursor_click_fade_complete(ClutterActor *actor, gpointer user_data)
{
	clutter_container_remove(
		CLUTTER_CONTAINER(clutter_stage_get_default()),
		actor, NULL
	);
}

static ClutterActor *ui_button_new(ClutterActor *stage, char *text)
{
	uint			bx, by;
	ClutterActor	*container,
					*background,
					*label;

	container = clutter_group_new();

	background = clutter_texture_new_from_file("data/ui-button.png", NULL);
	clutter_actor_get_size(background, &bx, &by);
	clutter_actor_set_position(background, - bx >> 1, - by >> 1);
	clutter_container_add_actor(CLUTTER_CONTAINER(container), background);

	label = clutter_label_new_with_text("Lucida Bold 30", text);
	clutter_label_set_color(label, &color_white);
	clutter_actor_get_size(label, &bx, &by);
	clutter_actor_set_position(label, - bx >> 1, - by >> 1);
	clutter_container_add_actor(CLUTTER_CONTAINER(container), label);

	label = clutter_label_new_with_text("Lucida Bold 30", text);
	clutter_label_set_color(label, &color_white_sh);
	clutter_actor_get_size(label, &bx, &by);
	clutter_actor_set_position(label, - (bx >> 1) + 2, - (by >> 1) + 2);
	clutter_container_add_actor(CLUTTER_CONTAINER(stage), container);

	clutter_actor_show(container);

	return container;
}

static void menu_cursor_handle(unsigned short type, void *userdata, void *data)
{
	ClutterActor	*stage, *circle, *actor;
	ClutterTimeline *timeline;
	ClutterEffectTemplate *template;
	uint			wx, wy;
	float			xpos, ypos;

	stage = clutter_stage_get_default();
	clutter_actor_get_size(stage, &wx, &wy);

	if ( type == NA_EV_CURSOR_NEW )
	{
		tuio_cursor_t	*c = (tuio_cursor_t *)data;
		xpos = c->xpos;
		ypos = c->ypos;
	}
	else if ( type == NA_EV_BUTTONPRESS )
	{
		ClutterButtonEvent *ev = (ClutterButtonEvent *)data;
		xpos = (float)ev->x / (float)wx;
		ypos = (float)ev->y / (float)wy;
	}

	circle = clutter_circle_new_with_color(&obj_background);
	clutter_circle_set_angle_stop(CLUTTER_CIRCLE(circle), 360);
	clutter_circle_set_radius(CLUTTER_CIRCLE(circle), 5);
	clutter_actor_set_position(CLUTTER_CIRCLE(circle), xpos * (float)wx, ypos * (float)wy);
	clutter_actor_show(circle);

	timeline = clutter_timeline_new_for_duration(400);
	template = clutter_effect_template_new(timeline, CLUTTER_ALPHA_SINE_INC);
	clutter_effect_fade(template, circle, 0x00, menu_cursor_click_fade_complete, NULL);
	clutter_effect_scale(template, circle, 20, 20, NULL, NULL);

	clutter_container_add_actor(CLUTTER_CONTAINER(stage), circle);

	/**/
	clutter_stage_ensure_current(stage);
	actor = clutter_stage_get_actor_at_pos(stage, xpos * (float)wx, ypos * (float)wy);
	l_printf("found %p", actor);
	/**/
}

int context_menu_activate(void *ctx, void *userdata)
{
	ClutterActor	*stage;
	uint			wx, wy;

	na_event_observe(NA_EV_CURSOR_NEW, menu_cursor_handle, NULL);
	na_event_observe(NA_EV_BUTTONPRESS, menu_cursor_handle, NULL);

	stage = clutter_stage_get_default();
	clutter_stage_set_color(CLUTTER_STAGE(stage), &stage_color);
	clutter_actor_get_size(stage, &wx, &wy);

	tex_bg = clutter_texture_new_from_file("data/background.png", NULL);
	clutter_actor_set_size(CLUTTER_ACTOR(tex_bg), wx, wy);
	clutter_container_add_actor(CLUTTER_CONTAINER(stage), CLUTTER_ACTOR(tex_bg));

	s_menu_start = ui_button_new(stage, "Démarrer");
	clutter_actor_set_position(s_menu_start, (wx >> 1), 100);

	s_menu_options = ui_button_new(stage, "Options");
	clutter_actor_set_position(s_menu_options, (wx >> 1), 250);

	s_menu_quit = ui_button_new(stage, "Quitter");
	clutter_actor_set_position(s_menu_quit, (wx >> 1), 400);

	return 0;
}

int context_menu_deactivate(void *ctx, void *userdata)
{
	na_event_remove(NA_EV_CURSOR_NEW, menu_cursor_handle, NULL);

	return 0;
}

int context_menu_update(void *ctx, void *userdata)
{
	return 1000;
}

void context_menu_register()
{
	bzero(&s_context, sizeof(s_context));

	strncpy(s_context.name, "menu", sizeof(s_context.name));
	s_context.fn_activate	= context_menu_activate;
	s_context.fn_deactivate	= context_menu_deactivate;
	s_context.fn_update		= context_menu_update;

	MUTEX_LOCK(context);
	na_ctx_register(&s_context);
	MUTEX_UNLOCK(context);
}
