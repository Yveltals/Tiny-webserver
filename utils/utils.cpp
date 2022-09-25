#include "../utils/utils.h"

int* signal_::pipefd = 0;

signal_::signal_(int *pipe)
{
    signal_::pipefd = pipe;
    setnonblocking(pipefd[1]); //设置管道写端为非阻塞
    addsig(SIGPIPE, SIG_IGN);
    addsig(SIGALRM, sig_handler, false);
    addsig(SIGTERM, sig_handler, false);
    alarm(TIMESLOT);
}
signal_::~signal_()
{
	close(pipefd[1]);
    close(pipefd[0]);
}

void signal_::addsig(int sig, void(handler)(int), bool restart)
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    if (restart)
        sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}

void signal_::sig_handler(int sig)
{
    //为保证函数的可重入性，保留原来的errno
    int save_errno = errno;
    int msg = sig;
    send(pipefd[1], (char *)&msg, 1, 0);
    errno = save_errno;
}

bool signal_::dealsignal(int fd, bool &timeout, bool &stop_server)
{
    int ret = 0;
    char signals[1024];
    ret = recv(fd, signals, sizeof(signals), 0);
    if (ret <= 0)
    {
        return false;
    }
    else
    {
        for (int i = 0; i < ret; ++i)
        {
            switch (signals[i])
            {
                case SIGALRM: {
                    timeout = true; break;
                }
                case SIGTERM: {
                    stop_server = true; break;
                }
            }
        }
    }
    return true;
}

/******************************************************/

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