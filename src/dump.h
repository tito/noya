#ifndef __DUMP_H
#define __DUMP_H

#include <stdio.h>
#include "scene.h"

void noya_dump_str(FILE *fp, char *prefix, char *key, char *value);
void noya_dump_int(FILE *fp, char *prefix, char *key, int value);
void noya_dump_actor(FILE *fp, void *userdata, void *prefix, char *key, char *value);
void noya_dump(char *filename, scene_t *scene);

#endif
