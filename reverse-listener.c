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
#define CMD_BUFFER_LEN	4096
#define _STR(a)		#a
#define STR(a)		_STR(a)

#include "common.h"

struct listener
{
	char		*host;
	char		*service;
	uint16_t	port;
	int		family;
	int		backlog;
};

static void usage(void)
{
	fprintf(stderr, "usage: %s [options] <host> <port>\n", PROGNAME);
	fprintf(stderr, "options:\n");
	fprintf(stderr, "\t-b <backlog>: define the maximum length to which the queue of pending connections may grow (default: " STR(LISTEN_BACKLOG) ")\n");
	fprintf(stderr, "\t-h          : display this and exit\n");
	fprintf(stderr, "\t-v          : display version and exit\n");
	fprintf(stderr, "\t-4          : use IPv4 socket\n");
	fprintf(stderr, "\t-6          : use IPv6 socket\n");
}


static void listener_init(struct listener *listener)
{
	listener->family = AF_UNSPEC;
	listener->backlog = LISTEN_BACKLOG;
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


static int command(int fd)
{
	size_t	len;
	int	llen;
	char	buf[CMD_BUFFER_LEN];

	display_tip();

	while ( 1 )
	{
		fputs(">> ", stdout);

		if ( fgets(buf, sizeof(buf), stdin) == NULL )
		{
			fprintf(stderr, "fgets: error occured\n");
			return -1;
		}

		if ( strcmp(buf, "quit\n") == 0 )
			return 0;

		len = strnlen(buf, sizeof(buf));

		if ( send(fd, buf, len, MSG_DONTWAIT) != len )
		{
			fprintf(stderr, "write: unable to send\n");
			return 0;
		}

		while ( (llen = recv(fd, buf, sizeof(buf), MSG_DONTWAIT)) )
		{
			if ( llen < 0 )
			{
				if ( errno == EAGAIN || errno == EWOULDBLOCK )
					break;
				fprintf(stderr, "read: unable to send\n");
				return 0;
			}

			printf("%.*s", llen, buf);
		}
	}

	return 0;
}


static const char *get_ip_str(const struct sockaddr *saddr)
{
	static char	buff[sizeof("[fe80:1212:290:1aff:fea3:1f5c]:65535")];
	char		ip[INET6_ADDRSTRLEN];
	const char	*ptr;
	socklen_t	len;

	switch (saddr->sa_family)
	{
		case AF_INET:
		len = sizeof(struct sockaddr_in);
		break;

		case AF_INET6:
		len = sizeof(struct sockaddr_in);
		break;

		default:
		return "";
	}

	if ( (ptr = inet_ntop(saddr->sa_family, (const void*) saddr->sa_data + 2, ip, len)) == NULL )
		return "";

	memset(buff, 0, sizeof(buff));
	snprintf(buff, sizeof(buff), "[%s]:%d", ptr, ntohs(((const struct sockaddr_in *)saddr)->sin_port));

	return buff;
}


static void serve(const struct listener *listener)
{
	int			sockfd,
				client,
				opt = 1;
	struct sockaddr_storage	saddr;
	struct sockaddr		*psaddr = (struct sockaddr *) &saddr;
	struct sockaddr_in	*sin4;
	struct sockaddr_in6	*sin6;
	struct sockaddr_storage	storage;
	struct sockaddr		*peeraddr = (struct sockaddr *) &storage;
	socklen_t		len;

	if ( (sockfd = socket(listener->family, SOCK_STREAM, 0)) < 0 )
	{
		perror("socket");
		exit(errno);
	}

	if ( setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof(opt)) < 0 )
	{
			perror("reuseaddr");
			exit(errno);
	}

	memset(&saddr, 0, sizeof(saddr));
	psaddr->sa_family = listener->family;

	switch ( listener->family )
	{
			case AF_INET:
			sin4 = (struct sockaddr_in *) psaddr;
			len = sizeof(struct sockaddr_in);
			sin4->sin_port = htons(listener->port);
			sin4->sin_addr.s_addr = !strcasecmp(listener->host, "any") ?
				htonl(INADDR_ANY) : inet_addr(listener->host);
			break;

			case AF_INET6:
			sin6 = (struct sockaddr_in6 *) psaddr;
			len = sizeof(struct sockaddr_in6);
			sin6->sin6_port = htons(listener->port);
			memset(sin6->sin6_addr.s6_addr, 0, 16);
			break;

			default:
			fprintf(stderr, "Unknown family %d\n", listener->family);
			exit(-1);
	}

	if ( bind(sockfd, psaddr, len) < 0 )
	{
		perror("bind");
		exit(errno);
	}

	if ( listen(sockfd, listener->backlog) < 0 )
	{
		perror("listen");
		exit(errno);
	}

	fprintf(stderr, "Waiting for connection...\n");

	if ( (client = accept(sockfd, peeraddr, &len)) < 0 )
	{
		perror("accept");
		exit(errno);
	}

	fprintf(stderr, "\nNew connection from %s!\n", get_ip_str(peeraddr));

	while ( command(client) )
		;

	close(client);
	close(sockfd);
}


int main(int argc, char *argv[])
{
	struct listener	listener;
	int	c;

	listener_init(&listener);

	while ( (c = getopt(argc, argv, "46b:hv")) != -1 )
	{
		switch ( c )
		{
			case '4':
			listener.family = AF_INET6;
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

	listener.host = argv[0];
	if ( listener_set_port(&listener, argv[1]) )
	{
		fprintf(stderr, "Invalid port %s\n", argv[1]);
		return -1;
	}

	serve(&listener);

	return 0;
}
