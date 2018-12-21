#ifndef __COMMON_H__
#define __COMMON_H__

#define _STR(a)		#a
#define STR(a)		_STR(a)

#define RSHELL_F_LISTEN	(1 << 0)
#define RSHELL_F_NOFORK	(1 << 1)

struct plugin;

struct rshell
{
	char		*host;
	char		*service;
	uint16_t	port;
	int		family;
	char		*shell;
	unsigned	flags;
	int		backlog;
	char		*peer;
	struct plugin	*plugin;
};

#include <ctype.h>

#define CMD_BUFFER_LEN	4096

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
