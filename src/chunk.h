#ifndef __CHUNK_H
#define __CHUNK_H

typedef struct
{
	float		*data;
	ushort		channels;
	uint		size;
} na_chunk_t;

na_chunk_t	*na_chunk_new(ushort channels, uint size);
void na_chunk_free(na_chunk_t *chunk);

#define na_chunk_get_channels(chunk)			\
	((chunk)->channels)
#define na_chunk_get_channel(chunk, channel)	\
	(channel >= (chunk)->channels ? NULL : (chunk)->data + (channel * (chunk)->size))

#endif
