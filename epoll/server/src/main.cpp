#include "../include/epoll_server.h"

int main(int argc, char* argv[])
{
	DEBUG(__func__ << " pid = " << getpid());
	if(2 != argc)
	{
		std::cerr << "Usage : " << argv[0] << " [port]" << std::endl;
		exit (EXIT_FAILURE);
	}

	char* port = argv[1]; 
	DEBUG("port = " << port);

	//创建红黑树,返回给全局 root_efd
	root_efd = epoll_create(MAX_EVENTS + 1);  
	if(root_efd < 0)
	{
		std::cout << "create efd in " << __func__ << " error = " << strerror(errno) << std::endl;
	}

	DEBUG("root_efd = " << root_efd);

	//初始化监听socket
	initListenSocket(root_efd, port);

	//定义这个结构体数组，用来接收epoll_wait传出的满足监听事件的fd结构体
	struct epoll_event events[MAX_EVENTS +1];

	int checkpos = 0;
	while(1)
	{
		/*
		long now = time(NULL);
		for(int i = 0; i < 100; ++i)
		{
			if(MAX_EVENTS == checkpos)
				checkpos = 0;
			if(1 != root_events[checkpos].status)
				continue;
			long duration = now - root_events[checkpos].last_active;
			if(duration >= 60)
			{
				close(root_events[checkpos].fd);
				printf("[fd=%d] timeout\n", root_events[checkpos].fd);
				eventdel(root_efd, &root_events[checkpos]);
			}
		}
		*/

		//调用eppoll_wait等待接入的客户端事件,epoll_wait传出的是满足监听条件的那些fd的struct epoll_event类型
		int nfd = epoll_wait(root_efd, events, MAX_EVENTS+1, 1000);
		if(nfd < 0)
		{
			std::cout << "epoll_wait error, exit."<< std::endl;
			exit(EXIT_FAILURE);
		}

		DEBUG("epoll_wait return nfd = " << nfd);

		for(int i = 0; i < nfd; ++i)
		{
				//evtAdd()函数中，添加到监听树中监听事件的时候将myevents_t结构体类型给了ptr指针
				//这里epoll_wait返回的时候，同样会返回对应fd的myevents_t类型的指针
				myEvent *ev = (myEvent*)events[i].data.ptr;

				if(root_efd == events[i].data.fd)
				{
					ev->call_back(ev->fd, events[i].events, ev->argv);
					continue;
				}

				//如果监听的是读事件，并返回的是读事件
				if((events[i].events & EPOLLIN) && (ev->events & EPOLLIN))
				{
					ev->call_back(ev->fd, events[i].events, ev->argv);
				}

				//如果监听的是写事件，并返回的是写事件
				if((events[i].events & EPOLLOUT) && (ev->events & EPOLLOUT))
				{
					ev->call_back(ev->fd, events[i].events, ev->argv);
				}
		}
	}

	return 0;
}

