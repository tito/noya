#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "noya.h"
#include "thread_input.h"

MUTEX_DECLARE(m_input);
pthread_t	thread_input;
short		c_want_leave	= 0;
short		c_running		= 0;
short		c_state			= THREAD_STATE_START;

static void *thread_input_run(void *arg)
{
	THREAD_ENTER;

	while ( 1 )
	{
		switch ( c_state )
		{
			case THREAD_STATE_START:
				l_printf(" - INPUT started.");
				c_state = THREAD_STATE_RUNNING;
				break;

			case THREAD_STATE_RESTART:
			case THREAD_STATE_STOP:
				l_printf(" - INPUT stopped.");
				if ( c_state == THREAD_STATE_RESTART )
					c_state = THREAD_STATE_START;
				else
					goto thread_leave;
				break;

			case THREAD_STATE_RUNNING:
				MUTEX_LOCK(m_input);
				if ( c_want_leave )
				{
					MUTEX_UNLOCK(m_input);
					c_state = THREAD_STATE_STOP;
				}
				MUTEX_UNLOCK(m_input);
				break;
		}
	}

thread_leave:;
	THREAD_LEAVE;
}

int thread_input_start(void)
{
	int ret;

	ret = pthread_create(&thread_input, NULL, thread_input_run, NULL);
	if ( ret )
	{
		l_printf("Cannot create input thread");
		return NOYA_ERR;
	}

	return NOYA_OK;
}

int thread_input_stop(void)
{
	if ( !c_running )
		return NOYA_OK;

	MUTEX_LOCK(m_input);
	c_want_leave = 1;
	MUTEX_UNLOCK(m_input);

	return NOYA_OK;
}
