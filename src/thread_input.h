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

	uint			s_id;
	uint			f_id;				/*< ficudial identifiant */
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

	uint			s_id;				/*< id of cursor */
	float			xpos;				/*< X position of cursor */
	float			ypos;				/*< Y position of cursor */
	uint			mov;				/*< counter of movement */
} tuio_cursor_t;

int thread_input_start(void);
int thread_input_stop(void);

#endif
