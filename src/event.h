#ifndef __EVENT_H
#define __EVENT_H

#include <sys/queue.h>

typedef void (*na_event_callback)(unsigned short ev_type, void *userdata, void *object);
typedef struct na_event_s
{
	unsigned short		type;
	na_event_callback		callback;
	void				*userdata;
	LIST_ENTRY(na_event_s)	next;
} na_event_t;

typedef struct
{
	na_event_t	*lh_first;
} na_event_list_t;

#define NA_EV_CURSOR_NEW		0x01
#define NA_EV_CURSOR_DEL		0x02
#define NA_EV_CURSOR_SET		0x03
#define NA_EV_CURSOR_CLICK		0x04
#define NA_EV_OBJECT_NEW		0x05
#define NA_EV_OBJECT_DEL		0x06
#define NA_EV_OBJECT_SET		0x07
#define NA_EV_BPM				0x08
#define NA_EV_SCENE_UPDATE		0x09

void na_event_init(void);
void na_event_free(void);
void na_event_observe(unsigned short ev_type, na_event_callback callback, void *userdata);
void na_event_send(unsigned short ev_type, void *data);
void na_event_remove(unsigned short ev_type, na_event_callback callback);

#endif
