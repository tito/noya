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

void na_event_init_ex(na_event_list_t *list)
{
	assert( list != NULL );

	LIST_INIT(list);
}

void na_event_free_ex(na_event_list_t *list)
{
	na_event_t *event;

	assert( list != NULL );

	while ( !LIST_EMPTY(list) )
	{
		event = LIST_FIRST(list);
		LIST_REMOVE(event, next);
		free(event);
	}
}

void na_event_observe_ex(na_event_list_t *list, ushort ev_type, na_event_callback callback, void *userdata)
{
	na_event_t *event;

	assert( list != NULL );

	event = malloc(sizeof(na_event_t));
	if ( event == NULL )
	{
		l_errorf("unable to create event observer !");
		return;
	}

	event->type		= ev_type;
	event->callback	= callback;
	event->userdata	= userdata;

	LIST_INSERT_HEAD(list, event, next);
}

void na_event_send_ex(na_event_list_t *list, ushort ev_type, void *data)
{
	na_event_t	*event;

	assert( list != NULL );

	for ( event = list->lh_first; event != NULL; event = event->next.le_next )
	{
		if ( event->type == ev_type )
			(*event->callback)(ev_type, event->userdata, data);
	}
}

void na_event_remove_ex(na_event_list_t *list, ushort ev_type, na_event_callback callback, void *userdata)
{
	na_event_t	*event;

	assert( list != NULL );

	for ( event = list->lh_first; event != NULL; event = event->next.le_next )
	{
		if ( event->type == ev_type &&
			event->callback == callback &&
			event->userdata == userdata )
		{
			LIST_REMOVE(event, next);
			return;
		}
	}
}


void na_event_init(void)
{
	na_event_init_ex(&na_event_list);
}

void na_event_free(void)
{
	na_event_free_ex(&na_event_list);
}

void na_event_observe(ushort ev_type, na_event_callback callback, void *userdata)
{
	na_event_observe_ex(&na_event_list, ev_type, callback, userdata);
}

void na_event_send(ushort ev_type, void *data)
{
	na_event_send_ex(&na_event_list, ev_type, data);
}

void na_event_remove(ushort ev_type, na_event_callback callback, void *userdata)
{
	na_event_remove_ex(&na_event_list, ev_type, callback, userdata);
}
