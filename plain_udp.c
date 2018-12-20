#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "plain_udp.h"
#include "tcpudp.h"

#define PLAIN_UDP_MAGIC		"!!"


static ssize_t plain_udp_send(int sockfd, const void *buf, size_t len, int flags,
		const struct sockaddr *destaddr, socklen_t addrlen)
{
	return sendto(sockfd, buf, len, flags, destaddr, addrlen);
}


static ssize_t plain_udp_recv(int sockfd, void *buf, size_t len, int flags,
	struct sockaddr *srcaddr, socklen_t *addrlen)
{
	return recvfrom(sockfd, buf, len, flags, srcaddr, addrlen);
}


static int plain_udp_connect(struct addrinfo *hints, const char *host, const char *service)
{
	int	sockfd;
	char	buf[sizeof(PLAIN_UDP_MAGIC)];

	hints->ai_socktype = SOCK_DGRAM;

	if ( (sockfd = tcpudp_get_socket(hints, host, service, connect)) < 0 )
		return sockfd;

	memcpy(buf, PLAIN_UDP_MAGIC, sizeof(buf));

	/* We need to send a dummy packet in order to be known by the peer.
	 */
	if ( send(sockfd, buf, sizeof(buf), 0) != sizeof(buf) )
	{
		close(sockfd);
		return -1;
	}

	return sockfd;
}


static int plain_udp_listen(struct addrinfo *hints, const char *host, const char *service)
{
	hints->ai_socktype = SOCK_DGRAM;

	if ( strcasecmp(host, "any") == 0 )
		host = NULL;

	return tcpudp_get_socket(hints, host, service, bind);
}


static int plain_udp_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
	char	buf[sizeof(PLAIN_UDP_MAGIC)];

	/* Retrieve the dummy packet send from the peer
	 */
	if ( recvfrom(sockfd, buf, sizeof(buf), 0, addr, addrlen) != sizeof(buf) )
		return -1;

	if ( memcmp(buf, PLAIN_UDP_MAGIC, sizeof(PLAIN_UDP_MAGIC)) != 0 )
		return -1;

	return sockfd;
}


struct plugin plain_udp_plugin =
{
	.connect	=	plain_udp_connect,
	.listen		=	plain_udp_listen,
	.accept		=	plain_udp_accept,
	.send		=	plain_udp_send,
	.recv		=	plain_udp_recv,
};
