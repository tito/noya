#ifndef __UTILS_H
#define __UTILS_H

typedef char color_t[4];

void trim(char *s);

void noya_color_write(char *buffer, int len, color_t *color);
int noya_color_read(color_t *color, char *in);

#endif
