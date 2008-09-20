#include <string.h>
#include <assert.h>
#include <clutter/clutter.h>

#include "utils.h"

void trim(char *s)
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

void noya_color_copy(ClutterColor *dst, color_t *src)
{
	assert( sizeof(ClutterColor) == sizeof(color_t) );
	memcpy(dst, src, sizeof(color_t));
}

