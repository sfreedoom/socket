#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <string.h>
#include <iostream>
#include <map>
 
#define MAX_EVENTS 1024 	//监听上限 
#define BUFLEN 4096		//缓存区大小

#define DEBUGFLAG 0
#define DEBUG(str){if(DEBUGFLAG) {std::cout << str << std::endl;}}

typedef struct myEvent{
	int fd;				//要监听的文件描述符	
	int events; 			//对应的监听事件，EPOLLIN和EPLLOUT
	void* argv;			//指向自己结构体指针
	void(*call_back)(int fd, int events, void* argv); //回调函数
	int status;			//是否在监听:1->在红黑树上(监听), 0->不在(不监听)
	char buf[BUFLEN];	
	int len;
	long last_active;	//记录每次加入红黑树 g_efd 的时间值
}EP_Event;

extern volatile int root_efd; //全局变量，作为红黑树根
extern std::map<int, EP_Event*> my_event_map;
 
//设置socket为非阻塞的
int make_socket_non_blocking (int sfd);

/*读取客户端发过来的数据的函数*/
void recvdata(int fd, int events, void* argv);

/*发送给客户端数据*/
void senddata(int fd, int events, void* argv);

/*
 * 封装一个自定义事件，包括fd，这个fd的回调函数，还有一个额外的参数项
 * 注意：在封装这个事件的时候，为这个事件指明了回调函数，一般来说，一个fd只对一个特定的事件
 * 感兴趣，当这个事件发生的时候，就调用这个回调函数
 */
//void eventSet(EP_Event* ev, int fd, void(*call_back)(int fd, int event, void* argv), void* argv);
void eventSet(int fd, void(*call_back)(int fd, int event, void* argv));

/* 向 epoll监听的红黑树 添加一个文件描述符 */
void eventAdd(int fd, int efd, int event);

/* 从epoll 监听的 红黑树中删除一个文件描述符*/
void eventDel(int epfd, EP_Event* ev);

void eventMod(int epfd, EP_Event* ev);

/*  当有文件描述符就绪, epoll返回, 调用该函数与客户端建立链接 */
void acceptConn(int lfd, int events, void* argv);

/*创建 socket, 初始化fd */
int initListenSocket(int efd, const char* str_port);

inline int itoa(int number, char* buffer, int length)
{
	char ltoa_str[60+1];
	int aLength = -1;
	sprintf(ltoa_str, "%ld", number);

	DEBUG("itoa ltoa_str = " << ltoa_str);

	if((aLength = strlen(ltoa_str)) >= length)
	{
		length = -1;
	}
	else
	{
		buffer = ltoa_str;
		length = aLength;
		DEBUG("itoa buffer = " << buffer);
	}

	DEBUG("itoa length == " << length);

	return length;
}
