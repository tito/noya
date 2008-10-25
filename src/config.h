#ifndef __CONFIG_H
#define __CONFIG_H

#include <sys/queue.h>

/* Configuration
 * FIXME Move this !
 */
#define NA_VERSION				"0.1alpha"
#define NA_TITLE				"NOYA - Real time music"
#define NA_BANNER				"Noya, ver " NA_VERSION " - by Mathieu Virbel <tito@bankiz.org>"
#define NA_WEBSITE				"Visit http://noya.txzone.net/ for more information"
#define	NA_TUIO_PORT			"3333"
#define NA_CONFIG_FN			"config.ini"
#define NA_OUTPUT_CHANNELS		2
#define NA_DEF_SAMPLERATE		44100
#define NA_DEF_SOUNDS_MAX		128
#define NA_DEF_PATH_LEN			512

#define	TUIO_OBJECT_OSCPATH				"/tuio/2Dobj"
#define	TUIO_CURSOR_OSCPATH				"/tuio/2Dcur"
#define	TUIO_OBJECT_MAX					128
#define	TUIO_CURSOR_MAX					64
#define	TUIO_CURSOR_THRESHOLD_CLICK		3


#if NA_OUTPUT_CHANNELS != 2
#error "Noya support only output stereo !"
#endif


/* default configuration
 */
#define NA_CONFIG_DEFAULT			(&na_config_entries)

/* list key/value in config
 */
typedef struct na_config_s {
	struct na_config_entry_s	*lh_first;
} na_config_t;

typedef struct na_config_entry_s {
	char *k, *v;
	LIST_ENTRY(na_config_entry_s) next;
} na_config_entry_t;

int		na_config_init(char *filename);
int		na_config_load(na_config_t *head, char *filename);
void	na_config_set(na_config_t *head, char *key, char *value);
void	na_config_set_float(na_config_t *head, char *key, float value);
char	*na_config_get(na_config_t *head, char *key);
int		na_config_get_int(na_config_t *head, char *key);
float	na_config_get_float(na_config_t *head, char *key);
void	na_config_free(na_config_t *config);

extern	na_config_t na_config_entries;

#endif
