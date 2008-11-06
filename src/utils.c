#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include <clutter/clutter.h>

#include "noya.h"
#include "utils.h"

LOG_DECLARE("UTILS");

config_setting_t *config_setting_get_member_with_class(config_setting_t *classes, char *class, config_setting_t *current, char *name)
{
	config_setting_t	*setting,
						*prop,
						*parent;
	char				*s1, *s2;

	/* search in current setting
	 */
	setting = config_setting_get_member(current, name);
	if ( setting )
		return setting;

	/* or search in class
	 */
	if ( classes == NULL || class == NULL )
		return NULL;

	setting = config_setting_get_member(classes, class);
	if ( setting == NULL )
		return NULL;

retry_parent:
	prop = config_setting_get_member(setting, name);
	if ( prop != NULL )
		return prop;

	/* class have parent ?
	 */
	parent = config_setting_get_member(setting, "parent");
	if ( parent == NULL )
		return NULL;

	s1 = config_setting_name(setting);
	s2 = config_setting_name(parent);

	if ( s1 && s2 && strcasecmp(s1, s2) == 0 )
	{
		l_errorf("recursion detected, abort!");
		return NULL;
	}

	/* seem ok, retry with parent
	 */
	setting = parent;
	goto retry_parent;
}

config_setting_t *config_lookup_with_default(config_t *base, config_t *current, char *name)
{
	char				basename[256];
	config_setting_t	*setting;

	setting = config_lookup(current, name);
	if ( setting != NULL )
		return setting;

	snprintf(basename, sizeof(basename), "noya.%s", name);
	setting = config_lookup(base, name);
	if ( setting != NULL )
		return setting;

	return NULL;
}

int fileexist(char *name)
{
	FILE *file;

	file = fopen(name, "r");
	if ( file == NULL )
		return 0;

	fclose(file);
	return 1;
}

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

int na_color_read(na_color_t *color, const char *in)
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
