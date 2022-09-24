#include "utils.h"
#include "threadpool.h"

#define MAX_FD 65536
#define BUF_SIZE 1024
#define SMALL_BUF 100
#define MAX_EVENT_NUM 100

int main(int argc, char *argv[])
{
	int listenfd, connfd, epollfd;
	struct sockaddr_in serv_adr, clnt_adr;
	int clnt_adr_size, flag=1;
	char buf[BUF_SIZE];
	pthread_t t_id;

	threadpool* pool = new threadpool();
	std::vector<http_conn> users = std::vector<http_conn>(MAX_FD);

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

	while(true)
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
				addfd(epollfd,connfd,true);
			}
            else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
            {
				removefd(epollfd,sockfd);
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
		
	}
	close(listenfd);
	return 0;
}


