#ifndef __CONFIG_H
#define __CONFIG_H

#include <sys/queue.h>

/* Configuration
 * FIXME Move this !
 */
#define	NOYA_DEBUG				1
#define NOYA_VERSION			"1.0"
#define NOYA_TITLE				"NOYA - Real time music"
#define NOYA_BANNER				"Noya, ver " NOYA_VERSION " - by Mathieu Virbel <tito@bankiz.org>"
#define	NOYA_TUIO_PORT			"3333"
#define NOYA_DEFAULT_FRAMERATE	1
#define NOYA_CONFIG_FN			"config.ini"


/* default configuration
 */
#define CONFIG_DEFAULT			(&config_entries)

/* list key/value in config
 */
typedef struct config_s {
	struct config_entry_s	*lh_first;
} config_t;

typedef struct config_entry_s {
	char *k, *v;
	LIST_ENTRY(config_entry_s) next;
} config_entry_t;

int		config_init(char *filename);
int		config_load(config_t *head, char *filename);
void	config_set(config_t *head, char *key, char *value);
char	*config_get(config_t *head, char *key);
int		config_get_int(config_t *head, char *key);
void	config_free(config_t *config);

extern	config_t config_entries;

#endif
