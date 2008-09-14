#ifndef __CONFIG_H
#define __CONFIG_H

#define	NOYA_DEBUG				1
#define NOYA_VERSION			"1.0"
#define NOYA_TITLE				"NOYA - Real time music"
#define NOYA_BANNER				"Noya, ver " NOYA_VERSION " - by Mathieu Virbel <tito@bankiz.org>"
#define	NOYA_TUIO_PORT			"3333"
#define NOYA_DEFAULT_FRAMERATE	1
#define NOYA_CONFIG_FN			"config.ini"


int		config_init(char *filename);
void	config_set(char *key, char *value);
char	*config_get(char *key);
int		config_get_int(char *key);

#endif
