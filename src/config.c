#include <stdio.h>
#include <stdlib.h>
#include <sys/queue.h>
#include <assert.h>
#include <string.h>
#include <pthread.h>
#include <ctype.h>

#include "noya.h"
#include "utils.h"

LOG_DECLARE("CONFIG");

#define LINE_MAX	512

config_t	config_entries;

int config_init(char *filename)
{
	assert( filename != NULL );

	/* init list
	 */
	LIST_INIT(&config_entries);

	return config_load(&config_entries, filename);
}

int config_load(config_t *head, char *filename)
{
	char line[LINE_MAX];
	char *key, *value;
	FILE *fd;

	assert( filename != NULL );

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
		config_set(head, line, value);
	}

	fclose(fd);
	return 0;
}

void config_set(config_t *head, char *key, char *value)
{
	config_entry_t *entry;

	assert( key != NULL );
	assert( value != NULL );

	entry = malloc(sizeof(config_entry_t));
	if ( entry == NULL )
		return;

	entry->k = strdup(key);
	entry->v = strdup(value);

	LIST_INSERT_HEAD(head, entry, next);
}

char *config_get(config_t *head, char *key)
{
	config_entry_t *entry;

	assert( head != NULL );
	assert( key != NULL );

	for ( entry = head->lh_first; entry != NULL; entry = entry->next.le_next )
		if ( strcmp(key, entry->k) == 0)
			return entry->v;

	return NULL;
}

int config_get_int(config_t *head, char *key)
{
	char *value;

	assert( head != NULL );

	value = config_get(head, key);
	if ( value == NULL )
		return -1;

	return strtol(value, NULL, 10);
}

void config_free(config_t *config)
{
	config_entry_t *entry;

	assert( config != NULL );

	while ( !LIST_EMPTY(config) )
	{
		entry = LIST_FIRST(config);
		LIST_REMOVE(entry, next);
		if ( entry->k )
			free(entry->k);
		if ( entry->v )
			free(entry->v);
		free(entry);
	}
}
