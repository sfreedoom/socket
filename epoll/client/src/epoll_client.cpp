#include "epoll_client.h"

void handle(int sockfd)
{
	char sendline[MAXLINE], recvline[MAXLINE];
	int n;
	for (;;) {
		if (fgets(sendline, MAXLINE, stdin) == NULL) {
			break;//read eof
		}
		/*
		//也可以不用标准库的缓冲流,直接使用系统函数无缓存操作
		if (read(STDIN_FILENO, sendline, MAXLINE) == 0) {
		break;//read eof
		}
		*/

		n = write(sockfd, sendline, strlen(sendline));
		DEBUG("write done. n= " << n);
		n = read(sockfd, recvline, MAXLINE);
		DEBUG("read done. n = " << n);
		if (n == 0) {
			printf("echoclient: server terminated prematurely\n");
			break;
		}
		write(STDOUT_FILENO, recvline, n);
		//如果用标准库的缓存流输出有时会出现问题
		//fputs(recvline, stdout);
	}
}
