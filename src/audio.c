#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#include <sndfile.h>

#include "noya.h"
#include "audio.h"

audio_list_t audio_entries;

audio_t *noya_audio_get_by_filename(char *filename)
{
	audio_t	*it;

	assert( filename != NULL );

	for ( it = audio_entries.lh_first; it != NULL; it = it->next.le_next )
	{
		if ( !(it->flags & AUDIO_FL_USED) )
			continue;
		if ( strcmp(filename, it->filename) == 0 )
			return it;
	}

	return NULL;
}

audio_t *noya_audio_load(char *filename)
{
	SNDFILE			*sfp = NULL;
	SF_INFO			sinfo;
	audio_t	*entry;

	assert( filename != NULL );

	entry = malloc(sizeof(audio_t));
	if ( entry == NULL )
		goto noya_audio_load_clean;
	bzero(entry, sizeof(audio_t));

	entry->filename = strdup(filename);
	if ( entry->filename == NULL )
		goto noya_audio_load_clean;

	entry->flags = AUDIO_FL_USED;

	LIST_INSERT_HEAD(&audio_entries, entry, next);

	return entry;

noya_audio_load_clean:;
	if ( entry != NULL )
	{
		if ( sfp != NULL )
			sf_close(sfp);
		if ( entry->filename != NULL )
			free(entry->filename), entry->filename = NULL;
		if ( entry->data != NULL )
			free(entry->data), entry->data = NULL;
		free(entry);
	}
	return NULL;
}

void noya_audio_play(audio_t *entry)
{
	assert( entry != NULL );
	entry->flags |= AUDIO_FL_WANTPLAY;
}

void noya_audio_stop(audio_t *entry)
{
	assert( entry != NULL );
	entry->flags &= ~AUDIO_FL_PLAY;
	entry->flags &= ~AUDIO_FL_WANTPLAY;
}

void noya_audio_set_loop(audio_t *entry, short isloop)
{
	assert( entry != NULL );
	if ( isloop )
		entry->flags |= AUDIO_FL_ISLOOP;
	else
		entry->flags &= ~AUDIO_FL_ISLOOP;
}

void noya_audio_set_volume(audio_t *entry, float volume)
{
	assert( entry != NULL );
	entry->volume = volume;
}

short noya_audio_is_play(audio_t *entry)
{
	assert( entry != NULL );
	return entry->flags & AUDIO_FL_PLAY;
}

void noya_audio_seek(audio_t *entry, long position)
{
	entry->dataidx = position;
}

