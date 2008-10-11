#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#include <sndfile.h>

#include "noya.h"
#include "audio.h"

LOG_DECLARE("AUDIO");

na_audio_list_t na_audio_entries;

na_audio_t *na_audio_get_by_filename(char *filename)
{
	na_audio_t	*it;

	assert( filename != NULL );

	for ( it = na_audio_entries.lh_first; it != NULL; it = it->next.le_next )
	{
		if ( !(it->flags & NA_AUDIO_FL_USED) )
			continue;
		if ( strcmp(filename, it->filename) == 0 )
			return it;
	}

	return NULL;
}

na_audio_t *na_audio_load(char *filename)
{
	na_audio_t	*entry;

	assert( filename != NULL );

	entry = malloc(sizeof(na_audio_t));
	if ( entry == NULL )
		goto na_audio_load_clean;
	bzero(entry, sizeof(na_audio_t));

	entry->filename	= strdup(filename);
	if ( entry->filename == NULL )
		goto na_audio_load_clean;

	entry->flags	= NA_AUDIO_FL_USED;
	entry->bpmidx	= -1;

	LIST_INSERT_HEAD(&na_audio_entries, entry, next);

	return entry;

na_audio_load_clean:;
	if ( entry != NULL )
	{
		if ( entry->filename != NULL )
			free(entry->filename), entry->filename = NULL;
		if ( entry->data != NULL )
			free(entry->data), entry->data = NULL;
		free(entry);
	}
	return NULL;
}

void na_audio_play(na_audio_t *entry)
{
	assert( entry != NULL );
	entry->flags |= NA_AUDIO_FL_PLAY;
}

void na_audio_wantplay(na_audio_t *entry)
{
	assert( entry != NULL );
	entry->flags |= NA_AUDIO_FL_WANTPLAY;
}

void na_audio_wantstop(na_audio_t *entry)
{
	assert( entry != NULL );
	entry->flags |= NA_AUDIO_FL_WANTSTOP;
}

void na_audio_stop(na_audio_t *entry)
{
	assert( entry != NULL );
	entry->flags &= ~NA_AUDIO_FL_PLAY;
	entry->flags &= ~NA_AUDIO_FL_WANTPLAY;
	entry->flags &= ~NA_AUDIO_FL_WANTSTOP;
}

void na_audio_set_loop(na_audio_t *entry, short isloop)
{
	assert( entry != NULL );
	if ( isloop )
		entry->flags |= NA_AUDIO_FL_ISLOOP;
	else
		entry->flags &= ~NA_AUDIO_FL_ISLOOP;
}

void na_audio_set_volume(na_audio_t *entry, float volume)
{
	assert( entry != NULL );
	entry->volume = volume;
}

short na_audio_is_play(na_audio_t *entry)
{
	assert( entry != NULL );
	return entry->flags & NA_AUDIO_FL_PLAY;
}

void na_audio_seek(na_audio_t *entry, long position)
{
	if ( position == 0 )
	{
		entry->dataidx	= position;
		entry->bpmidx	= -1;
	}
	else
	{
		l_errorf("seek in another position than 0 is not yet implemented.");
		/* FIXME : handle seek in another position than 0.
		 * This is due to the manager loop.
		 */
	}
}

