#ifndef __NA_AUDIO_H
#define __NA_AUDIO_H

#include <unistd.h>
#include <sys/queue.h>
#include <signal.h>

#include "chunk.h"

/* list of object to play
 */
typedef struct na_audio_s
{
#define NA_AUDIO_FL_USED		0x01
#define	NA_AUDIO_FL_LOADED		0x02
#define NA_AUDIO_FL_FAILED		0x04
#define	NA_AUDIO_FL_WANTSTOP	0x08
#define	NA_AUDIO_FL_WANTPLAY	0x10
#define NA_AUDIO_FL_PLAY		0x20
#define NA_AUDIO_FL_ISLOOP		0x40
	na_atomic_t		flags;

	char			*filename;

	/* data
	 */
	short			channels;
	uint			totalframes;
	float			*data;
	float			*datacur;
	long			dataidx;
	float			datalen;

	/* entry chunk
	 */
	na_chunk_t		input;

	/* control
	 */
	float			volume;				/*< RW : 0 - 1 (percent) */
	float			position;			/*< R  : 0 - 1 (percent) */
	float			duration;			/*< R  : 0 - n (seconds) */
	uint			bpmduration;		/*< R  : 0 - n (bpm) */
	int				bpmidx;				/*< RW : 0 - n (bpm from dataidx) */

	LIST_ENTRY(na_audio_s) next;
} na_audio_t;

typedef struct
{
	na_audio_t *lh_first;
} na_audio_list_t;

/* audio interface
 */
na_audio_t *na_audio_get_by_filename(char *filename);
na_audio_t *na_audio_load(char *filename);
void na_audio_set_loop(na_audio_t *entry, short isloop);
void na_audio_set_volume(na_audio_t *entry, float volume);
void na_audio_wantplay(na_audio_t *entry);
void na_audio_play(na_audio_t *entry);
void na_audio_wantstop(na_audio_t *entry);
void na_audio_stop(na_audio_t *entry);
void na_audio_seek(na_audio_t *entry, long position);
short na_audio_is_play(na_audio_t *entry);

extern na_audio_list_t na_audio_entries;

#endif
