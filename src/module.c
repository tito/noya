#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <dlfcn.h>
#include <dirent.h>

#include "noya.h"
#include "module.h"

LOG_DECLARE("MODULE");

static na_module_head_t na_module_list;

static int na_module_filter(const struct dirent *dirent)
{
	unsigned int len;
	len = strlen (dirent->d_name);
	return (len > 3 && strcmp (".so", dirent->d_name + len - 3) == 0) ? 1 : 0;
}

void na_modules_init()
{
	char			dl_name[1024];
	int				scandir_entries,
					libs_to_scan, i;
	char			*path,
					*name;
	struct dirent	**scandir_list;
	void			*dl_handle = NULL;
	na_module_t		*module = NULL;

	LIST_INIT(&na_module_list);

	l_printf("Load modules...");

	path = na_config_get(NA_CONFIG_DEFAULT, "noya.path.modules");
	if ( path == NULL )
	{
		l_errorf("no noya.path.modules config set !");
		return;
	}

	scandir_entries = scandir(path,  &scandir_list, na_module_filter, alphasort);
	if ( scandir_entries <= 0 )
		return;

	for ( libs_to_scan = 0; libs_to_scan < scandir_entries; libs_to_scan++ )
	{
		name = scandir_list[libs_to_scan]->d_name;
		snprintf(dl_name, sizeof(dl_name), "%s/%s", path, name);

		l_printf("Load %s", dl_name);

		dl_handle = dlopen (dl_name, RTLD_NOW);
		if ( dl_handle == NULL )
		{
			l_errorf("unable to load %s: %s", name, dlerror());
			goto na_module_init_failed;
		}

		module = malloc(sizeof(na_module_t));
		if ( module == NULL )
		{
			free(module);
			l_errorf("no enough memory to create module %s", name);
			goto na_module_init_failed;
		}

		bzero(module, sizeof(na_module_t));

		module->dl_handle = dl_handle;
		module->init = dlsym(dl_handle, "lib_init");
		module->exit = dlsym(dl_handle, "lib_exit");

		if ( module->init == NULL || module->exit == NULL )
		{
			l_errorf("Missing lib_[init|exit]() functions in %s", name);
			goto na_module_init_failed;
		}

		/* init module
		 */
		(*module->init)((void **)&module->name, (void *)&module->type);

#define MODULE_LOAD_DEF(def) \
		module->def = dlsym(dl_handle, "lib_"#def);	\
		if ( module->def == NULL ) { \
			l_printf("Unable to load %s, symbol %s is missing", module->name, #def); \
			goto na_module_init_failed; \
		}

		if ( module->type & NA_MOD_OBJECT )
		{
			MODULE_LOAD_DEF(object_new);
			MODULE_LOAD_DEF(object_free);
			MODULE_LOAD_DEF(object_config);
			MODULE_LOAD_DEF(object_global_config);
			MODULE_LOAD_DEF(object_update);
			MODULE_LOAD_DEF(object_prepare);
			MODULE_LOAD_DEF(object_unprepare);
			MODULE_LOAD_DEF(object_get_control);
		}

		if ( module->type & NA_MOD_WIDGET )
		{
			MODULE_LOAD_DEF(widget_new);
			MODULE_LOAD_DEF(widget_free);
			MODULE_LOAD_DEF(widget_clone);
			MODULE_LOAD_DEF(widget_config);
			MODULE_LOAD_DEF(widget_update);
			MODULE_LOAD_DEF(widget_prepare);
			MODULE_LOAD_DEF(widget_unprepare);
			MODULE_LOAD_DEF(widget_set_data);
		}

		LIST_INSERT_HEAD(&na_module_list, module, next);

		dl_handle = NULL;

na_module_init_failed:;
		if ( dl_handle != NULL )
			dlclose(dl_handle);
		dl_handle = NULL;
		module = NULL;
	}

	if ( scandir_entries > 0 )
	{
		for ( i = 0; i < scandir_entries; i++ )
			free(scandir_list[i]);
		free(scandir_list);
	}
}

static void na_module_free(na_module_t *module)
{
	assert( module != NULL );
	assert( module->exit != NULL );

	(*module->exit)();

	dlclose(module->dl_handle);

	if ( module->name != NULL )
		free(module->name);
	free(module);
}

void na_modules_free()
{
	na_module_t *module;

	while ( !LIST_EMPTY(&na_module_list) )
	{
		module = LIST_FIRST(&na_module_list);
		LIST_REMOVE(module, next);
		na_module_free(module);
	}
}

na_module_t *na_module_get(char *name, int type)
{
	na_module_t *module;

	assert( name != NULL );

	for ( module = na_module_list.lh_first; module != NULL; module = module->next.le_next )
	{
		if ( !(module->type & type) )
			continue;
		if ( strcmp(module->name, name) == 0 )
			return module;
	}

	l_errorf("unable to found %s", name);

	return NULL;
}
