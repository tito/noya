#ifndef __THREAD_MANAGER_H
#define __THREAD_MANAGER_H

#include <clutter/clutter.h>
#include "scene.h"
#include "module.h"

/* list of object in scene
 */
typedef struct manager_actor_s
{
	/* idx of object (in tuio env)
	 */
	unsigned int		id;

	/* fiducial id
	 */
	unsigned int		fid;

	/* base actor from scene
	 */
	scene_actor_base_t	*scene_actor;

	/* rendering actor
	 */
	ClutterActor		*clutter_actor;
	ClutterActor		*rotate_actor;

	LIST_ENTRY(manager_actor_s) next;
} manager_actor_t;

typedef struct
{
	manager_actor_t *lh_first;
} manager_actor_list_t;

/* list of cursor in scene
 */
typedef struct manager_cursor_s
{
	unsigned int	id;
	ClutterActor	*actor;
	char			label[10];
	LIST_ENTRY(manager_cursor_s) next;
} manager_cursor_t;

typedef struct
{
	manager_cursor_t *lh_first;
} manager_cursor_list_t;

/* thread management
 */
int thread_manager_start(void);
int thread_manager_stop(void);

#endif
