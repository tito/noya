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

static module_head_t module_list;

static int module_filter(const struct dirent *dirent)
{
	unsigned int len;
	len = strlen (dirent->d_name);
	return (len > 3 && strcmp (".so", dirent->d_name + len - 3) == 0) ? 1 : 0;
}

void noya_modules_init()
{
	char			dl_name[1024];
	int				scandir_entries,
					libs_to_scan, i;
	char			*path,
					*name;
	struct dirent	**scandir_list;
	void			*dl_handle = NULL;
	module_t		*module = NULL;

	LIST_INIT(&module_list);

	l_printf("Load modules...");

	path = config_get(CONFIG_DEFAULT, "noya.path.modules");
	if ( path == NULL )
	{
		l_errorf("no noya.path.modules config set !");
		return;
	}

	scandir_entries = scandir(path,  &scandir_list, module_filter, alphasort);
	if ( scandir_entries <= 0 )
		return;

	for ( libs_to_scan = 0; libs_to_scan < scandir_entries; libs_to_scan++ )
	{
		name = scandir_list[libs_to_scan]->d_name;
		snprintf(dl_name, sizeof(dl_name), "%s/%s", path, name);

		l_printf("Load %s", dl_name);

		dl_handle = dlopen (dl_name, RTLD_LAZY);
		if ( dl_handle == NULL )
		{
			l_errorf("unable to load %s: %s", name, dlerror());
			goto module_init_failed;
		}

		module = malloc(sizeof(module_t));
		if ( module == NULL )
		{
			free(module);
			l_errorf("no enough memory to create module %s", name);
			goto module_init_failed;
		}

		bzero(module, sizeof(module_t));

		module->dl_handle = dl_handle;
		module->init = dlsym(dl_handle, "lib_init");
		module->exit = dlsym(dl_handle, "lib_exit");

		if ( module->init == NULL || module->exit == NULL )
		{
			l_errorf("Missing lib_[init|exit]() functions in %s", name);
			goto module_init_failed;
		}

		/* init module
		 */
		(*module->init)((void **)&module->name, (void *)&module->type);


		if ( module->type & MODULE_TYPE_OBJECT )
		{
			module->object_new = dlsym(dl_handle, "lib_object_new");
			module->object_free = dlsym(dl_handle, "lib_object_free");
			module->object_config = dlsym(dl_handle, "lib_object_config");
			module->object_global_config = dlsym(dl_handle, "lib_object_global_config");
			module->object_update = dlsym(dl_handle, "lib_object_update");
			module->object_prepare = dlsym(dl_handle, "lib_object_prepare");
			module->object_unprepare = dlsym(dl_handle, "lib_object_unprepare");
			module->object_render = dlsym(dl_handle, "lib_object_render");
			module->object_get_control = dlsym(dl_handle, "lib_object_get_control");
		}

		if ( module->type & MODULE_TYPE_WIDGET )
		{
			module->widget_new = dlsym(dl_handle, "lib_widget_new");
			module->widget_free = dlsym(dl_handle, "lib_widget_free");
			module->widget_clone = dlsym(dl_handle, "lib_widget_clone");
			module->widget_config = dlsym(dl_handle, "lib_widget_config");
			module->widget_update = dlsym(dl_handle, "lib_widget_update");
			module->widget_prepare = dlsym(dl_handle, "lib_widget_prepare");
			module->widget_unprepare = dlsym(dl_handle, "lib_widget_unprepare");
			module->widget_set_data = dlsym(dl_handle, "lib_widget_set_data");
		}

		LIST_INSERT_HEAD(&module_list, module, next);

		dl_handle = NULL;

module_init_failed:;
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

static void module_free(module_t *module)
{
	assert( module != NULL );
	assert( module->exit != NULL );

	(*module->exit)();

	dlclose(module->dl_handle);

	if ( module->name != NULL )
		free(module->name);
	free(module);
}

void noya_modules_free()
{
	module_t *module;

	while ( !LIST_EMPTY(&module_list) )
	{
		module = LIST_FIRST(&module_list);
		LIST_REMOVE(module, next);
		module_free(module);
	}
}

module_t *noya_module_get(char *name, int type)
{
	module_t *module;

	assert( name != NULL );

	for ( module = module_list.lh_first; module != NULL; module = module->next.le_next )
	{
		if ( !(module->type & type) )
			continue;
		if ( strcmp(module->name, name) == 0 )
			return module;
	}

	l_errorf("unable to found %s", name);

	return NULL;
}
