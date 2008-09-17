#ifndef __THREAD_AUDIO_H
#define __THREAD_AUDIO_H

#include <sys/queue.h>

/* list of object to play
 */
LIST_HEAD(audio_head_s, audio_entry_s) audio_entries;
typedef struct audio_entry_s
{
#define AUDIO_ENTRY_FL_USED		0x01
#define	AUDIO_ENTRY_FL_LOADED	0x02
#define AUDIO_ENTRY_FL_FAILED	0x04
#define AUDIO_ENTRY_FL_PLAY		0x10
#define AUDIO_ENTRY_FL_ISLOOP	0x20
	sig_atomic_t	flags;

	unsigned int	id;
	char			*filename;

	/* data
	 */
	unsigned int	totalframes;
	float			*data;
	float			*datacur;
	int				dataidx;
	float			datalen;

	/* effects
	 */
	float			volume;

	LIST_ENTRY(audio_entry_s) next;
} audio_entry_t;

/* audio interface
 */
audio_entry_t *noya_audio_get_by_filename(char *filename);
audio_entry_t *noya_audio_get_by_id(unsigned int id);
audio_entry_t *noya_audio_load(char *filename, unsigned int id);
void noya_audio_set_loop(audio_entry_t *entry, short isloop);
void noya_audio_set_volume(audio_entry_t *entry, float volume);
void noya_audio_play(audio_entry_t *entry);
void noya_audio_stop(audio_entry_t *entry);
short noya_audio_is_play(audio_entry_t *entry);

/* thread management
 */
int thread_audio_start(void);
int thread_audio_stop(void);

#endif
