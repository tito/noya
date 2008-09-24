#ifndef __NOYA_H
#define __NOYA_H

#include <pthread.h>
#include <signal.h>
#include "config.h"

/* Types
 */
#define NOYA_OK					0
#define NOYA_ERR				1

#define THREAD_STATE_START		0
#define THREAD_STATE_RUNNING	1
#define THREAD_STATE_STOP		2
#define THREAD_STATE_RESTART	3

/* Macros
 */
#define MUTEX_DECLARE(a)		pthread_mutex_t	a = PTHREAD_MUTEX_INITIALIZER
#define MUTEX_IMPORT(a)			extern pthread_mutex_t a
#define MUTEX_LOCK(a)			pthread_mutex_lock(&a)
#define MUTEX_UNLOCK(a)			pthread_mutex_unlock(&a)
#define THREAD_GLOBAL_LOCK		MUTEX_LOCK(g_thread_mutex)
#define THREAD_GLOBAL_UNLOCK	MUTEX_UNLOCK(g_thread_mutex)
#define THREAD_ENTER			do { THREAD_GLOBAL_LOCK; c_running = 1; g_threads++; THREAD_GLOBAL_UNLOCK; } while(0);
#define THREAD_LEAVE			do { THREAD_GLOBAL_LOCK; c_running = 0; g_threads--; THREAD_GLOBAL_UNLOCK; } while(0);

/* Log
 */
#ifdef NOYA_DEBUG
#define LOG_DECLARE(a)			static char *log_module = a;
#define l_printf(...) do {						\
		THREAD_GLOBAL_LOCK;						\
		printf("[%-8s] ", log_module);			\
		printf(__VA_ARGS__);					\
		printf("\n");							\
		THREAD_GLOBAL_UNLOCK;					\
	} while(0);
#else
#define l_printf(...)
#endif

/* Options
 */
typedef struct
{
	char			*scene_fn;
	short			dump;
} options_t;
extern options_t g_options;

/* Globals
 */
extern sig_atomic_t g_threads;
MUTEX_IMPORT(g_thread_mutex);

#endif
