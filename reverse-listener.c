#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>

#define PROGNAME	"reverse-listener"
#define LISTEN_BACKLOG	1
#define _STR(a)		#a
#define STR(a)		_STR(a)

#include "common.h"
#include "plugin.h"
#include "plain_tcp.h"
#include "plain_udp.h"

struct listener
{
	char		*host;
	char		*service;
	uint16_t	port;
	int		family;
	int		backlog;
	char		*peer;
	struct plugin	*plugin;
};


static void usage(void)
{
	fprintf(stderr, "usage: %s [options] <host> <port> [peer]\n", PROGNAME);
	fprintf(stderr, "arguments:\n");
	fprintf(stderr, "\thost        : mandatory specific host to bind (could be \"any\")\n");
	fprintf(stderr, "\tport        : mandatory specific port to bind\n");
	fprintf(stderr, "\tpeer        : optional peer address to accept connection from\n");
	fprintf(stderr, "options:\n");
	fprintf(stderr, "\t-b <backlog>: define the maximum length to which the queue of pending connections may grow (default: " STR(LISTEN_BACKLOG) ")\n");
	fprintf(stderr, "\t-h          : display this and exit\n");
	fprintf(stderr, "\t-v          : display version and exit\n");
	fprintf(stderr, "\t-4          : use IPv4 socket\n");
	fprintf(stderr, "\t-6          : use IPv6 socket\n");
	fprintf(stderr, "\t-u          : use an UDP socket\n");
}


static void listener_init(struct listener *listener)
{
	listener->family	= AF_UNSPEC;
	listener->backlog	= LISTEN_BACKLOG;
	listener->peer		= NULL;
	listener->plugin	= &plain_tcp_plugin;
}


static int listener_set_port(struct listener *listener, char *port)
{
	long int	res;

	res = strtol(port, NULL, 10);

	if ( res <= 0 || res > 0xffff )
		return -1;

	listener->service = port;
	listener->port = (uint16_t) res;

	return 0;
}


static void display_tip(void)
{
	fprintf(stderr, "Type 'quit' to close the connection\n");
}


static int command(const struct plugin * const plugin, int fd,
		struct sockaddr *destaddr, socklen_t addrlen)
{
	ssize_t	len;
	int	llen;
	char	buf[CMD_BUFFER_LEN];
	char	*cmd;

	display_tip();

	while ( 1 )
	{
		fputs(">> ", stdout);

		if ( fgets(buf, sizeof(buf), stdin) == NULL )
		{
			fprintf(stderr, "fgets: error occured\n");
			return -1;
		}

		len = strnlen(buf, sizeof(buf));
		cmd = trim(buf, &len);

		if ( ! len )
			continue;

		if ( strcmp(cmd, "quit") == 0 )
			return 0;

		if ( plugin->send(fd, cmd, len, MSG_DONTWAIT, destaddr, addrlen) != len )
		{
			fprintf(stderr, "write: unable to send\n");
			return 0;
		}

		llen = sizeof(buf);
		while ( llen == sizeof(buf) &&
				(llen = plugin->recv(fd, buf, sizeof(buf), 0, destaddr, &addrlen)) )
		{
			if ( llen < 0 )
			{
				if ( errno == EAGAIN || errno == EWOULDBLOCK )
					break;
				fprintf(stderr, "read: unable to recv\n");
				return 0;
			}

			printf("%.*s", llen, buf);
		}
	}

	return 0;
}


static int check_peer(const struct listener *listener, const struct sockaddr *peer, socklen_t len)
{
	char			host[NI_MAXHOST],
				service[NI_MAXSERV];

	if ( listener->peer == NULL )
		return 1;

	if ( getnameinfo(peer, len, host, sizeof(host), service, sizeof(service), NI_NUMERICHOST) != 0 )
	{
		perror("getnameinfo");
		return 0;
	}

	if ( strcasecmp(host, listener->peer) == 0 )
		return 1;

	fprintf(stderr, "Peer %s mismatched (not %s).\n", host, listener->peer);

	return 0;
}


static const char *get_ip_str(const struct sockaddr *saddr)
{
	static char	buff[sizeof("[fe80:1212:290:1aff:fea3:1f5c]:65535")];
	char		ip[INET6_ADDRSTRLEN];
	const char	*ptr;
	socklen_t	len;
	const char	*fmt;

	switch (saddr->sa_family)
	{
		case AF_INET:
		len = sizeof(struct sockaddr_in);
		fmt = "%s:%d";
		break;

		case AF_INET6:
		len = sizeof(struct sockaddr_in6);
		fmt = "[%s]:%d";
		break;

		default:
		return "";
	}

	if ( (ptr = inet_ntop(saddr->sa_family, (const void*) saddr->sa_data + 2, ip, len)) == NULL )
		return "";

	memset(buff, 0, sizeof(buff));
	snprintf(buff, sizeof(buff), fmt, ptr, ntohs(((const struct sockaddr_in *)saddr)->sin_port));

	return buff;
}


static void serve(const struct listener *listener)
{
	int			sockfd,
				client;
	struct addrinfo		hints = {0};
	struct sockaddr_storage	storage;
	struct sockaddr		*peeraddr = (struct sockaddr *) &storage;
	socklen_t		len = sizeof(struct sockaddr_storage);

	hints.ai_family = listener->family;

	if ( (sockfd = listener->plugin->listen(&hints, listener->host, listener->service)) < 0 )
		return;

	fprintf(stderr, "Waiting for connection...\n");

	while ( 1 )
	{
		if ( (client = listener->plugin->accept(sockfd, peeraddr, &len)) < 0 )
		{
			perror("accept");
			exit(errno);
		}
	
		if ( check_peer(listener, peeraddr, len) )
			break;
	}

	fprintf(stderr, "\nNew connection from %s!\n", get_ip_str(peeraddr));

	while ( command(listener->plugin, client, peeraddr, len) )
		;

	close(client);
	close(sockfd);
}


int main(int argc, char *argv[])
{
	struct listener	listener;
	int	c;

	listener_init(&listener);

	while ( (c = getopt(argc, argv, "46b:huv")) != -1 )
	{
		switch ( c )
		{
			case '4':
			listener.family = AF_INET;
			break;

			case '6':
			listener.family = AF_INET6;
			break;

			case 'b':
			listener.backlog = atoi(optarg);
			break;

			case 'h':
			usage();
			return 0;

			case 'u':
			listener.plugin = &plain_udp_plugin;
			break;

			case 'v':
			version();
			return 0;
		}
	}

	argv += optind;
	argc -= optind;

	if ( argc < 2 || argc > 3 )
	{
		usage();
		return -1;
	}

	if ( argc == 3 )
		listener.peer = argv[2];

	listener.host = argv[0];
	if ( listener_set_port(&listener, argv[1]) )
	{
		fprintf(stderr, "Invalid port %s\n", argv[1]);
		return -1;
	}

	serve(&listener);

	return 0;
}
