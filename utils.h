#include <bits/stdc++.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <sys/epoll.h>

#define EPOLLOUT EPOLLOUT

//ET模式必须设置非阻塞
int setnonblocking(int fd);
//将内核事件表注册读事件,开启EPOLLONESHOT和ET
void addfd(int epollfd, int fd, bool oneshot=false);
//从内核时间表删除描述符
void removefd(int epollfd, int fd);
//将事件重置为EPOLLONESHOT
void modfd(int epollfd, int fd, int ev);

void Bind(int fd, const sockaddr *addr, socklen_t len);
void Listen(int fd, int n);
int Accept(int fd, sockaddr* addr, socklen_t* addr_len);
int Epoll_wait(int epfd, epoll_event *events, int maxevents, int timeout);