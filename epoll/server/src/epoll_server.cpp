#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include "../include/epoll_server.h"

volatile int root_efd;	//全局变量，作为红黑树根
std::map<int, EP_Event*> my_event_map;
 
int setNoBlocking (int sfd)
{
	DEBUG(__func__ << " sfd = " << sfd);
    int flags, s;

    //得到文件状态标志
    flags = fcntl (sfd, F_GETFL, 0);
    if (flags == -1)
    {
		std::cout << __func__ << "set no blocking failed, error: " << strerror(errno) << std::endl;
		//perror ("fcntl");
        return -1;
    }

    //设置文件状态标志
    flags |= O_NONBLOCK;
    s = fcntl (sfd, F_SETFL, flags);
    if (s == -1)
    {
		std::cout << __func__ << "set no blocking failed, error: " << strerror(errno) << std::endl;
		//perror ("fcntl");
        return -1;
    }

    return 0;
}

void eventSet(int fd, void(*call_back)(int fd, int events, void* argv))
{
	DEBUG(__func__ << " fd = " << fd);
	DEBUG(__func__ << " pid = " << getpid());
	//std::map<int, EP_Event>::iterator itr;
	//for(itr = my_event_map.begin(); my_event_map.end() != itr; ++itr)
	if(my_event_map.size() >= MAX_EVENTS)
	{
		std::cerr << "max event number [" << MAX_EVENTS << "] achieved!" << std::endl;
	}

	if(my_event_map.end() == my_event_map.find(fd))
	{
		EP_Event aEv;

		aEv.fd = fd;
		aEv.call_back = call_back;
		aEv.events = 0;
		aEv.status = 0;

		if(aEv.len <= 0)
		{
			memset(aEv.buf, 0, sizeof(aEv.buf));
		}

		aEv.last_active = time(NULL);
		my_event_map.insert(std::pair<int, EP_Event*>(fd, &aEv));

		DEBUG(__func__ << " insert fd[" << fd << "] into event map successful." );
		//let argv point to itself
		std::map<int, EP_Event*>::reverse_iterator iter= my_event_map.rbegin();
		if(my_event_map.rend() != iter)
		{
			iter->second->argv = iter->second;
		}
	}
	else
	{
		DEBUG(__func__ << " fd = " << fd << " is using now.")
	}
}

void eventAdd(int fd, int epfd, int events)
{
	DEBUG(__func__ << " epfd = " << epfd << " events = " << events);
	DEBUG(__func__ << " pid = " << getpid());

	struct epoll_event epv ={0, {0}};
	int op = 0;

	std::map<int, EP_Event*>::iterator iter;
	iter  = my_event_map.find(fd);
	if(my_event_map.end() != iter)
	{
		//EPOLLIN 或 EPOLLOUT
		epv.events = iter->second->events = events;
		//ptr指向一个结构体（之前的epoll模型红黑树上挂载的是文件描述符cfd和lfd，现在是ptr指针)
		epv.data.ptr = iter->second;

		//status 说明文件描述符是否在红黑树上 0不在，1 在
		if(0 == iter->second->status)
		{
			op = EPOLL_CTL_ADD; //将其加入红黑树 g_efd, 并将status置1
			iter->second->status = 1;
		}

		DEBUG(__func__ << " ev->status = " << iter->second->status);
		DEBUG(__func__ << " epfd = " << epfd);
		DEBUG(__func__ << " op = " << op);

		if(epoll_ctl(epfd, op, iter->second->fd, &epv) < 0)
		{
			std::cout << "event add failure. [fd=" << iter->second->fd << "],events[" << events<< "]" << std::endl;
		}
		else
		{
			std::cout << "event add successfully. [fd=" << iter->second->fd << "],events[" << events<< "]" << std::endl;
		}
	}
	else
	{
		DEBUG(__func__ << "no active socket fd");	
	}

	return;
}

void eventDel(int epfd, EP_Event* ev)
{
	DEBUG(__func__ << " epfd = " << epfd );
	struct epoll_event epv = {0, {0}};
	if(1 != ev->status) //如果fd没有添加到监听树上，就不用删除，直接返回
		return;

	//如果fd在监听树上,从map中删除
	std::map<int, EP_Event*>::iterator iter;
	iter  = my_event_map.find(ev->fd);
	my_event_map.erase(iter);

	epv.data.ptr = NULL;
	ev->status = 0;

	epoll_ctl(epfd, EPOLL_CTL_DEL, ev->fd, &epv);
	return;
}

void eventMod(int epfd, EP_Event* ev)
{
	DEBUG(__func__ << " epfd = " << epfd );
	struct epoll_event epv = {0, {0}};
	if(1 != ev->status) //如果fd没有添加到监听树上，就不用删除，直接返回
		return;

	epv.events = EPOLLOUT;
	epv.data.ptr = ev;
	ev->status = 1;
	epoll_ctl(epfd, EPOLL_CTL_MOD, ev->fd, &epv);
	return;
}

void acceptConn(int lfd, int events, void* argv)
{
	DEBUG(__func__ << " lfd = " << lfd << " events = " << events << " root_efd = " << root_efd);
	DEBUG(__func__ << " pid = " << getpid());
	struct sockaddr in_addr;
	socklen_t in_len = sizeof(in_addr);
	int infd, i;

	infd = accept(lfd, (struct sockaddr*)&in_addr, &in_len);
	if(-1 == infd)
	{
		//if((errno == EAGAIN) || (errno == EWOULDBLOCK))
		if(EAGAIN != errno && EINTR != errno)
		{
			sleep(1);
		}
		std::cout << __func__ << " : accept " << strerror(errno) << std::endl;
	}

	do
	{
		//从全局数组root_events中找一个空闲元素，类似于select中找值为-1的元素
		std::map<int, EP_Event*>::iterator itr;
		for(itr = my_event_map.begin(); my_event_map.end() != itr; ++itr)
		{
			DEBUG(__func__ << " map.index[" << itr->first << "]index.status = " << itr->second->status);
			if(0 == itr->second->status)
				break;
		}

		//超出连接数上限
		if(MAX_EVENTS == my_event_map.size())
		{
			std::cout << __func__ << " max connect limit " << MAX_EVENTS << std::endl;
			break;
		}

		char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV] = {0};

		//将地址转化为主机名或者服务名
		int s = getnameinfo (&in_addr, in_len,
				hbuf, sizeof(hbuf),
				sbuf, sizeof(sbuf),
				NI_NUMERICHOST | NI_NUMERICSERV);//flag参数:以数字名返回

		//主机地址和服务地址
		if (s == 0)
		{   
			printf("Accepted connection on descriptor %d "
					"(host=%s, port=%s)\n", infd, hbuf, sbuf);
		}   

		setNoBlocking(infd);

		//找到合适的节点之后，将其添加到监听树中，并监听读事件
		eventSet(infd, recvdata);
		eventAdd(infd, root_efd, (EPOLLIN | EPOLLET));

	}while(0);

	return;
}

void recvdata(int fd, int events, void* argv)
{
	DEBUG(__func__ << " fd = " << fd << " events = " << events);
	EP_Event* ev = (EP_Event*)argv;
	int len = 0;

	//读取客户端发过来的数据
	len = recv(fd, ev->buf, sizeof(ev->buf), 0);
	DEBUG(__func__ << " len = " << len);

	//将该节点从红黑树上摘除
	eventDel(root_efd, ev);
	//
	//eventMod(root_efd, ev);

	if(len > 0)
	{
		ev->len = len;
		ev->buf[len] = '\0';
		std::cout << __func__ << "[" << fd << "]:" << ev->buf << std::endl;

		//设置该fd对应的回调函数为senddata
		eventSet(fd, senddata);

		//将fd加入红黑树g_efd中,监听其写事件
		eventAdd(fd, root_efd, (EPOLLOUT | EPOLLET));
	}
	else if(0 == len)
	{
		close(ev->fd);
		/* ev-g_events 地址相减得到偏移元素位置 */
		//std::cout << "[fd=" << fd << "] pos[" << ev - root_events << "], closed." << std::endl;
	}
	else
	{
		close(ev->fd);
		std::cout << "[fd=" << fd << "] error[" << errno << "]:" << strerror(errno) << std::endl;
	}
	return;
}

void senddata(int fd, int events, void* argv)
{
	DEBUG(__func__ << " fd = " << fd << " events = " << events);
	EP_Event* ev = (EP_Event*)argv;
	int len = 0;

	ev->len = read(0, ev->buf, sizeof(ev->buf));
	ev->buf[ev->len++] = '\0';

	//直接将数据回射给客户端
	len = send(fd, ev->buf, ev->len, 0);
	//len = write(fd, ev->buf, ev->len);

	//从红黑树g_efd中移除
	eventDel(root_efd, ev);

	if(len > 0)
	{
		std::cout << "send [fd=" << fd << "], [length=" << len << "]:" << ev->buf << std::endl;
		//将该fd的回调函数改为recvdata
		eventSet(fd, recvdata);

		//重新添加到红黑树上，设为监听读事件
		eventAdd(fd, root_efd, (EPOLLIN | EPOLLET));
	}
	else
	{
		close(ev->fd);
		std::cout << "send [fd=" << fd << "] error=" << strerror(errno) << std::endl;
	}
	return;
}

int initListenSocket(int epfd, const char* str_port)
{
	DEBUG(__func__ << " epfd = " << epfd << " port = " << str_port);
	DEBUG(__func__ << " pid = " << getpid());
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	int s, lfd = 0;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;     /* Return IPv4 and IPv6 choices */
	hints.ai_socktype = SOCK_STREAM; /* We want a TCP socket */
	hints.ai_flags = AI_PASSIVE;     /* All interfaces */

	s = getaddrinfo(NULL, str_port, &hints, &result);
	if (s != 0)
	{
		fprintf (stderr, "getaddrinfo: %s\n", gai_strerror (s));
		return -1;
	}

	for (rp = result; rp != NULL; rp = rp->ai_next)
	{
		lfd = socket (rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (lfd == -1)
			continue;

		s = bind (lfd, rp->ai_addr, rp->ai_addrlen);
		if (s == 0)
		{
			/* We managed to bind successfully! */
			break;
		}

		close (lfd);
	}

	if (rp == NULL)
	{
		fprintf (stderr, "Could not bind\n");
		return -1;
	}

	freeaddrinfo (result);

	setNoBlocking(lfd);

	listen(lfd, 20);

	eventSet(lfd, acceptConn);

	//将lfd添加到监听树上，监听读事件
	eventAdd(lfd, epfd, (EPOLLIN | EPOLLET));

	return 0;
}
