#ifndef __MODULE_H
#define __MODULE_H

#include "event.h"

#define NA_MOD_OBJECT			0x0001			/* provide object interface */
#define NA_MOD_WIDGET			0x0002			/* provide widget interface */
#define NA_MOD_EVENT			0x0004			/* provide event interface */

typedef void (*_fn_control)(void*, float);

typedef struct na_module_s
{
	char	*name;
	int		type;
	void	*dl_handle;

	/* init lib
	 * arg: return name, return type
	 */
	void (*init)(void **, void *);

	/* exit lib
	 */
	void (*exit)(void);

	/* object
	 */

	/* global object config
	 * args: key, value
	 */
	void (*object_global_config)(void *, void *);

	/* create object on actor
	 * args: scene
	 * return: object data
	 */
	void *(*object_new)(void *);

	/* free object
	 * args: object data
	 */
	void (*object_free)(void *);

	/* configure object
	 * args: object data, key, value
	 */
	void (*object_config)(void *, void *, void *);

	/* prepare render
	 * args: object data, manager actor
	 */
	void (*object_prepare)(void *, void *);

	/* unprepare render
	 * args: object data
	 */
	void (*object_unprepare)(void *);

	/* update
	 * args: object data
	 */
	void (*object_update)(void *);

	/* controls
	 * args: name
	 */
	void *(*object_get_control)(void *);


	/* widget
	 */

	/* create widget on actor (give position)
	 * args: scene
	 * return: widget data
	 */
	void *(*widget_new)(void *);

	/* clone widget (used for default configuration)
	 * args: widget data, scene
	 */
	void *(*widget_clone)(void *, void *);

	/* detach widget
	 * args: widget data
	 */
	void (*widget_free)(void *);

	/* configure widget
	 * args: widget data, key, value
	 */
	void (*widget_config)(void *, void *, void *);

	/* prepare rendering (create object..)
	 * args: widget data, manager actor
	 */
	void (*widget_prepare)(void *, void *);

	/* unprepare rendering (delete object..)
	 * args: widget data
	 */
	void (*widget_unprepare)(void *);

	/* update phase (in rendering)
	 * args: widget data
	 */
	void (*widget_update)(void *);

	/* set data 
	 * args: widget data, data
	 */
	void (*widget_set_data)(void *, void *);


	/* control
	 */

	/* update a control
	 * args: manager actor, value of control
	 */
	void (*ctl_update)(void *, float);


	/* event
	 */

	/* observe an event
	 * args: module data, type of event, callback, userdata
	 */
	void (*event_observe)(void *, ushort, na_event_callback, void *);

	/* unobserve an event
	 * args: module data, type of event, callback
	 */
	void (*event_remove)(void *, ushort, na_event_callback, void *);

	LIST_ENTRY(na_module_s) next;
} na_module_t;

typedef struct
{
	na_module_t *lh_first;
} na_module_head_t;

void		na_modules_init();
void		na_modules_free();
na_module_t	*na_module_get(char *name, int type);

#endif
