#ifndef __MODULE_H
#define __MODULE_H

#define MODULE_TYPE_OBJECT		0x01
#define MODULE_TYPE_WIDGET		0x02

typedef void (*_fn_control)(void*,float);

typedef struct module_s
{
	char	*name;
	int		type;

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

	/* render
	 * args: object data
	 */
	void (*object_render)(void *);

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

	LIST_ENTRY(module_s) next;
} module_t;

typedef struct
{
	module_t *lh_first;
} module_head_t;

void		noya_modules_init();
void		noya_modules_free();
module_t	*noya_module_get(char *name, int type);

#endif
