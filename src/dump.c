#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <strings.h>

#include "noya.h"
#include "dump.h"
#include "scene.h"

LOG_DECLARE("DUMP");

void noya_dump_str(FILE *fp, char *prefix, char *key, char *value)
{
	fprintf(fp, "%s.%s = %s\n", prefix, key, value);
}

void noya_dump_int(FILE *fp, char *prefix, char *key, int value)
{
	fprintf(fp, "%s.%s = %d\n", prefix, key, value);
}

void noya_dump_actor(FILE *fp, void *userdata, void *prefix, char *key, char *value)
{
	char				buffer[12];
	scene_actor_base_t	*actor;

	assert( userdata != NULL );

	actor = (scene_actor_base_t *)userdata;

	fprintf(fp, "# settings for %s\n", prefix);
	fprintf(fp, "%s.width = %d\n", prefix, actor->width);
	fprintf(fp, "%s.height = %d\n", prefix, actor->height);
	noya_color_write(buffer, sizeof(buffer), &actor->background_color);
	fprintf(fp, "%s.background.color = %s\n", prefix, buffer);
	noya_color_write(buffer, sizeof(buffer), &actor->border_color);
	fprintf(fp, "%s.border.color = %s\n", prefix, buffer);
	fprintf(fp, "%s.border.width = %d\n", prefix, actor->border_width);
	fprintf(fp, "%s.loop = %d\n", prefix, actor->is_loop ? 1 : 0);
	fprintf(fp, "\n");
}

void noya_dump(char *filename, scene_t *scene)
{
	FILE	*fp = stderr;
	char	buffer[128];
	scene_actor_base_t *actor;

	if ( filename != NULL )
	{
		fp = fopen(filename, "w");
		if ( fp == NULL )
		{
			l_errorf("unable to open %s", filename);
			return;
		}
	}

	/* dump general informations
	 */
	fprintf(fp, "# Scene dump\n");
	fprintf(fp, "# @name : %s\n", scene->name);
	fprintf(fp, "\n");

	/* dump scene config
	 */
	noya_color_write(buffer, sizeof(buffer), &scene->background_color);
	noya_dump_str(fp, "scene", "background.color", buffer);
	noya_dump_int(fp, "scene", "config.precision", scene->precision);
	noya_dump_int(fp, "scene", "config.bpm", scene->bpm);

	/* dump default actors
	 */
	noya_dump_actor(fp, &scene->def_actor, "scene.act.default", NULL, NULL);


	/* dump actors
	 */
	for ( actor = scene->actors.lh_first; actor != NULL; actor = actor->next.le_next )
	{
		snprintf(buffer, sizeof(buffer), "scene.act.%d", actor->id);
		noya_dump_actor(fp, actor, buffer, NULL, NULL);
	}


	if ( filename != NULL )
		fclose(fp);
}
