#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <strings.h>

#include "plain_tcp.h"
#include "tcpudp.h"


static int plain_tcp_connect(struct addrinfo *hints, const char *host, const char *service)
{
	hints->ai_socktype = SOCK_STREAM;

	return tcpudp_get_socket(hints, host, service, connect);
}


static int plain_tcp_bind(int socketfd, const struct sockaddr *sockaddr, socklen_t len)
{
	int		opt = 1;

	if ( setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof(opt)) < 0 )
		return -1;

	return bind(socketfd, sockaddr, len);
}


static int plain_tcp_listen(struct addrinfo *hints, const char *host, const char *service)
{
	int	socketfd,
		backlog = 1;

	hints->ai_socktype = SOCK_STREAM;
	hints->ai_flags = AI_PASSIVE;

	if ( strcasecmp(host, "any") == 0 )
		host = NULL;

	if ( (socketfd = tcpudp_get_socket(hints, host, service, plain_tcp_bind)) < 0 )
		return socketfd;

	if ( listen(socketfd, backlog) < 0 )
	{
		perror("listen");
		return -1;
	}

	return socketfd;
}


static ssize_t plain_tcp_send(int sockfd, const void *buf, size_t len, int flags,
	const struct sockaddr *destaddr, socklen_t addrlen)
{
	(void)destaddr;
	(void)addrlen;

	return send(sockfd, buf, len, flags);
}


static ssize_t plain_tcp_recv(int sockfd, void *buf, size_t len, int flags,
	struct sockaddr *srcaddr, socklen_t *addrlen)
{
	(void)srcaddr;
	(void)addrlen;

	return recv(sockfd, buf, len, flags);
}


struct plugin plain_tcp_plugin =
{
	.connect	=	plain_tcp_connect,
	.listen		=	plain_tcp_listen,
	.accept		=	accept,
	.send		=	plain_tcp_send,
	.recv		=	plain_tcp_recv,
};
