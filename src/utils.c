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

void noya_color_write(char *buffer, int len, color_t *color)
{
	snprintf(buffer, sizeof(buffer),
		"%x%x%x%x",
		color[0],
		color[1],
		color[2],
		color[3]
	);
}

void noya_color_read(color_t *color, char *in)
{
	char	buf[2] = {0};
	char	*dst = (char *)color;

	if ( strlen(in) < 8 )
	{
		l_printf("Invalid color : %s", in);
		return;
	}

	buf[0] = in[0], buf[1] = in[1];
	dst[0] = strtol(buf, NULL, 16);
	buf[0] = in[2], buf[1] = in[3];
	dst[1] = strtol(buf, NULL, 16);
	buf[0] = in[4], buf[1] = in[5];
	dst[2] = strtol(buf, NULL, 16);
	buf[0] = in[6], buf[1] = in[7];
	dst[3] = strtol(buf, NULL, 16);
}
