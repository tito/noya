#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#include <sndfile.h>

#include "noya.h"
#include "audio.h"

LOG_DECLARE("AUDIO");
MUTEX_DECLARE(audiosfx);

na_audio_list_t na_audio_entries;

static void na_audio_update_output(na_audio_t *audio)
{
	na_audio_sfx_t	*sfx;
	na_chunk_t		*chunk;

	assert( audio != NULL );

	/* search chunk to connect
	 */
	if ( LIST_EMPTY(&audio->sfx) )
		chunk = audio->input;
	else
	{
		/* get the last sfx in the list...
		 */
		LIST_FOREACH(sfx, &audio->sfx, next)
		{
			chunk = sfx->out;
		}
	}

	assert( na_chunk_get_channels(chunk) > 0 );

	if ( na_chunk_get_channels(chunk) == 1 )
	{
		audio->out_L = na_chunk_get_channel(chunk, 0);
		audio->out_R = na_chunk_get_channel(chunk, 0);
	}
	else
	{
		audio->out_L = na_chunk_get_channel(chunk, 0);
		audio->out_R = na_chunk_get_channel(chunk, 1);
	}
}

na_audio_t *na_audio_get_by_filename(char *filename)
{
	na_audio_t	*it;

	assert( filename != NULL );

	for ( it = na_audio_entries.lh_first; it != NULL; it = it->next.le_next )
	{
		if ( !(atomic_read(&it->flags) & NA_AUDIO_FL_USED) )
			continue;
		if ( strcmp(filename, it->filename) == 0 )
			return it;
	}

	return NULL;
}

na_audio_t *na_audio_load(char *filename)
{
	int			cfg_frames;
	na_audio_t	*entry;

	assert( filename != NULL );

	entry = malloc(sizeof(na_audio_t));
	if ( entry == NULL )
		goto na_audio_load_clean;
	bzero(entry, sizeof(na_audio_t));

	entry->filename	= strdup(filename);
	if ( entry->filename == NULL )
		goto na_audio_load_clean;

	atomic_set(&entry->flags, NA_AUDIO_FL_USED);
	entry->bpmidx	= -1;

	cfg_frames		= config_lookup_int(&g_config, "noya.audio.frames");
	entry->input	= na_chunk_new(NA_OUTPUT_CHANNELS, cfg_frames);
	if ( entry->input == NULL )
		goto na_audio_load_clean;

	entry->_ref++;
	LIST_INSERT_HEAD(&na_audio_entries, entry, next);

	return entry;

na_audio_load_clean:;
	if ( entry != NULL )
		na_audio_free(entry);
	return NULL;
}

void na_audio_free(na_audio_t *entry)
{
	entry->_ref--;
	if ( entry->_ref )
		return;

	if ( entry->filename != NULL )
		free(entry->filename);
	if ( entry->data != NULL )
		free(entry->data);
	if ( entry->input != NULL )
		na_chunk_free(entry->input);
	free(entry);
}

void na_audio_play(na_audio_t *entry)
{
	assert( entry != NULL );
	if ( atomic_read(&entry->flags) & NA_AUDIO_FL_PLAY )
		return;
	na_audio_update_output(entry);
	atomic_set(&entry->flags, atomic_read(&entry->flags) | NA_AUDIO_FL_PLAY);
}

void na_audio_wantplay(na_audio_t *entry)
{
	assert( entry != NULL );
	atomic_set(&entry->flags, atomic_read(&entry->flags) | NA_AUDIO_FL_WANTPLAY);
}

void na_audio_wantstop(na_audio_t *entry)
{
	assert( entry != NULL );
	atomic_set(&entry->flags, atomic_read(&entry->flags) | NA_AUDIO_FL_WANTSTOP);
}

void na_audio_stop(na_audio_t *entry)
{
	assert( entry != NULL );
	atomic_set(&entry->flags, atomic_read(&entry->flags) & ~NA_AUDIO_FL_PLAY);
	atomic_set(&entry->flags, atomic_read(&entry->flags) & ~NA_AUDIO_FL_WANTPLAY);
	atomic_set(&entry->flags, atomic_read(&entry->flags) & ~NA_AUDIO_FL_WANTSTOP);
}

void na_audio_set_loop(na_audio_t *entry, short isloop)
{
	assert( entry != NULL );
	if ( isloop )
		atomic_set(&entry->flags, atomic_read(&entry->flags) | NA_AUDIO_FL_ISLOOP);
	else
		atomic_set(&entry->flags, atomic_read(&entry->flags) & ~NA_AUDIO_FL_ISLOOP);
}

void na_audio_set_volume(na_audio_t *entry, float volume)
{
	assert( entry != NULL );
	entry->volume = volume;
}

short na_audio_is_play(na_audio_t *entry)
{
	assert( entry != NULL );
	return atomic_read(&entry->flags) & NA_AUDIO_FL_PLAY;
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

void na_audio_sfx_free(na_audio_sfx_t *sfx)
{
	if ( sfx->out )
		na_chunk_free(sfx->out);
	free(sfx);
}

na_audio_sfx_t *na_audio_sfx_new(
	int in_channels,
	int out_channels,
	na_audio_sfx_connect_fn fn_connect,
	na_audio_sfx_disconnect_fn fn_disconnect,
	na_audio_sfx_process_fn fn_process,
	void *userdata)
{
	na_audio_sfx_t	*sfx;
	int				cfg_frames;

	sfx = malloc(sizeof(na_audio_sfx_t));
	if ( sfx == NULL )
		goto na_audio_sfx_add_failed;
	bzero(sfx, sizeof(na_audio_sfx_t));

	sfx->userdata			= userdata;
	sfx->in_channels		= in_channels;
	sfx->out_channels		= out_channels;
	sfx->fn_connect			= fn_connect;
	sfx->fn_disconnect		= fn_disconnect;
	sfx->fn_process			= fn_process;

	/* create chunk for output
	 */
	cfg_frames		= config_lookup_int(&g_config, "noya.audio.frames");
	sfx->out		= na_chunk_new(out_channels, cfg_frames);
	if ( sfx->out == NULL )
		goto na_audio_sfx_add_failed;

	return sfx;

na_audio_sfx_add_failed:
	if ( sfx )
		na_audio_sfx_free(sfx);
	return NULL;
}

void na_audio_sfx_connect(na_audio_t *audio, na_audio_sfx_t *sfx)
{
	assert( audio != NULL );
	assert( sfx != NULL );

	/* connect effect with head
	 */
	sfx->in = audio->input;
	if ( !LIST_EMPTY(&audio->sfx) )
		LIST_FIRST(&audio->sfx)->in = sfx->out;

	/* add effect to list
	 */
	LIST_INSERT_HEAD(&audio->sfx, sfx, next);
	na_audio_update_output(audio);

	/* connect plugin
	 */
	if ( sfx->fn_connect )
	{
		sfx->fn_connect(sfx->userdata, sfx->in, sfx->out);
		na_audio_update_output(audio);
	}
}

void na_audio_sfx_disconnect(na_audio_t *audio, na_audio_sfx_t *sfx)
{
	assert( sfx != NULL );

	/* link the next sfx to the current link
	 * => preserve link between sfx.
	 */
	if ( sfx->next.le_next )
		sfx->next.le_next->in = sfx->in;

	/* remove from list, and update links
	 */
	LIST_REMOVE(sfx, next);
	na_audio_update_output(audio);

	/* disconnect plugin
	 */
	if ( sfx->fn_disconnect )
		sfx->fn_disconnect(sfx->userdata);

	na_audio_sfx_free(sfx);
}

void na_audio_sfx_process(na_audio_t *audio)
{
	na_audio_sfx_t	*sfx;

	assert( audio != NULL );

	/* nothing to process if they are no sfx.
	 */
	if ( LIST_EMPTY(&audio->sfx) )
		return;

	/* process effects plugin
	 */
	LIST_FOREACH(sfx, &audio->sfx, next)
	{
		sfx->fn_process(sfx->userdata);
	}
}
