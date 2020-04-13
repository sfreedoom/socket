#include <unistd.h>
#include <sys/types.h>       /* basic system data types */
#include <sys/socket.h>      /* basic socket definitions */
#include <netinet/in.h>      /* sockaddr_in{} and other Internet defns */
#include <arpa/inet.h>       /* inet(3) functions */
#include <netdb.h> /*gethostbyname function */
#include <iostream>

#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#define MAXLINE 1024
#define FLAG 0
#define DEBUG(str){if(FLAG) {std::cout << str << std::endl;}}

void handle(int connfd);
