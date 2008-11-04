#ifndef __EVENT_H
#define __EVENT_H

#include <unistd.h>
#include <sys/queue.h>

typedef void (*na_event_callback)(ushort ev_type, void *userdata, void *object);
typedef struct na_event_s
{
	ushort					type;
	na_event_callback		callback;
	void					*userdata;
	LIST_ENTRY(na_event_s)	next;
} na_event_t;

typedef struct
{
	na_event_t				*lh_first;
	int						have_changed;
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
#define NA_EV_WINDOW_UPDATE		0x0A

#define NA_EV_ACTOR_PREPARE		0xA1
#define NA_EV_ACTOR_UNPREPARE	0xA2
#define NA_EV_AUDIO_PLAY		0xA3
#define NA_EV_AUDIO_STOP		0xA4

/* default noya event list
 */
void na_event_init(void);
void na_event_free(void);
void na_event_observe(ushort ev_type, na_event_callback callback, void *userdata);
void na_event_send(ushort ev_type, void *data);
void na_event_remove(ushort ev_type, na_event_callback callback, void *userdata);
void na_event_stop(void);

/* for other event list (modules...)
 */
void na_event_init_ex(na_event_list_t *list);
void na_event_free_ex(na_event_list_t *list);
void na_event_observe_ex(na_event_list_t *list, ushort ev_type, na_event_callback callback, void *userdata);
void na_event_send_ex(na_event_list_t *list, ushort ev_type, void *data);
void na_event_remove_ex(na_event_list_t *list, ushort ev_type, na_event_callback callback, void *userdata);
int  na_event_have_changed(na_event_list_t *list);
void na_event_clear_changed(na_event_list_t *list);

#endif
