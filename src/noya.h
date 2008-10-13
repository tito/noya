#ifndef __NOYA_H
#define __NOYA_H

#include <pthread.h>
#include <signal.h>
#include "config.h"

/* Types
 */
#define NA_OK					0
#define NA_ERR					-1

#define THREAD_STATE_START		0
#define THREAD_STATE_RUNNING	1
#define THREAD_STATE_STOP		2
#define THREAD_STATE_RESTART	3

/* Macros
 */
#define MUTEX_DECLARE(a)		pthread_mutex_t	__mtx__##a = PTHREAD_MUTEX_INITIALIZER
#define MUTEX_IMPORT(a)			extern pthread_mutex_t __mtx__##a
#define MUTEX_LOCK(a)			pthread_mutex_lock(&__mtx__##a)
#define MUTEX_UNLOCK(a)			pthread_mutex_unlock(&__mtx__##a)
#define THREAD_GLOBAL_LOCK		MUTEX_LOCK(global)
#define THREAD_GLOBAL_UNLOCK	MUTEX_UNLOCK(global)
#define THREAD_ENTER			do { THREAD_GLOBAL_LOCK; c_running = 1; g_threads++; THREAD_GLOBAL_UNLOCK; } while(0);
#define THREAD_LEAVE			do { THREAD_GLOBAL_LOCK; c_running = 0; g_threads--; THREAD_GLOBAL_UNLOCK; } while(0);

/* Log
 */
#ifdef NA_DEBUG
#define LOG_DECLARE(a)			static char *log_module = a;
#define l_printf(...) do {						\
		THREAD_GLOBAL_LOCK;						\
		printf("[%-8s] ", log_module);			\
		printf(__VA_ARGS__);					\
		printf("\n");							\
		THREAD_GLOBAL_UNLOCK;					\
	} while(0);
#define l_errorf(...) do {						\
		THREAD_GLOBAL_LOCK;						\
		printf("[%-8s] \033[31m\033[1mERROR : ", log_module);			\
		printf(__VA_ARGS__);					\
		printf("\033[0m\n");						\
		THREAD_GLOBAL_UNLOCK;					\
	} while(0);
#else
#define l_printf(...)
#define l_errorw(...)
#endif

typedef volatile sig_atomic_t  na_atomic_t;

/* Options
 */
typedef struct
{
	char			*scene_fn;
	short			dump;
} na_options_t;
extern na_options_t g_options;

/* Globals
 */
extern na_atomic_t g_threads;
extern na_atomic_t g_want_leave;
MUTEX_IMPORT(global);

#endif
