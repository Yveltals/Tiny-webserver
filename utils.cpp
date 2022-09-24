#include "utils.h"


void addfd(int epollfd, int fd, bool oneshot)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    if(oneshot) event.events |= EPOLLONESHOT;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

void removefd(int epollfd, int fd)
{
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
    printf("Disconnection\n");
}

void modfd(int epollfd, int fd, int ev)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

int setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

/*******************************************************/

void Bind(int fd, const sockaddr *adr, socklen_t len)
{
    if(bind(fd, adr, len)==-1){
        fprintf(stderr, "bind() error\n"); exit(1);
    }
}
void Listen(int fd, int n)
{
    if(listen(fd, 20)==-1){
		fprintf(stderr, "listen() error\n"); exit(1);
    }
}
int Accept(int fd, sockaddr* addr, socklen_t* addr_len)
{
    int connfd = accept(fd, addr, addr_len);
    if (connfd < 0) {
        fprintf(stderr, "accept errno is:%d", errno); exit(1);
    }
    return connfd;
}
int Epoll_wait(int epfd, epoll_event *events, int maxevents, int timeout)
{
    int number = epoll_wait(epfd, events, maxevents, timeout);
    if (number < 0 && errno != EINTR) {
        fprintf(stderr,"epoll failure\n"); exit(1);
    }
    return number;
}