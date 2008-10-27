#ifndef __SCENE_H
#define __SCENE_H

#include <sys/queue.h>
#include "audio.h"
#include "utils.h"
#include "module.h"

typedef struct na_scene_actor_base_s
{
	int			id;							/*< id of object */

	int			width;						/*< width */
	int			height;						/*< height */
	na_color_t	background_color;			/*< background color */
	na_color_t	border_color;				/*< border color */
	int			border_width;				/*< border width */

	short		is_loop;					/*< indicate if actor animation is looping */
	
	_fn_control ctl_angle;					/*< indicate control for angle */
	_fn_control ctl_x;						/*< indicate control for X axis */
	_fn_control ctl_y;						/*< indicate control for Y axis */

	na_module_t	*mod;						/*< module to extend base object */
	void		*data_mod;					/*< private data for module */

	LIST_ENTRY(na_scene_actor_base_s) next;

} na_scene_actor_base_t;

typedef struct na_scene_actor_head_s
{
	na_scene_actor_base_t	*lh_first;
} na_scene_actor_list_t;

typedef struct
{
	char					*name;				/*< name of scene */
	na_color_t				background_color;	/*< scene background color */
	na_scene_actor_base_t	def_actor;			/*< default params for actor */
	na_scene_actor_list_t	actors;				/*< actors list */

	/* general scene config
	 */
	short	precision;					/*< precision in bpm */
	short	bpm;						/*< bpm ! */
	short	measure;					/*< bpm per measure */

} na_scene_t;

typedef struct
{
	uint	beat;
	uint	measure;
	ushort	beatinmeasure;
} na_bpm_t;

na_scene_t	*na_scene_load(char *filename);
void	na_scene_free(na_scene_t *scene);
na_scene_actor_base_t *na_scene_actor_get(na_scene_t *scene, int idx);
na_scene_actor_base_t *na_scene_actor_new(na_scene_t *scene, int idx);
void	na_scene_actor_free(na_scene_actor_base_t *actor);
void	na_scene_prop_set(na_scene_t *scene, na_scene_actor_base_t *actor, char *key, char *value);

#endif
