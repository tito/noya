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

na_config_t	na_config_entries;

int na_config_init(char *filename)
{
	assert( filename != NULL );

	/* init list
	 */
	LIST_INIT(&na_config_entries);

	return na_config_load(&na_config_entries, filename);
}

int na_config_load(na_config_t *head, char *filename)
{
	char line[512];
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
		fgets(line, sizeof(line), fd);

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
		na_config_set(head, line, value);
	}

	fclose(fd);
	return 0;
}

void na_config_set(na_config_t *head, char *key, char *value)
{
	na_config_entry_t *entry;

	assert( key != NULL );
	assert( value != NULL );

	entry = malloc(sizeof(na_config_entry_t));
	if ( entry == NULL )
		return;

	entry->k = strdup(key);
	entry->v = strdup(value);

	LIST_INSERT_HEAD(head, entry, next);
}

char *na_config_get(na_config_t *head, char *key)
{
	na_config_entry_t *entry;

	assert( head != NULL );
	assert( key != NULL );

	for ( entry = head->lh_first; entry != NULL; entry = entry->next.le_next )
		if ( strcmp(key, entry->k) == 0)
			return entry->v;

	return NULL;
}

int na_config_get_int(na_config_t *head, char *key)
{
	char *value;

	assert( head != NULL );

	value = na_config_get(head, key);
	if ( value == NULL )
		return -1;

	return strtol(value, NULL, 10);
}

void na_config_free(na_config_t *config)
{
	na_config_entry_t *entry;

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
