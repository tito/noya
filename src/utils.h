#ifndef __UTILS_H
#define __UTILS_H

typedef char na_color_t[4];

void trim(char *s);

void na_color_write(char *buffer, int len, na_color_t *color);
int na_color_read(na_color_t *color, char *in);

#endif
