#include "utils.h"
#include "threadpool.h"
#include "timer.h"

int main(int argc, char *argv[])
{
	int listenfd, connfd, epollfd;
	struct sockaddr_in serv_adr, clnt_adr;
	int clnt_adr_size, flag=1;
	char buf[BUF_SIZE];
	pthread_t t_id;

	int pipefd[2];
	int ret = socketpair(PF_UNIX, SOCK_STREAM, 0, (int*)&pipefd);
    assert(ret != -1);
	signal_ *sig = new signal_(pipefd);

	threadpool* pool = new threadpool();
	connection* timer_queue = new connection();
	std::vector<http_conn> users = std::vector<http_conn>(MAX_FD);
	std::vector<timer> timers = std::vector<timer>(MAX_FD);

	listenfd=socket(PF_INET, SOCK_STREAM, 0);
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family=AF_INET;
	serv_adr.sin_addr.s_addr=htonl(INADDR_ANY);
	serv_adr.sin_port = htons(atoi(argv[1]));
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
	Bind(listenfd, (struct sockaddr*)&serv_adr, sizeof(serv_adr));
	Listen(listenfd, 20);

	epoll_event events[MAX_EVENT_NUM];
    epollfd = epoll_create(5);
	addfd(epollfd,listenfd);
    addfd(epollfd, signal_::pipefd[0]); //设置管道读端为ET非阻塞

	bool timeout = false;
    bool stop_server = false;

	while(!stop_server)
	{
		int number = Epoll_wait(epollfd, events, MAX_EVENT_NUM, -1);
		for(int i=0;i<number;++i){
			int sockfd = events[i].data.fd;
			// printf(">>> sockfd = %d\n",sockfd);
			if (sockfd == listenfd)
            {
			    socklen_t clnt_len = sizeof(clnt_adr);
				int connfd = Accept(listenfd,(struct sockaddr*)&clnt_adr,&clnt_len);
				//Todo: connect counts < n
				printf("Connection Request[%d] : %s:%d\n",connfd,inet_ntoa(clnt_adr.sin_addr), ntohs(clnt_adr.sin_port));
				users[connfd].init(connfd,epollfd);
				timers[connfd].init(connfd,epollfd);
				addfd(epollfd,connfd,true);
				timer_queue->add((timer*)&timers[connfd]);
			}
            else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
            {
				removefd(epollfd,sockfd);
            }
			else if ((sockfd == signal_::pipefd[0]) && (events[i].events & EPOLLIN))
            {
                bool flag = sig->dealsignal(sockfd, timeout, stop_server);
                if (false == flag)
                    fprintf(stderr,"dealclientdata failure\n");
            }
            else if (events[i].events & EPOLLIN)
            {
            	// printf("connfd read!\n");
				pool->append(&users[sockfd], false);
            }
            else if (events[i].events & EPOLLOUT)
            {
				// printf("connfd write!\n");
				pool->append(&users[sockfd], true);
            }
			
		}
		if(timeout)
		{
			printf("tick~\n"); 
			timer_queue->tick();
			alarm(TIMESLOT);
			timeout = false;
		}
	}
	close(listenfd);
	delete(sig);
	delete(pool);
	delete(timer_queue);
	return 0;
}


