#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/queue.h>
#include <assert.h>

#include "noya.h"
#include "context.h"

LOG_DECLARE("CTX");

MUTEX_DECLARE(context);

typedef struct
{
	na_ctx_t	*lh_first;
} na_ctx_list_t;
na_ctx_list_t	s_ctx			= {0};
na_ctx_t		*s_ctx_current	= NULL,
				*s_ctx_next		= NULL;

void na_ctx_register(na_ctx_t *ctx)
{
	MUTEX_LOCK(context);
	LIST_INSERT_HEAD(&s_ctx, ctx, next);
	MUTEX_UNLOCK(context);
}

na_ctx_t *na_ctx_resolve(const char *name)
{
	na_ctx_t *ctx;

	MUTEX_LOCK(context);
	LIST_FOREACH(ctx, &s_ctx, next)
	{
		if ( strncmp(ctx->name, name, sizeof(ctx->name)) == 0 )
		{
			MUTEX_UNLOCK(context);
			return ctx;
		}
	}
	MUTEX_UNLOCK(context);

	return NULL;
}

na_ctx_t *na_ctx_current(void)
{
	return s_ctx_current;
}

void na_ctx_switch(na_ctx_t *to)
{
	MUTEX_LOCK(context);
	s_ctx_next = to;
	MUTEX_UNLOCK(context);
}

int na_ctx_manager_switch()
{
	na_ctx_t	*old;

	MUTEX_LOCK(context);

switch_retry:;

	old = s_ctx_current;

	if ( s_ctx_next == NULL || s_ctx_next == s_ctx_current )
	{
		MUTEX_UNLOCK(context);
		return 0;
	}

	l_printf("switch to %s", s_ctx_next->name);

	if ( s_ctx_current )
	{
		if ( (s_ctx_current->fn_deactivate)(s_ctx_current, s_ctx_current->userdata) < 0 )
		{
			l_errorf("unable to deactivate context %s", s_ctx_current->name);
			MUTEX_UNLOCK(context);
			return -1;
		}

		s_ctx_current = NULL;
	}

	s_ctx_current = s_ctx_next;
	s_ctx_next = NULL;

	if ( s_ctx_current )
	{
		if ( (s_ctx_current->fn_activate)(s_ctx_current, s_ctx_current->userdata) < 0 )
		{
			l_errorf("unable to activate context %s", s_ctx_current->name);
			s_ctx_current = NULL;

			if ( old )
			{
				l_errorf("back to old context");
				s_ctx_next = old;
				old = NULL;
				goto switch_retry;
			}

			MUTEX_UNLOCK(context);
			return -1;
		}
	}

	MUTEX_UNLOCK(context);

	return 0;
}

int na_ctx_manager_update(void)
{
	if ( s_ctx_current == NULL )
		return 0;
	if ( s_ctx_current->fn_update == NULL )
		return 0;
	return (s_ctx_current->fn_update)(s_ctx_current, s_ctx_current->userdata);
}
