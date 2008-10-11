#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>

#include "noya.h"
#include "event.h"

LOG_DECLARE("EVENT")

static na_event_list_t	na_event_list;

void na_event_init(void)
{
	LIST_INIT(&na_event_list);
}

void na_event_free(void)
{
	na_event_t *event;
	while ( !LIST_EMPTY(&na_event_list) )
	{
		event = LIST_FIRST(&na_event_list);
		LIST_REMOVE(event, next);
		free(event);
	}
}

void na_event_observe(unsigned short ev_type, na_event_callback callback, void *userdata)
{
	na_event_t *event;

	event = malloc(sizeof(na_event_t));
	if ( event == NULL )
	{
		l_errorf("unable to create event observer !");
		return;
	}

	event->type		= ev_type;
	event->callback	= callback;
	event->userdata	= userdata;

	LIST_INSERT_HEAD(&na_event_list, event, next);
}

void na_event_send(unsigned short ev_type, void *data)
{
	na_event_t	*event;

	for ( event = na_event_list.lh_first; event != NULL; event = event->next.le_next )
	{
		if ( event->type == ev_type )
			(*event->callback)(ev_type, event->userdata, data);
	}
}

void na_event_remove(unsigned short ev_type, na_event_callback callback)
{
	na_event_t	*event;

	for ( event = na_event_list.lh_first; event != NULL; event = event->next.le_next )
	{
		if ( event->type == ev_type && event->callback == callback )
		{
			LIST_REMOVE(event, next);
			return;
		}
	}
}


