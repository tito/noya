#ifndef __AUDIO_H
#define __AUDIO_H

#include <unistd.h>
#include <sys/queue.h>
#include <signal.h>

/* list of object to play
 */
typedef struct audio_s
{
#define AUDIO_FL_USED		0x01
#define	AUDIO_FL_LOADED		0x02
#define AUDIO_FL_FAILED		0x04
#define	AUDIO_FL_WANTSTOP	0x08
#define	AUDIO_FL_WANTPLAY	0x10
#define AUDIO_FL_PLAY		0x20
#define AUDIO_FL_ISLOOP		0x40
	__atomic__		flags;

	char			*filename;

	/* data
	 */
	short			channels;
	uint			totalframes;
	float			*data;
	float			*datacur;
	long			dataidx;
	float			datalen;

	/* control
	 */
	float			volume;				/*< RW : 0 - 1 (percent) */
	float			position;			/*< R  : 0 - 1 (percent) */
	float			duration;			/*< R  : 0 - n (seconds) */
	uint			bpmduration;		/*< R  : 0 - n (bpm) */
	int				bpmidx;				/*< RW : 0 - n (bpm from dataidx) */

	LIST_ENTRY(audio_s) next;
} audio_t;

typedef struct
{
	audio_t *lh_first;
} audio_list_t;

/* audio interface
 */
audio_t *noya_audio_get_by_filename(char *filename);
audio_t *noya_audio_load(char *filename);
void noya_audio_set_loop(audio_t *entry, short isloop);
void noya_audio_set_volume(audio_t *entry, float volume);
void noya_audio_wantplay(audio_t *entry);
void noya_audio_play(audio_t *entry);
void noya_audio_wantstop(audio_t *entry);
void noya_audio_stop(audio_t *entry);
void noya_audio_seek(audio_t *entry, long position);
short noya_audio_is_play(audio_t *entry);

extern audio_list_t audio_entries;

#endif
