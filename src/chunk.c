#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

#include "noya.h"
#include "chunk.h"

na_chunk_t	*na_chunk_new(ushort channels, uint size)
{
	na_chunk_t *chunk;

	/* we want, for precaution, at least 1 channel with 16 sample.
	 */
	assert( channels >= 1 );
	assert( size >= 16 );

	chunk = malloc(sizeof(na_chunk_t));
	if ( chunk == NULL )
		return NULL;

	chunk->size		= size;
	chunk->channels	= channels;
	chunk->data		= malloc(sizeof(float) * chunk->size * chunk->channels);
	if ( chunk->data == NULL )
	{
		free(chunk);
		return NULL;
	}

	return chunk;
}

void na_chunk_free(na_chunk_t *chunk)
{
	if ( chunk->data )
		free(chunk->data);
	free(chunk);
}
