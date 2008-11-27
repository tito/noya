#include <string.h>
#include "noya.h"
#include "context.h"

static na_ctx_t		s_context;

int context_menu_activate(void *ctx, void *userdata)
{
	return 0;
}

int context_menu_deactivate(void *ctx, void *userdata)
{
	return 0;
}

int context_menu_update(void *ctx, void *userdata)
{
	return 0;
}

void context_menu_register()
{
	bzero(&s_context, sizeof(s_context));

	strncpy(s_context.name, "menu", sizeof(s_context.name));
	s_context.fn_activate	= context_menu_activate;
	s_context.fn_deactivate	= context_menu_deactivate;
	s_context.fn_update		= context_menu_update;

	na_ctx_register(&s_context);
}
