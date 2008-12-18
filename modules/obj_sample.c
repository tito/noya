#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <fftw3.h>

#include "noya.h"
#include "audio.h"
#include "module.h"
#include "config.h"
#include "utils.h"
#include "thread_manager.h"
#include "event.h"

#include "actors/clutter-circle.h"

#define MODULE_NAME "obj_sample"
#define BAR_COUNT	24

LOG_DECLARE("MOD_OBJ_SAMPLE");

static char *mod_settings[] =
{
	"file",
	"bargraph",
	NULL
};

typedef struct
{
	na_scene_t			*scene;
	manager_actor_t		*actor;

#define OBJ_FL_HIDE_BARGRAPH		0x01
	unsigned int		flags;

	fftwf_plan			fftpl;

	/* rendering issues
	 */
	ClutterActor	*group_cube;
	ClutterActor	*circle_volume;
	ClutterActor	*circle_position;
	ClutterActor	*bars[BAR_COUNT];

	/* audio attached for sample
	 */
	char			*filename;		/*< audio filename */
	na_audio_t		*audio;			/*< audio entry (for audio manager) */

	/* event observers
	 */
	na_event_list_t	observers;

} obj_t;

obj_t			def_obj				= {0};
static float	*bargraph_data		= NULL;
static uint		*bargraph_datasize	= 0;

#if 0
static void *object_resolve_value(obj_t *obj, char *value)
{
	assert( obj != NULL );
	assert( value != NULL );

	if ( strcmp(value, "volume") == 0 )
		return &obj->audio->volume;
	if ( strcmp(value, "position") == 0 )
		return &obj->audio->position;
	return NULL;
}
#endif

void lib_init(char **name, int *type, char ***settings)
{
	*name = strdup(MODULE_NAME);
	*type = NA_MOD_OBJECT | NA_MOD_EVENT;
	*settings = mod_settings;
}

void lib_exit(void)
{
	if ( bargraph_data != NULL )
		free(bargraph_data), bargraph_data = NULL;
	bargraph_datasize = 0;
}

obj_t *lib_object_new(na_scene_t *scene)
{
	obj_t *obj;

	obj = malloc(sizeof(obj_t));
	if ( obj == NULL )
		return NULL;
	bzero(obj, sizeof(obj_t));

	obj->scene = scene;

	na_event_init_ex(&obj->observers);

	return obj;
}

void lib_object_free(obj_t *obj)
{
	assert( obj != NULL );

	na_event_free_ex(&obj->observers);

	if ( obj->filename != NULL )
		free(obj->filename);
	if ( obj->audio != NULL )
		na_audio_free(obj->audio);

	if ( obj->fftpl )
		fftwf_destroy_plan(obj->fftpl);

	free(obj);
}

void lib_object_config(obj_t *obj, char *key, config_setting_t *value)
{
	const char	 *s_value;

	if ( strcmp(key, "file") == 0 )
	{
		s_value = config_setting_get_string(value);
		obj->filename = strdup(s_value);
		obj->audio = na_audio_load(s_value);
	}
	else if ( strcmp(key, "bargraph") == 0 )
	{
		s_value = config_setting_get_string(value);
		if ( strcmp(s_value, "hide") == 0 )
			obj->flags |= OBJ_FL_HIDE_BARGRAPH;
	}
	else
		l_printf("Invalid configuration %s", key);
}

void lib_object_prepare(obj_t *obj, manager_actor_t *actor)
{
	ClutterColor	obj_background,
					obj_border;
	ClutterActor	*ac;
	char			number[5];
	guint			radius;
	int				i, d;

	assert( obj != NULL );
	assert( actor != NULL );

	/* save actor
	 */
	obj->actor = actor;

	/* send events
	 */
	na_event_send_ex(&obj->observers, NA_EV_ACTOR_PREPARE, obj->actor, sizeof(manager_actor_t));
	na_event_send_ex(&obj->observers, NA_EV_AUDIO_PLAY, obj->audio, sizeof(na_audio_t));

	/* we want to be played !
	 */
	na_audio_wantplay(obj->audio);
	na_audio_set_loop(obj->audio, actor->scene_actor->is_loop);

	/* RENDERING !
	 */

	obj->group_cube = clutter_group_new();
	clutter_container_add_actor(CLUTTER_CONTAINER(actor->rotate_actor), obj->group_cube);

	/* create object
	 */
	radius = actor->scene_actor->width >> 1;

	memcpy(&obj_background, &actor->scene_actor->background_color, sizeof(obj_background));
	memcpy(&obj_border, &actor->scene_actor->border_color, sizeof(obj_border));
	ac = clutter_circle_new_with_color(&obj_background);
	clutter_circle_set_angle_stop(CLUTTER_CIRCLE(ac), 360);
	clutter_circle_set_border_color((ClutterRectangle *)ac, &obj_border);
//	clutter_rectangle_set_border_width((ClutterRectangle *)ac, actor->scene_actor->border_width);
	clutter_actor_set_width(ac, actor->scene_actor->width);
	clutter_actor_set_height(ac, actor->scene_actor->height);
	clutter_circle_set_radius(CLUTTER_CIRCLE(ac), radius);
	clutter_container_add_actor(CLUTTER_CONTAINER(obj->group_cube), ac);

	ac = clutter_circle_new_with_color(&obj_border);
	clutter_actor_set_width(ac, actor->scene_actor->width);
	clutter_actor_set_height(ac, actor->scene_actor->height);
	clutter_circle_set_radius(CLUTTER_CIRCLE(ac), radius);
	clutter_circle_set_width(CLUTTER_CIRCLE(ac), 10);
	clutter_container_add_actor(CLUTTER_CONTAINER(actor->clutter_actor), ac);
	obj->circle_position = ac;

	ac = clutter_circle_new_with_color(&obj_border);
	clutter_actor_set_width(ac, actor->scene_actor->width);
	clutter_actor_set_height(ac, actor->scene_actor->height);
	clutter_circle_set_radius(CLUTTER_CIRCLE(ac), radius + 10);
	clutter_circle_set_width(CLUTTER_CIRCLE(ac), 10);
	clutter_container_add_actor(CLUTTER_CONTAINER(actor->clutter_actor), ac);
	obj->circle_volume = ac;

	if ( ! (obj->flags & OBJ_FL_HIDE_BARGRAPH) )
	{
		for ( i = 0; i < BAR_COUNT; i++ )
		{
			ac = clutter_circle_new_with_color(&obj_border);
			clutter_actor_set_width(ac, actor->scene_actor->width);
			clutter_actor_set_height(ac, actor->scene_actor->height);
			clutter_circle_set_radius(CLUTTER_CIRCLE(ac), radius);
			clutter_circle_set_width(CLUTTER_CIRCLE(ac), 10);
			d = 360 / BAR_COUNT;
			clutter_circle_set_angle_start(CLUTTER_CIRCLE(ac), i * d);
			clutter_circle_set_angle_stop(CLUTTER_CIRCLE(ac), i * d + d);
			clutter_container_add_actor(CLUTTER_CONTAINER(actor->clutter_actor), ac);
			obj->bars[i] = ac;
		}
	}

	/* create text
	 */
	snprintf(number, sizeof(number), "%d", actor->tuio->f_id);
	ac = clutter_label_new_with_text("Lucida 12", number);
	clutter_actor_set_position(ac, actor->scene_actor->width / 2 -6, actor->scene_actor->height / 2 -10);

	clutter_container_add_actor(CLUTTER_CONTAINER(obj->group_cube), ac);
}

void lib_object_unprepare(obj_t *obj)
{
	/* send events
	 */
	na_event_send_ex(&obj->observers, NA_EV_AUDIO_STOP, obj->audio, sizeof(na_audio_t));
	na_event_send_ex(&obj->observers, NA_EV_ACTOR_UNPREPARE, obj->actor, sizeof(manager_actor_t));

	/* stop audio
	 */
	na_audio_stop(obj->audio);
	na_audio_seek(obj->audio, 0);

	/* never remove object from clutter
	 * parent remove all of this tree
	 */
	obj->actor		= NULL;
	obj->group_cube	= NULL;
}

void lib_object_update(obj_t *obj)
{
	assert( obj != NULL );

	clutter_circle_set_angle_stop(
		CLUTTER_CIRCLE(obj->circle_volume),
		(uint) (obj->audio->volume * 360.0f)
	);

	clutter_circle_set_angle_stop(
		CLUTTER_CIRCLE(obj->circle_position),
		(uint) (obj->audio->position * 360.0f)
	);

	if ( ! (obj->flags & OBJ_FL_HIDE_BARGRAPH) )
	{
		/* bar graph
		 */
		if ( obj->audio->input != NULL && obj->audio->input->data != NULL )
		{
			static float			v, *tmp;
			static int				n, i;

			n = obj->audio->input->size;

			/* allocate memory
			 * reajust if we got big sound.
			 */
			if ( bargraph_datasize != n )
			{
				tmp		= realloc(bargraph_data, sizeof(float) * n);
				if ( tmp == NULL )
					return;
				bargraph_data		= tmp;
				bargraph_datasize	= n;
			}

			/* execute fast fourier plan
			 */
			if ( obj->fftpl == NULL )
			{
				obj->fftpl	= fftwf_plan_r2r_1d(n,
					obj->audio->input->data, bargraph_data,
					FFTW_R2HC, FFTW_FORWARD | FFTW_PRESERVE_INPUT
				);
			}

			fftwf_execute(obj->fftpl);

			/* and ajust bargraph !
			 */
			for ( i = 0; i < n && i < BAR_COUNT; i++ )
			{
				v = sqrtf(bargraph_data[i] * bargraph_data[i]) * 30;
				if ( v > 50 )
					v = 50;
				clutter_circle_set_width(CLUTTER_CIRCLE(obj->bars[i]), v);
			}
		}
	}
}

void ctl_volume(void *data, float value)
{
	obj_t *obj = (obj_t *)data;

	assert( obj != NULL );

	na_audio_set_volume(obj->audio, value);
}

_fn_control lib_object_get_control(char *name)
{
	if ( strcmp(name, "volume") == 0 )
		return ctl_volume;
	return NULL;
}

void lib_event_observe(obj_t *obj, ushort ev_type, na_event_callback callback, void *userdata)
{
	l_printf("Add observer %d", ev_type);
	na_event_observe_ex(&obj->observers, ev_type, callback, userdata);

	if ( ev_type == NA_EV_ACTOR_PREPARE )
		na_event_send_ex(&obj->observers, NA_EV_ACTOR_PREPARE, obj->actor, sizeof(manager_actor_t));
	else if ( ev_type == NA_EV_AUDIO_PLAY )
		na_event_send_ex(&obj->observers, NA_EV_AUDIO_PLAY, obj->audio, sizeof(na_audio_t));
}

void lib_event_remove(obj_t *obj, ushort ev_type, na_event_callback callback, void *userdata)
{
	l_printf("Remove observer %d", ev_type);
	na_event_remove_ex(&obj->observers, ev_type, callback, userdata);
}
