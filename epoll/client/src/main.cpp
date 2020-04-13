#include <iostream>
#include "../include/epoll_client.h"

int main(int argc, char **argv)
{
	char * servInetAddr = "127.0.0.1";
	int servPort = 6888;
	char buf[MAXLINE];
	int connfd;
	struct sockaddr_in servaddr;

	if (argc == 2) {
		servInetAddr = argv[1];
	}
	if (argc == 3) {
		servInetAddr = argv[1];
		servPort = atoi(argv[2]);
	}
	if (argc > 3) {
		printf("usage: echoclient <IPaddress> <Port>\n");
		return -1;
	}

	connfd = socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(servPort);
	inet_pton(AF_INET, servInetAddr, &servaddr.sin_addr);

	if (connect(connfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
		perror("connect error");
		return -1;
	}
	printf("welcome to TestLink\n");
	handle(connfd);     /* do it all */
	close(connfd);
	printf("exit\n");
	exit(0);
}
