#ifndef __THREAD_MANAGER_H
#define __THREAD_MANAGER_H

#include <clutter/clutter.h>
#include "scene.h"
#include "module.h"
#include "thread_input.h"

/* list of object in scene
 */
typedef struct manager_actor_s
{
	/* idx of object (in tuio env)
	 */
	uint					id;

	/* fiducial id
	 */
	tuio_object_t			*tuio;

	/* base actor from scene
	 */
	na_scene_actor_t		*scene_actor;

	/* rendering actor
	 */
	ClutterActor			*clutter_actor;
	ClutterActor			*rotate_actor;

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
	uint	id;
	ClutterActor	*actor;
	char			label[10];
	LIST_ENTRY(manager_cursor_s) next;
} manager_cursor_t;

typedef struct
{
	manager_cursor_t *lh_first;
} manager_cursor_list_t;

manager_actor_list_t *na_manager_get_actors(void);
manager_actor_t *na_manager_actor_get_by_fid(uint fid);

/* thread management
 */
int thread_manager_start(void);
int thread_manager_stop(void);

#endif
