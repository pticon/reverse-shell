#ifndef __TCPUDP_H__
#define __TCPUDP_H__

static inline int tcpudp_get_socket(struct addrinfo *hints, const char *host,
	const char *service, int (*cb)(int, const struct sockaddr *, socklen_t))
{
	int		ret,
			socketfd;
	struct addrinfo	*result,
			*rp;

	if ( (ret = getaddrinfo(host, service, hints, &result)) != 0 )
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
		return -1;
	}

	/* getaddrinfo() returns a list of address structures.
	 * Try each address until we successfully bind().
	 */
	for ( rp = result ; rp != NULL ; rp = rp->ai_next )
	{
		if ( (socketfd = socket(rp->ai_family, hints->ai_socktype, 0)) < 0 )
			continue;

		if ( cb(socketfd, rp->ai_addr, rp->ai_addrlen) == 0 )
			break;

		close(socketfd);
	}

	freeaddrinfo(result);

	if ( rp == NULL )
	{
		fprintf(stderr, "Could not bind any socket.\n");
		return -1;
	}

	return socketfd;
}

#endif /* __TCPUDP_H__ */
