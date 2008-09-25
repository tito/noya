#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>

#include "noya.h"
#include "event.h"

LOG_DECLARE("EVENT")

static event_list_t	event_list;

void noya_event_init(void)
{
	LIST_INIT(&event_list);
}

void noya_event_free(void)
{
	event_t *event;
	while ( !LIST_EMPTY(&event_list) )
	{
		event = LIST_FIRST(&event_list);
		LIST_REMOVE(event, next);
		free(event);
	}
}

void noya_event_observe(unsigned short ev_type, event_callback callback, void *userdata)
{
	event_t *event;

	event = malloc(sizeof(event_t));
	if ( event == NULL )
	{
		l_errorf("unable to create event observer !");
		return;
	}

	event->type		= ev_type;
	event->callback	= callback;
	event->userdata	= userdata;

	LIST_INSERT_HEAD(&event_list, event, next);
}

void noya_event_send(unsigned short ev_type, void *data)
{
	event_t	*event;

	for ( event = event_list.lh_first; event != NULL; event = event->next.le_next )
	{
		if ( event->type == ev_type )
			(*event->callback)(ev_type, event->userdata, data);
	}
}

void noya_event_remove(unsigned short ev_type, event_callback callback)
{
	event_t	*event;

	for ( event = event_list.lh_first; event != NULL; event = event->next.le_next )
	{
		if ( event->type == ev_type && event->callback == callback )
		{
			LIST_REMOVE(event, next);
			return;
		}
	}
}


