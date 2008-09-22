#ifndef __EVENT_H
#define __EVENT_H

#include <sys/queue.h>

typedef void (*event_callback)(unsigned short ev_type, void *userdata, void *object);
typedef struct event_s
{
	unsigned short		type;
	event_callback		callback;
	void				*userdata;
	LIST_ENTRY(event_s)	next;
} event_t;

typedef struct
{
	event_t	*lh_first;
} event_list_t;

#define EV_CURSOR_NEW		0x01
#define EV_CURSOR_DEL		0x02
#define EV_CURSOR_SET		0x03
#define EV_CURSOR_CLICK		0x04
#define EV_OBJECT_NEW		0x05
#define EV_OBJECT_DEL		0x06
#define EV_OBJECT_SET		0x07
#define EV_BPM				0x08

void noya_event_init(void);
void noya_event_free(void);
void noya_event_observe(unsigned short ev_type, event_callback callback, void *userdata);
void noya_event_send(unsigned short ev_type, void *data);
void noya_event_remove(unsigned short ev_type, event_callback callback);

#endif
