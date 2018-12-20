#ifndef __PLUGIN_H__
#define __PLUGIN_H__

struct addrinfo;

struct plugin
{
	int	(*connect)(struct addrinfo *hints, const char *host, const char *service);
	int	(*listen)(struct addrinfo *hints, const char *host, const char *service);
	int	(*accept)(int sockfd, struct sockaddr *addr, socklen_t *len);
	ssize_t	(*send)(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *destaddr, socklen_t addrlen);
	ssize_t	(*recv)(int sockfd, void *buf, size_t len, int flags, struct sockaddr *srcaddr, socklen_t *addrlen);
};

#endif /* __PLUGIN_H__ */
