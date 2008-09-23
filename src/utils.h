#ifndef __UTILS_H
#define __UTILS_H

#include <clutter/clutter.h>
#include "thread_manager.h"

typedef char color_t[4];

void trim(char *s);

void noya_color_write(char *buffer, int len, color_t *color);
void noya_color_read(color_t *color, char *in);

#endif
