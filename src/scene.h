#ifndef __SCENE_H
#define __SCENE_H

#include <sys/queue.h>
#include "audio.h"
#include "module.h"

typedef char color_t[4];

typedef struct scene_actor_base_s
{
	int		id;							/*< id of object */

	int		width;						/*< width */
	int		height;						/*< height */
	color_t	background_color;			/*< background color */
	color_t	border_color;				/*< border color */
	int		border_width;				/*< border width */

	short	is_loop;					/*< indicate if actor animation is looping */
	
	_fn_control ctl_angle;				/*< indicate control for angle */
	_fn_control ctl_x;					/*< indicate control for X axis */
	_fn_control ctl_y;					/*< indicate control for Y axis */

	module_t	*mod;					/*< module to extend base object */
	void		*data_mod;				/*< private data for module */

	LIST_ENTRY(scene_actor_base_s) next;

} scene_actor_base_t;

typedef struct scene_actor_head_s
{
	scene_actor_base_t	*lh_first;
} scene_actor_list_t;

typedef struct
{
	char				*name;				/*< name of scene */
	color_t				background_color;	/*< scene background color */
	scene_actor_base_t	def_actor;			/*< default params for actor */
	scene_actor_list_t	actors;				/*< actors list */

	/* general scene config
	 */
	short	precision;					/*< precision in bpm */
	short	bpm;						/*< bpm ! */

} scene_t;

scene_t	*noya_scene_load(char *filename);
void	noya_scene_free(scene_t *scene);
scene_actor_base_t *noya_scene_actor_get(scene_t *scene, int idx);
scene_actor_base_t *noya_scene_actor_new(scene_t *scene, int idx);
void	noya_scene_actor_free(scene_actor_base_t *actor);
void	noya_scene_prop_set(scene_t *scene, scene_actor_base_t *actor, char *key, char *value);

#endif
