#ifndef __THREAD_INPUT_H
#define __THREAD_INPUT_H

/* Structures
 */
typedef struct
{
#define TUIO_OBJECT_FL_INIT		0x01	/*< indicate if object is just init */
#define TUIO_OBJECT_FL_USED		0x02	/*< indicate if object is used */
#define TUIO_OBJECT_FL_TRYFREE	0x04	/*< used for blur event */
	unsigned short	flags;

	unsigned int	s_id;
	unsigned int	f_id;
	float			xpos;
	float			ypos;
	float			angle;
	float			xspeed;
	float			yspeed;
	float			rspeed;
	float			maccel;
	float			raccel;
} tuio_object_t;

typedef struct
{
#define TUIO_CURSOR_FL_INIT		0x01	/*< indicate if cursor is just init */
#define TUIO_CURSOR_FL_USED		0x02	/*< indicate if it in used */
#define TUIO_CURSOR_FL_TRYFREE	0x04	/*< used for blur event */
	unsigned short	flags;

	unsigned int	s_id;				/*< id of cursor */
	float			xpos;				/*< X position of cursor */
	float			ypos;				/*< Y position of cursor */
	unsigned int	mov;				/*< counter of movement */
} tuio_cursor_t;

typedef void (*event_callback)(unsigned short ev_type, void *object);
typedef struct
{
	unsigned short	type;
	event_callback	callback;
} tuio_event_t;

#define EV_CURSOR_NEW		0x01
#define EV_CURSOR_DEL		0x02
#define EV_CURSOR_SET		0x03
#define EV_CURSOR_CLICK		0x04
#define EV_OBJECT_NEW		0x05
#define EV_OBJECT_DEL		0x06
#define EV_OBJECT_SET		0x07

void event_observe(unsigned short ev_type, event_callback callback);
void event_send(unsigned short ev_type, void *data);

int thread_input_start(void);
int thread_input_stop(void);

#endif
