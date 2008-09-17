#ifndef __SCENE_MANAGER_H
#define __SCENE_MANAGER_H

#include <sys/queue.h>
#include "thread_audio.h"

/* access
 */
#define SCENE_ACTOR_BASE(a)			((scene_actor_base_t *)(a))
#define SCENE_ACTOR_SAMPLE(a)		((scene_actor_sample_t *)(a))

/* controls
 */
#define SCENE_CONTROL_NONE			0x00
#define SCENE_CONTROL_VOLUME		0x01

typedef char color_t[8];

typedef struct scene_actor_base_s
{
#define SCENE_ACTOR_TYPE_BASE		0x00
#define SCENE_ACTOR_TYPE_SAMPLE		0x01
	short	type;						/*< type of object */
	int		id;							/*< id of object */

	int		width;						/*< width */
	int		height;						/*< height */
	char	background_color[8];		/*< background color */
	char	border_color[8];			/*< border color */
	int		border_width;				/*< border width */

	short	is_loop;					/*< indicate if actor animation is looping */

	short	ctl_angle;					/*< indicate control for angle */
	short	ctl_x;						/*< indicate control for X axis */
	short	ctl_y;						/*< indicate control for Y axis */

	LIST_ENTRY(scene_actor_base_s) next;

} scene_actor_base_t;

typedef struct
{
	scene_actor_base_t	parent;			/*< extend */

	char				*filename;		/*< audio filename */
	audio_entry_t		*audio;			/*< audio entry (for audio manager) */
} scene_actor_sample_t;

typedef struct scene_actor_head_s
{
	scene_actor_base_t	*lh_first;
} scene_actor_head_t;

typedef struct
{
	char				*name;			/*< name of scene */
	char				background_color[8]; /*< scene background color */
	scene_actor_base_t	def_actor;		/*< default params for actor */
	scene_actor_head_t	actors;
} scene_t;

scene_t	*noya_scene_load(char *filename);
void	noya_scene_free(scene_t *scene);
scene_actor_base_t *noya_scene_actor_get(scene_t *scene, int idx);
scene_actor_base_t *noya_scene_actor_new(scene_t *scene, int idx);
void	noya_scene_actor_free(scene_actor_base_t *actor);
void	noya_scene_prop_set(scene_t *scene, scene_actor_base_t *actor, char *key, char *value);

#endif
