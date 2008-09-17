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

