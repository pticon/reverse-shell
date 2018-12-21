#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "common.h"
#include "reverse-listener.h"
#include "reverse-shell.h"
#include "plugin.h"
#include "plain_tcp.h"
#include "plain_udp.h"

#define PROGNAME	"rshell"
#define VERSION		"1.2.0-dev"
#define SHELL		"sh"
#define PATH		"/bin"
#define PATHSHELL	PATH "/" SHELL


static void version(void)
{
	fprintf(stderr, "%s %s\n", PROGNAME, VERSION);

	exit(0);
}


static void usage(int ret)
{
	fprintf(stderr, "usage: %s [options] <host> <port> [peer]\n", PROGNAME);
	fprintf(stderr, "arguments:\n");
	fprintf(stderr, "\thost        : mandatory specific host to bind or connect (could be \"any\" in listen mode)\n");
	fprintf(stderr, "\tport        : mandatory specific port to bind or connect\n");
	fprintf(stderr, "\tpeer        : optional peer address to accept connection from in listen mode\n");
	fprintf(stderr, "options:\n");
	fprintf(stderr, "\t-4         : use IPv4 socket\n");
	fprintf(stderr, "\t-6         : use IPv6 socket\n");
	fprintf(stderr, "\t-f         : foreground mode (eg: no fork)\n");
	fprintf(stderr, "\t-h         : display this and exit\n");
	fprintf(stderr, "\t-l         : listen mode\n");
	fprintf(stderr, "\t-s <shell> : give the path shell (default: %s)\n", PATHSHELL);
	fprintf(stderr, "\t-u         : use an UDP socket\n");
	fprintf(stderr, "\t-v         : display version and exit\n");

	exit(ret);
}


static void rshell_init(struct rshell *rshell)
{
	rshell->family	= AF_UNSPEC;
	rshell->shell	= PATHSHELL;
	rshell->flags	= 0;
	rshell->peer	= NULL;
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


static int parse_option(int argc, char *argv[], struct rshell *rshell)
{
	int		c;

	rshell_init(rshell);

	while ( (c = getopt(argc, argv, "46fhls:uv")) != -1 )
	{
		switch ( c )
		{
			case '4':
			rshell->family = AF_INET;
			break;

			case '6':
			rshell->family = AF_INET6;
			break;

			case 'f':
			rshell->flags |= RSHELL_F_NOFORK;
			break;

			case 'h':
			usage(0);
			/* NOTREACHED */
			return 0;

			case 'l':
			rshell->flags |= RSHELL_F_LISTEN;
			break;

			case 's':
			rshell->shell = optarg;
			break;

			case 'u':
			rshell->plugin = &plain_udp_plugin;
			break;

			case 'v':
			version();
			return 0;
		}
	}

	argv += optind;
	argc -= optind;

	if ( rshell->flags & RSHELL_F_LISTEN )
	{
		if ( argc < 2 || argc > 3 )
			usage(-1);

		if ( argc == 3 )
			rshell->peer = argv[2];
	}
	else
	{
		if ( argc != 2 )
			usage(-1);
	}

	rshell->host = argv[0];
	if ( rshell_set_port(rshell, argv[1]) )
	{
		fprintf(stderr, "Invalid port %s\n", argv[1]);
		return -1;
	}

	return 0;
}


int main(int argc, char *argv[])
{
	int		ret;
	struct rshell	rshell;

	if ( (ret = parse_option(argc, argv, &rshell)) )
		return ret;

	if ( rshell.flags & RSHELL_F_LISTEN )
		return rshell_serve(&rshell);

	return rshell_connect(&rshell);
}
