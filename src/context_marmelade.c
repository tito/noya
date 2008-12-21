#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/queue.h>
#include <assert.h>
#include <clutter/clutter.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>

#include <string.h>
#include "noya.h"
#include "event.h"
#include "thread_manager.h"
#include "thread_input.h"
#include "thread_audio.h"
#include "scene.h"
#include "utils.h"
#include "context.h"
#include "rtc.h"
#include "db.h"

#include "actors/clutter-round-rectangle.h"

#define UI_MARGIN		3
#define UIT_MARGIN		6

LOG_DECLARE("MARMELADE");
MUTEX_IMPORT(context);

typedef struct
{
	char			*title;
	char			*filename;
	ClutterActor	*actor;
} ac_file_entry_t;

typedef struct ac_sound_s
{
	ClutterActor	*actor;
	na_audio_t		*audio;

	SLIST_ENTRY(ac_sound_s)		next;
} ac_sound_t;

typedef struct
{
	ac_sound_t		*slh_first;
} ac_sound_list_t;

typedef struct ac_cursor_s
{
	uint			s_id;
	float			xpos,
					ypos;

	ClutterActor	*actor;
	ac_file_entry_t	*file;
	ac_sound_t		*sound;

	SLIST_ENTRY(ac_cursor_s)	next;
} ac_cursor_t;

typedef struct
{
	ac_cursor_t		*slh_first;
} ac_cursor_list_t;

static na_ctx_t			s_context;
static ClutterColor		stage_color		= { 0x00, 0x00, 0x00, 0xff },
						scrollbars_bg	= { 0x15, 0x19, 0x14, 0xff },
						scrollbars_bc	= { 0x2e, 0x5d, 0x63, 0xff },
						playground_bg	= { 0x15, 0x19, 0x14, 0xff },
						playground_bc	= { 0x2e, 0x5d, 0x63, 0xff },
						menu_bg		= { 0x15, 0x19, 0x14, 0xff },
						menu_bc		= { 0x2e, 0x5d, 0x63, 0xff },
						transitions_bg	= { 0x15, 0x19, 0x14, 0xff },
						transitions_bc	= { 0x2e, 0x5d, 0x63, 0xff },
						filelist_bg		= { 0x15, 0x19, 0x14, 0xff },
						filelist_bc		= { 0x2e, 0x5d, 0x63, 0xff },
						file_fg			= { 0xff, 0xff, 0xff, 0xff },
						sound_bc		= { 0xc9, 0x44, 0x45, 0xff },
						sound_bg		= { 0x45, 0x23, 0x21, 0xff };
static ac_file_entry_t	*files			= NULL;
static int				filescount		= 0;
static float			scrollbar_start	= 0;

static ClutterActor		*ca_ui,
						*ca_menu,			*cr_menu		= NULL,
						*ca_scrollbars,		*cr_scrollbars	= NULL,
						*ca_filelist,		*cr_filelist	= NULL,
						*ca_actions,		*cr_actions		= NULL,
						*ca_playground,		*cr_playground	= NULL,
						*ca_actions_stop,
						*ca_actions_delete,
						*ac_scrollbox;

static ac_cursor_list_t	ac_cursors		= {0};
static ac_sound_list_t	ac_sounds		= {0};

void marmelade_filelist_refresh_scroll();

void clutter_actor_set_geometry_from(ClutterActor *dst, ClutterActor *src, int px, int py)
{
	ClutterGeometry	geom = { 0 };

	clutter_actor_get_geometry(src, &geom);
	geom.x		= px;
	geom.y		= py;
	geom.width	-= px + px;
	geom.height	-= py + py;

	l_printf("Geometry will be : +%d+%d:%dx%d", geom.x, geom.y, geom.width, geom.height);
	clutter_actor_set_geometry(dst, &geom);
}

int clutter_in_actor(ClutterActor *obj, ClutterActor *surf)
{
	gint	ox, oy, ow, oh;
	gint	sx, sy, sw, sh;

	clutter_actor_get_transformed_position(obj, &ox, &oy);
	clutter_actor_get_transformed_position(surf, &sx, &sy);
	clutter_actor_get_transformed_size(obj, &ow, &oh);
	clutter_actor_get_transformed_size(surf, &sw, &sh);

	ox += ow >> 1;
	oy += oh >> 1;
	return (ox > sx && ox < sx + sw && oy > sy && oy < sy + sh) ? 1 : 0;
}

static void marmelade_cursor_handle(unsigned short type, void *userdata, void *data)
{
	ClutterActor		*stage, *actor;
	uint				wx, wy, i;
	ac_cursor_t			*cur;
	tuio_cursor_t		*tuio_cur;
	ac_file_entry_t		*file;
	ac_sound_t			*sound;
	ClutterGeometry		geom;

	stage = clutter_stage_get_default();
	clutter_actor_get_size(stage, &wx, &wy);

	tuio_cur = (tuio_cursor_t *)data;

	switch ( type )
	{
		case NA_EV_CURSOR_NEW:
			cur = malloc(sizeof(ac_cursor_t));
			if ( cur == NULL )
				return;
			bzero(cur, sizeof(ac_cursor_t));

			cur->s_id = tuio_cur->s_id;
			cur->xpos = tuio_cur->xpos * wx;
			cur->ypos = tuio_cur->ypos * wy;

			SLIST_INSERT_HEAD(&ac_cursors, cur, next);
			break;

		case NA_EV_CURSOR_DEL:
			SLIST_FOREACH(cur, &ac_cursors, next)
			{
				if ( cur->s_id != tuio_cur->s_id )
					continue;
				SLIST_REMOVE(&ac_cursors, cur, ac_cursor_s, next);
				return;
			}
			return;

		case NA_EV_CURSOR_SET:
			SLIST_FOREACH(cur, &ac_cursors, next)
			{
				if ( cur->s_id == tuio_cur->s_id )
					break;
			}
			if ( cur == NULL )
				return;
			cur->xpos = tuio_cur->xpos * wx;
			cur->ypos = tuio_cur->ypos * wy;
	}

	if ( type == NA_EV_CURSOR_NEW )
	{
		actor = clutter_stage_get_actor_at_pos(CLUTTER_STAGE(stage), cur->xpos, cur->ypos);
		while ( actor != NULL && (cur->actor == NULL || cur->file == NULL) )
		{
			if ( actor == ac_scrollbox )
			{
				cur->actor = actor;
			}
			else
			{
				SLIST_FOREACH(sound, &ac_sounds, next)
				{
					if ( actor != sound->actor )
						continue;
					cur->sound = sound;
				}

				if ( cur->sound == NULL )
				{
					for ( i = 0, file = files; i < filescount; i++, file++ )
					{
						if ( actor != file->actor )
							continue;
						cur->file = file;
					}
				}
			}

			actor = clutter_actor_get_parent(actor);
		}
	}

	if ( cur->actor )
	{
		if ( cur->actor == ac_scrollbox )
		{
			float y = cur->ypos, min_y, max_y;
			min_y = clutter_actor_get_y(clutter_actor_get_parent(cur->actor)),
			max_y = min_y + clutter_actor_get_height(clutter_actor_get_parent(cur->actor));

			if ( y < min_y )
				y = min_y;
			if ( y > max_y )
				y = max_y;

			scrollbar_start = (y - min_y) / (max_y - min_y);
			marmelade_filelist_refresh_scroll();
		}
	}

	if ( cur->file )
	{
		/* no sound yet, wait to move it outside fileset
		 */
		if ( cur->sound == NULL )
		{
			clutter_actor_get_geometry(ca_filelist, &geom);
			if ( cur->xpos < geom.x + geom.width )
				return;

			cur->sound = malloc(sizeof(ac_sound_t));
			if ( cur->sound == NULL )
				return;
			bzero(cur->sound, sizeof(ac_sound_t));

			cur->sound->audio = na_audio_load(cur->file->filename);
			na_audio_set_loop(cur->sound->audio, 1);

			cur->sound->actor = clutter_round_rectangle_new_with_color(&sound_bg);
			clutter_round_rectangle_set_border_color(cur->sound->actor, &sound_bc);
			clutter_actor_set_size(cur->sound->actor, 30, 30);

			clutter_container_add_actor(CLUTTER_CONTAINER(stage), cur->sound->actor);

			SLIST_INSERT_HEAD(&ac_sounds, cur->sound, next);
		}
	}

	if ( cur->sound )
	{
		clutter_actor_set_position(cur->sound->actor, cur->xpos, cur->ypos);
		na_audio_set_volume(cur->sound->audio, tuio_cur->ypos);

		if ( clutter_in_actor(cur->sound->actor, ca_actions_stop) )
		{
			na_audio_stop(cur->sound->audio);
		}
		else if ( clutter_in_actor(cur->sound->actor, ca_actions_delete) )
		{
			cur->file = NULL;
			na_audio_stop(cur->sound->audio);
			clutter_container_remove_actor(CLUTTER_CONTAINER(stage), cur->sound->actor);
			SLIST_REMOVE(&ac_cursors, cur, ac_cursor_s, next);
			//free(cur->sound);
//			free(cur);
		}
		else if ( clutter_in_actor(cur->sound->actor, ca_playground) )
		{
			na_audio_wantplay(cur->sound->audio);
		}
	}
}

void marmelade_filelist_free()
{
	int				i;
	ac_file_entry_t *entry;

	for ( entry = files, i = 0; i < filescount; i++, entry++ )
	{
		if ( entry->filename)
			free(entry->filename);
		if ( entry->title	)
			free(entry->title);
		free(entry);
	}

	files		= NULL;
	filescount	= 0;
}

void marmelade_filelist_refresh_scroll()
{
	int					i, y = 0;
	ClutterGeometry		geom;
	ClutterActor		*actor;

	for ( i = 0; i < filescount; i++ )
		clutter_actor_hide(files[i].actor);

	clutter_actor_get_geometry(ca_filelist, &geom);
	y = UIT_MARGIN;
	geom.height -= UIT_MARGIN << 1;
	geom.width	-= UIT_MARGIN << 1;

	for ( i = (int)((float)scrollbar_start * (float)filescount); i < filescount && y < geom.height ; i++ )
	{
		actor = files[i].actor;
		clutter_actor_set_position(actor, UIT_MARGIN, y);
		clutter_actor_set_width(actor, geom.width - 50);
		y += clutter_actor_get_height(actor) + UIT_MARGIN;
		if ( y < geom.height )
			clutter_actor_show(actor);
	}

	clutter_actor_set_y(ac_scrollbox, scrollbar_start * geom.height + UIT_MARGIN);
}


void marmelade_filelist_refresh()
{
	int		bpm		= 140,
			count, i;
	char	*tone	= "e",
			**titles = NULL,
			**filenames = NULL;
	ClutterActor	*box, *text;
	ClutterGeometry	geom;

	marmelade_filelist_free();

	if ( na_db_get_filename_by_bpm_and_tone(bpm, tone, &count, &titles, &filenames) < 0 )
	{
		l_errorf("Unable to get filelist !");
		return;
	}

	if ( count <= 0 )
	{
		l_printf("No file found");
		return;
	}

	files = malloc(sizeof(ac_file_entry_t) * count);
	if ( files == NULL )
	{
		l_errorf("Unable to malloc file entries");
		return;
	}
	bzero(files, sizeof(ac_file_entry_t));
	filescount = count;

	for ( i = 0; i < count; i ++ )
	{
		files[i].title		= titles[i];
		files[i].filename	= filenames[i];
		files[i].actor		= clutter_group_new();

		clutter_actor_get_geometry(ca_filelist, &geom);

		text = clutter_label_new_with_text("Lucida 12", files[i].title);
		clutter_label_set_color(text, &file_fg);
		clutter_actor_set_position(text, UI_MARGIN, UI_MARGIN);
		clutter_label_set_line_wrap(text, TRUE);
		clutter_label_set_line_wrap_mode(text, PANGO_WRAP_WORD);
		clutter_actor_set_width(text, geom.width - (UIT_MARGIN * 2));

		box = clutter_round_rectangle_new_with_color(&filelist_bg);
		clutter_actor_set_size(box, geom.width - (UIT_MARGIN * 2), clutter_actor_get_height(text) + UIT_MARGIN);
		clutter_round_rectangle_set_border_color(box, &filelist_bc);

		clutter_container_add_actor(CLUTTER_CONTAINER(files[i].actor), box);
		clutter_container_add_actor(CLUTTER_CONTAINER(files[i].actor), text);

		clutter_actor_hide(files[i].actor);
		clutter_container_add_actor(CLUTTER_CONTAINER(ca_filelist), files[i].actor);
	}

	marmelade_filelist_refresh_scroll();
}

void marmelade_ui_refresh()
{
	clutter_actor_set_geometry_from(cr_scrollbars, ca_scrollbars, UI_MARGIN, UI_MARGIN);
	clutter_actor_set_geometry_from(cr_filelist, ca_filelist, UI_MARGIN, UI_MARGIN);
	clutter_actor_set_geometry_from(cr_menu, ca_menu, UI_MARGIN, UI_MARGIN);
	clutter_actor_set_geometry_from(cr_playground, ca_playground, UI_MARGIN, UI_MARGIN);
	clutter_actor_set_geometry_from(cr_actions, ca_actions, UI_MARGIN, UI_MARGIN);
}

void marmelade_ui_scrollbars()
{
	ClutterActor	*rect;
	rect = clutter_round_rectangle_new_with_color(&scrollbars_bg);
	clutter_round_rectangle_set_border_color(CLUTTER_ROUND_RECTANGLE(rect), &scrollbars_bc);
	clutter_actor_set_geometry_from(rect, ca_scrollbars, UI_MARGIN, UI_MARGIN);
	clutter_container_add_actor(CLUTTER_CONTAINER(ca_scrollbars), rect);
	cr_scrollbars = rect;

	ac_scrollbox = clutter_round_rectangle_new_with_color(&scrollbars_bg);
	clutter_round_rectangle_set_border_color(CLUTTER_ROUND_RECTANGLE(ac_scrollbox), &scrollbars_bc);
	clutter_actor_set_size(ac_scrollbox, 30 - UIT_MARGIN * 2, 30 - UIT_MARGIN * 2);
	clutter_actor_set_position(ac_scrollbox, UIT_MARGIN, UIT_MARGIN);
	clutter_container_add_actor(CLUTTER_CONTAINER(ca_scrollbars), ac_scrollbox);
}

void marmelade_ui_filelist()
{
	ClutterActor	*rect;

	rect = clutter_round_rectangle_new_with_color(&filelist_bg);
	clutter_round_rectangle_set_border_color(CLUTTER_ROUND_RECTANGLE(rect), &filelist_bc);
	clutter_round_rectangle_set_border_width(CLUTTER_ROUND_RECTANGLE(rect), 2);
	clutter_actor_set_geometry_from(rect, ca_filelist, UI_MARGIN, UI_MARGIN);
	clutter_container_add_actor(CLUTTER_CONTAINER(ca_filelist), rect);
	cr_filelist = rect;
}

void marmelade_ui_playground()
{
	ClutterActor	*rect;

	rect = clutter_round_rectangle_new_with_color(&playground_bg);
	clutter_round_rectangle_set_border_color(CLUTTER_ROUND_RECTANGLE(rect), &playground_bc);
	clutter_round_rectangle_set_border_width(CLUTTER_ROUND_RECTANGLE(rect), 2);
	clutter_actor_set_geometry_from(rect, ca_playground, UI_MARGIN, UI_MARGIN);
	clutter_container_add_actor(CLUTTER_CONTAINER(ca_playground), rect);
	cr_playground = rect;
}

void marmelade_ui_actions()
{
	ClutterActor	*rect;
	ClutterGeometry	geom;

	rect = clutter_round_rectangle_new_with_color(&transitions_bg);
	clutter_round_rectangle_set_border_color(CLUTTER_ROUND_RECTANGLE(rect), &transitions_bc);
	clutter_round_rectangle_set_border_width(CLUTTER_ROUND_RECTANGLE(rect), 2);
	clutter_actor_set_geometry_from(rect, ca_actions, UI_MARGIN, UI_MARGIN);
	clutter_container_add_actor(CLUTTER_CONTAINER(ca_actions), rect);
	cr_actions = rect;

	clutter_actor_get_geometry(ca_actions, &geom);

	geom.width	-= UIT_MARGIN * 2;
	geom.height	= geom.width;
	geom.x		= UIT_MARGIN;
	geom.y		= UIT_MARGIN;

	rect = clutter_round_rectangle_new_with_color(&transitions_bg);
	clutter_round_rectangle_set_border_color(CLUTTER_ROUND_RECTANGLE(rect), &transitions_bc);
	clutter_round_rectangle_set_border_width(CLUTTER_ROUND_RECTANGLE(rect), 2);
	clutter_actor_set_geometry(rect, &geom);
	clutter_container_add_actor(CLUTTER_CONTAINER(ca_actions), rect);
	ca_actions_stop = rect;

	geom.y		+= geom.height;

	rect = clutter_round_rectangle_new_with_color(&transitions_bg);
	clutter_round_rectangle_set_border_color(CLUTTER_ROUND_RECTANGLE(rect), &transitions_bc);
	clutter_round_rectangle_set_border_width(CLUTTER_ROUND_RECTANGLE(rect), 2);
	clutter_actor_set_geometry(rect, &geom);
	clutter_container_add_actor(CLUTTER_CONTAINER(ca_actions), rect);
	ca_actions_delete = rect;
}

void marmelade_ui_menu()
{
	ClutterActor	*rect;

	rect = clutter_round_rectangle_new_with_color(&menu_bg);
	clutter_round_rectangle_set_border_color(CLUTTER_ROUND_RECTANGLE(rect), &menu_bc);
	clutter_round_rectangle_set_border_width(CLUTTER_ROUND_RECTANGLE(rect), 2);
	clutter_actor_set_geometry_from(rect, ca_menu, UI_MARGIN, UI_MARGIN);
	clutter_container_add_actor(CLUTTER_CONTAINER(ca_menu), rect);
	cr_menu = rect;
}

static void marmelade_window_handle(unsigned short type, void *userdata, void *data)
{
	ClutterActor	*stage;
	uint			wx, wy;

	stage = clutter_stage_get_default();
	clutter_actor_get_size(stage, &wx, &wy);

	clutter_actor_set_position(ca_ui, 0, 0);
	clutter_actor_set_size(ca_ui, wx, wy);

	/* Set size
	 */
	clutter_actor_set_size(ca_menu, 300, 50);
	clutter_actor_set_position(ca_scrollbars, 0, 50);
	clutter_actor_set_size(ca_scrollbars, 30, wy - 50);
	clutter_actor_set_position(ca_filelist, 30, 50);
	clutter_actor_set_size(ca_filelist, 300 - 30, wy - 50);

	clutter_actor_set_position(ca_playground, 300, 0);
	clutter_actor_set_size(ca_playground, wx - 100 - 300, wy);

	clutter_actor_set_position(ca_actions, wx - 100, 0);
	clutter_actor_set_size(ca_actions, 100, wy);

	marmelade_ui_refresh();
	marmelade_filelist_refresh_scroll();
}


int context_marmelade_activate(void *ctx, void *userdata)
{
	ClutterActor	*stage;

	stage = clutter_stage_get_default();
	clutter_stage_set_color(CLUTTER_STAGE(stage), &stage_color);

	ca_ui			= clutter_group_new();
	ca_menu			= clutter_group_new();
	ca_scrollbars	= clutter_group_new();
	ca_filelist		= clutter_group_new();
	ca_actions		= clutter_group_new();
	ca_playground	= clutter_group_new();

	clutter_group_add_many(CLUTTER_GROUP(ca_ui),
		ca_menu, ca_scrollbars,
		ca_filelist, ca_actions,
		ca_playground, NULL
	);

	clutter_container_add_actor(CLUTTER_CONTAINER(stage), ca_ui);
	marmelade_window_handle(0, NULL, NULL);

	/* colors
	 */
	marmelade_ui_scrollbars();
	marmelade_ui_menu();
	marmelade_ui_filelist();
	marmelade_ui_actions();
	marmelade_ui_playground();

	/* files
	 */
	marmelade_filelist_refresh();

	/* events
	 */
	na_event_observe(NA_EV_CURSOR_NEW, marmelade_cursor_handle, NULL);
	na_event_observe(NA_EV_CURSOR_SET, marmelade_cursor_handle, NULL);
	na_event_observe(NA_EV_CURSOR_DEL, marmelade_cursor_handle, NULL);
//	na_event_observe(NA_EV_BUTTONPRESS, marmelade_cursor_handle, NULL);
	na_event_observe(NA_EV_WINDOW_UPDATE, marmelade_window_handle, NULL);

	return 0;
}

int context_marmelade_deactivate(void *ctx, void *userdata)
{
	na_event_remove(NA_EV_CURSOR_NEW, marmelade_cursor_handle, NULL);
	na_event_remove(NA_EV_CURSOR_SET, marmelade_cursor_handle, NULL);
	na_event_remove(NA_EV_CURSOR_DEL, marmelade_cursor_handle, NULL);
//	na_event_remove(NA_EV_BUTTONPRESS, marmelade_cursor_handle, NULL);
	na_event_remove(NA_EV_WINDOW_UPDATE, marmelade_window_handle, NULL);
	return 0;
}

int context_marmelade_update(void *ctx, void *userdata)
{
	return 1000;
}

void context_marmelade_register()
{
	bzero(&s_context, sizeof(s_context));

	strncpy(s_context.name, "marmelade", sizeof(s_context.name));
	s_context.fn_activate	= context_marmelade_activate;
	s_context.fn_deactivate	= context_marmelade_deactivate;
	s_context.fn_update		= context_marmelade_update;

	MUTEX_LOCK(context);
	na_ctx_register(&s_context);
	MUTEX_UNLOCK(context);
}
