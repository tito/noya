#ifndef __CTX_H
#define __CTX_H

#include <unistd.h>
#include <sys/queue.h>
#include "noya.h"

typedef int (*na_ctx_fn)(void *ctx, void *userdata);

typedef struct na_ctx_s
{
#define NA_CTX_FL_LOADED	0x0001
#define NA_CTX_FL_CURRENT	0x0002
	na_atomic_t	flags;

	char		name[64];

	na_ctx_fn	fn_activate;
	na_ctx_fn	fn_deactivate;
	na_ctx_fn	fn_update;
	void		*userdata;

	LIST_ENTRY(na_ctx_s) next;
} na_ctx_t;

void		na_ctx_register(na_ctx_t *ctx);
na_ctx_t	*na_ctx_resolve(const char *name);
void		na_ctx_switch(na_ctx_t *to);
na_ctx_t	*na_ctx_current(void);
int			na_ctx_manager_switch(void);
int			na_ctx_manager_update(void);

#endif
