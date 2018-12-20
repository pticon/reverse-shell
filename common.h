#ifndef __COMMON_H__
#define __COMMON_H__

#define VERSION		"1.2.0-dev"


static inline void version(void)
{
	fprintf(stderr, "%s %s\n", PROGNAME, VERSION);
}

#endif /* __COMMON_H__ */
