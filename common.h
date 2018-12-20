#ifndef __COMMON_H__
#define __COMMON_H__

#define VERSION		"1.2.0-dev"

#include <ctype.h>

#define CMD_BUFFER_LEN	4096

static inline void version(void)
{
	fprintf(stderr, "%s %s\n", PROGNAME, VERSION);
}

static inline char *trim(char *str, ssize_t *plen)
{
	char	*end;
	ssize_t	len = *plen;

	while ( len && isspace(*str) )
		str++, len--;

	if ( len )
	{
		end = str + len - 1;

		while ( end > str && isspace(*end) )
			len--, *end = '\0', end--;
	}

	*plen = len;

	return str;
}

#endif /* __COMMON_H__ */
