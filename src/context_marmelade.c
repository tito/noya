#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/queue.h>
#include <assert.h>
#include <clutter/clutter.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>

#include <string.h>
#include "noya.h"
#include "event.h"
#include "thread_manager.h"
#include "thread_input.h"
#include "thread_audio.h"
#include "scene.h"
#include "utils.h"
#include "context.h"
#include "rtc.h"

LOG_DECLARE("MARMELADE");
MUTEX_IMPORT(context);

static na_ctx_t		s_context;

int context_marmelade_activate(void *ctx, void *userdata)
{
	return 0;
}

int context_marmelade_deactivate(void *ctx, void *userdata)
{
	return 0;
}

int context_marmelade_update(void *ctx, void *userdata)
{
	return 0;
}

void context_marmelade_register()
{
	bzero(&s_context, sizeof(s_context));

	strncpy(s_context.name, "marmelade", sizeof(s_context.name));
	s_context.fn_activate	= context_marmelade_activate;
	s_context.fn_deactivate	= context_marmelade_deactivate;
	s_context.fn_update		= context_marmelade_update;

	MUTEX_LOCK(context);
	na_ctx_register(&s_context);
	MUTEX_UNLOCK(context);
}
