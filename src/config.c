#include <stdio.h>
#include <stdlib.h>
#include <sys/queue.h>
#include <assert.h>
#include <string.h>
#include <pthread.h>
#include <ctype.h>

#include "noya.h"

#define LINE_MAX	512

/* list of object in scene
 */
LIST_HEAD(config_head_s, config_entry_s) config_entries;
typedef struct config_entry_s {
	char *k, *v;
	LIST_ENTRY(config_entry_s) next;
} config_entry_t;

static void trim(char *s)
{
	char *p;

	for ( p = s; *s; s++ )
	{
		if ( isspace((unsigned char)*s) )
			continue;
		if ( *s == '\\' && *(s + 1) )
			s++;
		*p++ = *s;
	}
	*p = 0;
}

int config_init(char *filename)
{
	char line[LINE_MAX];
	char *key, *value;
	FILE *fd;

	assert( filename != NULL );

	/* init list
	 */
	LIST_INIT(&config_entries);

	/* read config
	 */
	fd = fopen(filename, "r");
	if ( fd == NULL )
		return -1;

	while ( !feof(fd) )
	{
		fgets(line, LINE_MAX, fd);

		/* skip comment
		 */
		if ( line[0] == '#' )
			continue;

		/* parse key
		 */
		key = strtok(line, "=");
		if ( key == NULL )
			continue;

		/* parse value
		 */
		value = strtok(NULL, "=");
		if ( value == NULL )
			continue;

		/* separate string
		 */
		*value = '\0';
		value++;

		trim(key);
		trim(value);

		l_printf("Read <%s> = <%s>", key, value);
		config_set(line, value);
	}

	fclose(fd);
	return 0;
}

void config_set(char *key, char *value)
{
	config_entry_t *entry;

	assert( key != NULL );
	assert( value != NULL );

	entry = malloc(sizeof(config_entry_t));
	if ( entry == NULL )
		return;

	entry->k = strdup(key);
	entry->v = strdup(value);

	LIST_INSERT_HEAD(&config_entries, entry, next);
}

char *config_get(char *key)
{
	config_entry_t *entry;

	assert( key != NULL );

	for ( entry = config_entries.lh_first; entry != NULL; entry = entry->next.le_next )
		if ( strcmp(key, entry->k) == 0)
			return entry->v;

	return NULL;
}

int config_get_int(char *key)
{
	char *value;

	value = config_get(key);
	if ( value == NULL )
		return -1;

	return strtol(value, NULL, 10);
}
