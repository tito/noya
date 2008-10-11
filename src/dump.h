#ifndef __DUMP_H
#define __DUMP_H

#include <stdio.h>
#include "scene.h"

void na_dump_str(FILE *fp, char *prefix, char *key, char *value);
void na_dump_int(FILE *fp, char *prefix, char *key, int value);
void na_dump_actor(FILE *fp, void *userdata, char *prefix, char *key, char *value);
void na_dump(char *filename, na_scene_t *scene);

#endif
