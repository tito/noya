#ifndef __UTILS_H
#define __UTILS_H

#define TUIO_PI_TO_DEG(a)	((a - 3.1416) * 57.2957795)    //360 / 2 x PI
#define PI2					6.2832

typedef char na_color_t[4];

#include <libconfig.h>

config_setting_t *config_lookup_with_default(config_t *base, config_t *current, char *name);
config_setting_t *config_setting_get_member_with_class(config_setting_t *classes, char *class, config_setting_t *current, char *name);
int fileexist(char *name);
void trim(char *s);

void na_color_write(char *buffer, int len, na_color_t *color);
int na_color_read(na_color_t *color, const char *in);

#endif
