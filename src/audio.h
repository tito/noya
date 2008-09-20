#ifndef __AUDIO_H
#define __AUDIO_H

#include <sys/queue.h>
#include <signal.h>

/* list of object to play
 */
typedef struct audio_s
{
#define audio_FL_USED		0x01
#define	audio_FL_LOADED	0x02
#define audio_FL_FAILED	0x04
#define audio_FL_PLAY		0x10
#define audio_FL_ISLOOP	0x20
	sig_atomic_t	flags;

	char			*filename;

	/* data
	 */
	unsigned int	totalframes;
	float			*data;
	float			*datacur;
	long			dataidx;
	float			datalen;

	/* effects
	 */
	float			volume;
	float			position;

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
void noya_audio_play(audio_t *entry);
void noya_audio_stop(audio_t *entry);
void noya_audio_seek(audio_t *entry, long position);
short noya_audio_is_play(audio_t *entry);

extern audio_list_t audio_entries;

#endif
