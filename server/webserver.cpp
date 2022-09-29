#include "webserver.h"
using namespace std;

WebServer::WebServer(): port_(9000), timeoutMS_(60000), isClose_(false), 
    timer_(new HeapTimer()), threadpool_(new ThreadPool(8)), epoller_(new Epoller())
{
    HttpConn::userCount = 0;
    SqlConnPool::Instance()->Init();

    listenEvent_ = EPOLLRDHUP | EPOLLET;
    connEvent_ = EPOLLRDHUP | EPOLLET | EPOLLONESHOT;

    if(!InitSocket_()) { isClose_ = true;}
}

WebServer::~WebServer() {
    close(listenFd_);
    isClose_ = true;
    free(srcDir_);
    SqlConnPool::Instance()->ClosePool();
}

/* Create listenFd */
bool WebServer::InitSocket_() {
    int ret;
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port_);
    //TODO 优雅关闭

    listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if(listenFd_ < 0) {
        puts("Create socket error!");
        return false;
    }

    int optval = 1;
    /* 端口复用 只有最后一个套接字会正常接收数据 */
    ret = setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int));
    if(ret == -1) {
        puts("set socket setsockopt error !");
        close(listenFd_);
        return false;
    }

    ret = bind(listenFd_, (struct sockaddr *)&addr, sizeof(addr));
    if(ret < 0) {
        puts("Bind Port:%d error!");
        close(listenFd_);
        return false;
    }

    ret = listen(listenFd_, 6);
    if(ret < 0) {
        puts("Listen port:%d error!");
        close(listenFd_);
        return false;
    }
    ret = epoller_->AddFd(listenFd_,  listenEvent_ | EPOLLIN);
    if(ret == 0) {
        puts("Add listen error!");
        close(listenFd_);
        return false;
    }
    SetFdNonblock(listenFd_);
    return true;
}

int WebServer::SetFdNonblock(int fd) {
    assert(fd > 0);
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
}

void WebServer::Start() {
    int timeMS = -1;  /* epoll wait timeout == -1 无事件将阻塞 */
    if(!isClose_) { puts("========== Server start =========="); }
    while(!isClose_) {
        if(timeoutMS_ > 0) {
            timeMS = timer_->GetNextTick();
        }
        int eventCnt = epoller_->Wait(timeMS);
        for(int i = 0; i < eventCnt; i++) 
        {
            int fd = epoller_->GetEventFd(i);
            uint32_t events = epoller_->GetEvents(i);
            if(fd == listenFd_) {
                DealListen_();
            }
            else if(events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                assert(users_.count(fd) > 0);
                CloseConn_(&users_[fd]);
            }
            else if(events & EPOLLIN) {
                assert(users_.count(fd) > 0);
                threadpool_->AddTask(std::bind(&WebServer::OnRead_, this, &users_[fd]));
            }
            else if(events & EPOLLOUT) {
                assert(users_.count(fd) > 0);
                threadpool_->AddTask(std::bind(&WebServer::OnWrite_, this, &users_[fd]));
            } else {
                puts("Unexpected event");
            }
        }
    }
}

void WebServer::DealListen_() {
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
   do {
    int fd = accept(listenFd_, (struct sockaddr *)&addr, &len);
    if(fd <= 0) { return;}
    else if(HttpConn::userCount >= MAX_FD) {
        // SendError_(fd, "Server busy!");
        puts("Clients is full!");
        return;
    }
    users_[fd].init(epoller_->epollFd_ ,fd, addr);
    if(timeoutMS_ > 0) {
        timer_->add(fd, timeoutMS_, std::bind(&WebServer::CloseConn_, this, &users_[fd]));
    }
    epoller_->AddFd(fd, EPOLLIN | connEvent_);
    SetFdNonblock(fd);
    // printf("Client[%d] in!\n", users_[fd].fd_);
   } while(listenEvent_ & EPOLLET);
}

void WebServer::CloseConn_(HttpConn* client) {
    assert(client);
    epoller_->DelFd(client->fd_);
    client->Close();
}

void WebServer::OnRead_(HttpConn* client) {
    bool ret = client->read();
    if(ret){
        if(!client->process()){
            epoll_ctl(client->epollFd_, EPOLL_CTL_DEL, client->fd_, 0);
            close(client->fd_);
        }
    }else{
        puts("read error!");
        epoll_ctl(client->epollFd_, EPOLL_CTL_DEL, client->fd_, 0);
        close(client->fd_);
    }
}

void WebServer::OnWrite_(HttpConn* client) {
    if (!client->write())
    {   //短连接或出错
        puts("close fd_!");
        epoll_ctl(client->epollFd_, EPOLL_CTL_DEL, client->fd_, 0);
        close(client->fd_);
    }else{ 
        //保持长连接
        //用于短连接webbench测试
        // epoll_ctl(client->epollFd_, EPOLL_CTL_DEL, client->fd_, 0);
        // close(client->fd_);
    }
}