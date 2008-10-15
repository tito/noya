#ifndef __UTILS_H
#define __UTILS_H

#define TUIO_PI_TO_DEG(a)	((a - 3.1416) * 57.2957795)    //360 / 2 x PI
#define PI2					6.2832

typedef char na_color_t[4];

void trim(char *s);

void na_color_write(char *buffer, int len, na_color_t *color);
int na_color_read(na_color_t *color, char *in);

#endif
