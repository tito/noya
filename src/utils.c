#include <string.h>
#include <assert.h>
#include <clutter/clutter.h>

#include "utils.h"

void trim(char *s)
{
	char *p, *os = s;
	short start = 0;

	for ( p = s; *s; s++ )
	{
		if ( !start && isspace((unsigned char)*s) )
			continue;
		start = 1;
		if ( (*s == '\\' || *s == '"') && *(s + 1) )
			s++;
		*p++ = *s;
	}
	*p = 0;

	s = os + strlen(os) - 1;
	while ( s >= os )
	{
		if ( !isspace((unsigned char)*s) )
			break;
		*s = 0;
		s--;
	}
}

void na_color_write(char *buffer, int len, na_color_t *color)
{
	char *scolor = (char *)color;
	snprintf(buffer, sizeof(buffer),
		"%02hhx%02hhx%02hhx%02hhx",
		scolor[0],
		scolor[1],
		scolor[2],
		scolor[3]
	);
}

int na_color_read(na_color_t *color, char *in)
{
	char	buf[2] = {0};
	char	*dst = (char *)color;

	if ( strlen(in) < 8 )
		return -1;

	buf[0] = in[0], buf[1] = in[1];
	dst[0] = strtol(buf, NULL, 16);
	buf[0] = in[2], buf[1] = in[3];
	dst[1] = strtol(buf, NULL, 16);
	buf[0] = in[4], buf[1] = in[5];
	dst[2] = strtol(buf, NULL, 16);
	buf[0] = in[6], buf[1] = in[7];
	dst[3] = strtol(buf, NULL, 16);

	return 0;
}
