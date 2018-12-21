#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>


#include "common.h"
#include "plugin.h"


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


static int check_peer(const struct rshell *rshell, const struct sockaddr *peer, socklen_t len)
{
	char	host[NI_MAXHOST],
		service[NI_MAXSERV];

	if ( rshell->peer == NULL )
		return 1;

	if ( getnameinfo(peer, len, host, sizeof(host), service, sizeof(service), NI_NUMERICHOST) != 0 )
	{
		perror("getnameinfo");
		return 0;
	}

	if ( strcasecmp(host, rshell->peer) == 0 )
		return 1;

	fprintf(stderr, "Peer %s mismatched (not %s).\n", host, rshell->peer);

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


int rshell_serve(const struct rshell *rshell)
{
	int			sockfd,
				client;
	struct addrinfo		hints = {0};
	struct sockaddr_storage	storage;
	struct sockaddr		*peeraddr = (struct sockaddr *) &storage;
	socklen_t		len = sizeof(struct sockaddr_storage);

	hints.ai_family = rshell->family;

	if ( (sockfd = rshell->plugin->listen(&hints, rshell->host, rshell->service)) < 0 )
		return -1;;

	fprintf(stderr, "Waiting for connection...\n");

	while ( 1 )
	{
		if ( (client = rshell->plugin->accept(sockfd, peeraddr, &len)) < 0 )
		{
			perror("accept");
			return errno;
		}
	
		if ( check_peer(rshell, peeraddr, len) )
			break;
	}

	fprintf(stderr, "\nNew connection from %s!\n", get_ip_str(peeraddr));

	while ( command(rshell->plugin, client, peeraddr, len) )
		;

	close(client);
	close(sockfd);

	return 0;
}
