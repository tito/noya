#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>
#include <clutter/clutter.h>

#include "noya.h"
#include "event.h"

LOG_DECLARE("EVENT")
MUTEX_DECLARE(event);

static na_atomic_t		event_lock;
static na_event_list_t	na_event_list;

typedef struct
{
	na_event_list_t		*list;
	ushort				ev_type;
	void				*data;
	uint				datasize;
} na_clutter_event_t;

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

	list->have_changed = 1;
}

static gboolean na_event_clutter_idle_callback(gpointer data)
{
	na_event_t			*event;
	na_clutter_event_t	*evc	= (na_clutter_event_t *)data;
	for ( event = evc->list->lh_first; event != NULL; event = event->next.le_next )
	{
		if ( event->type == evc->ev_type )
			(*event->callback)(evc->ev_type, event->userdata, evc->data);
	}

	if ( evc->data != NULL )
		free(evc->data);
	free(evc);
	return FALSE;
}

void na_event_send_ex(na_event_list_t *list, ushort ev_type, void *data, uint datasize)
{
	na_clutter_event_t *evc;

	evc = malloc(sizeof(na_clutter_event_t));
	if ( evc == NULL )
		return;

	bzero(evc, sizeof(na_clutter_event_t));
	evc->list		= list;
	evc->ev_type	= ev_type;

	/* make a copy of data
	 */
	if ( data )
	{
		evc->data		= malloc(datasize);
		if ( evc->data == NULL )
		{
			free(evc);
			return;
		}

		evc->datasize	= datasize;
		bcopy(data, evc->data, datasize);
	}

	clutter_threads_add_idle(na_event_clutter_idle_callback, evc);
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

	list->have_changed = 1;
}

int  na_event_have_changed(na_event_list_t *list)
{
	return list->have_changed;
}

void na_event_clear_changed(na_event_list_t *list)
{
	list->have_changed = 0;
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

void na_event_send(ushort ev_type, void *data, uint datasize)
{
	if ( atomic_read(&event_lock) )
		return;
	na_event_send_ex(&na_event_list, ev_type, data, datasize);
}

void na_event_remove(ushort ev_type, na_event_callback callback, void *userdata)
{
	na_event_remove_ex(&na_event_list, ev_type, callback, userdata);
}

void na_event_stop(void)
{
	atomic_set(&event_lock, 1);
}
