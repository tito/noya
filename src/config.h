#ifndef __CONFIG_H
#define __CONFIG_H

#include <sys/queue.h>

/* Configuration
 * FIXME Move this !
 */
#define NA_VERSION				"1.0"
#define NA_TITLE				"NOYA - Real time music"
#define NA_BANNER				"Noya, ver " NA_VERSION " - by Mathieu Virbel <tito@bankiz.org>"
#define	NA_TUIO_PORT			"3333"
#define NA_CONFIG_FN			"config.ini"


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
char	*na_config_get(na_config_t *head, char *key);
int		na_config_get_int(na_config_t *head, char *key);
void	na_config_free(na_config_t *config);

extern	na_config_t na_config_entries;

#endif
