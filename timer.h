#ifndef TIMER_H
#define TIMER_H
#include "./utils/utils.h"

class timer
{
public:
    timer(){};
    ~timer(){};

    void init();
    void init(int sockfd, int epollfd);
    int sockfd;
    int epollfd;
    time_t expire;
    void (*cb_func)(timer* t);
};
class connection
{
public:
    std::list<timer*> timer_queue;

    connection(){};
    ~connection(){};
    void add(timer* t);
    void tick();
    void update(timer* t);
};


void connection::add(timer* t)
{
    timer_queue.push_back(t);
}
void connection::tick()
{
    time_t cur = time(NULL);
    while (!timer_queue.empty() && timer_queue.front()->expire < cur)
    {
        timer *t = timer_queue.front();
        t->cb_func(t);
        timer_queue.pop_front();

    }
}
void connection::update(timer* t)
{
    time_t cur = time(NULL);
    t->expire = cur + 2 * TIMESLOT;
    for(auto it=timer_queue.begin();it!=timer_queue.end();++it){
        if((*it) == t)
        {
            timer_queue.erase(it);
            break;
        }
    }
    add(t);
    // printf("update ~~\n");
}
// 移除客户端监听，关闭套接字
void remove(timer* t)
{
    epoll_ctl(t->epollfd, EPOLL_CTL_DEL, t->sockfd, 0);
    close(t->sockfd);
    printf("<Timeout sockfd: %d>\n",t->sockfd);
    // http_conn::m_user_count--;
}
void timer::init(int sockfd,int epollfd)
{
    this->sockfd = sockfd;
    this->epollfd = epollfd;
    this->expire = time(NULL) + 3*TIMESLOT;
    this->cb_func = remove;
}

#endif