#include "httpconn.h"

std::atomic<int> HttpConn::userCount;

HttpConn::HttpConn() { 
    epollFd_ = -1;
    fd_ = -1;
    addr_ = { 0 };
    isClose_ = true;
};

HttpConn::~HttpConn() { 
    Close(); 
};

void HttpConn::init(int epollfd, int fd, const sockaddr_in& addr) {
    assert(fd > 0);
    userCount++;
    addr_ = addr;
    epollFd_ = epollfd;
    fd_ = fd;
    isClose_ = false;
    init();
}

void HttpConn::init()
{
    read_idx = 0;
    write_idx = 0;
    bytes_to_send = 0;
    bytes_sended = 0;
    memset(read_buf, '\0', 2048);
    memset(write_buf, '\0', 2048);
}
void HttpConn::Close() {
    UnmapFile();
    if(isClose_ == false){
        isClose_ = true;
        close(fd_);
        userCount--;
        printf("Client[%d](%s:%d) quit, UserCount:%d\n",fd_,inet_ntoa(addr_.sin_addr),addr_.sin_port,(int)userCount);
    }
}

void HttpConn::UnmapFile() {
    if(mmFile_) {
        munmap(mmFile_, mmFileStat_.st_size);
        mmFile_ = nullptr;
    }else{
        puts("mmFile is nullptr");
    }
}


bool HttpConn::read(){
    if(read_idx > 2048) 
    {
        return false;
    }
    int bytes_read = 0;
    while (true)
    {
        bytes_read = recv(fd_, read_buf+read_idx, 2048-read_idx, 0);
        if (bytes_read == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            return false;
        }
        else if (bytes_read == 0)
        {
            return false;
        }
        read_idx += bytes_read;
    }
    // puts(read_buf);
    return true;
}

bool HttpConn::write(){
    int temp = 0, newadd = 0;
    if (bytes_to_send == 0)
    {
        fprintf(stderr, "bytes to send = 0 -> EPOLLIN\n");
        // modfd(epollFd_, fd_, EPOLLIN);
        epoll_event event;
        event.data.fd = fd_;
        event.events = EPOLLIN | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
        epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd_, &event);
        init();
        return true;
    }
    while (1)
    {
        temp = writev(fd_, iov_, iovCnt_);
        if(temp >= 0)
        {
            bytes_sended += temp;
            newadd = bytes_sended - write_idx;
        }
        else
        {   
            // 大文件传输缓冲区满导致,更新iovec结构体的指针和长度,并注册写事件
            if (errno == EAGAIN)
            {
                if (bytes_sended >= iov_[0].iov_len) {
                    iov_[0].iov_len = 0;
                    iov_[1].iov_base = mmFile_ + newadd;
                    iov_[1].iov_len = bytes_to_send;
                }
                else {
                    iov_[0].iov_base = write_buf + bytes_sended;
                    iov_[0].iov_len = iov_[0].iov_len - bytes_sended;
                }
                // modfd(epollFd_, fd_, EPOLLOUT);
                epoll_event event;
                event.data.fd = fd_;
                event.events = EPOLLOUT | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
                epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd_, &event);
                return true;
            }
            fprintf(stderr,"writev error\n");
            UnmapFile();
            return false;
        }
        bytes_to_send -= temp;
    
        if (bytes_to_send <= 0)
        {
            UnmapFile();
            // modfd(epollFd_, fd_, EPOLLIN);
            epoll_event event;
            event.data.fd = fd_;
            event.events = EPOLLIN | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
            epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd_, &event);
            init();
            return true; // TODO浏览器的请求为长连接
        }
    }

    return true;
}

bool HttpConn::dealfile(){
    char *p, *space=(char*)" ";
    p = strtok(read_buf,space);
    strcpy(methed,p);
    p = strtok(NULL,space);
    strcpy(file_path,(char*)"static");
    strcat(file_path,p); 
    // printf("file_path: %s\n",file_path);
    if(strlen(file_path)==7)
    {
        strcat(file_path,(char*)"index.html");
    }
    return true;
}
bool HttpConn::dealuser(){
    int i=0;
    char *name,*pwd;
    for(;i<read_idx-3;++i){
        if(read_buf[i]=='\r' && read_buf[i+1]=='\n' && 
            read_buf[i+2]=='\r' && read_buf[i+3]=='\n')
        {
            i+=4; break;
        }
    }
    if(i>=read_idx-3){
        fprintf(stderr, "POST request anaylise failed\n");
        return false;
    }
    // printf("%s\n",&read_buf[i]);
    name = &read_buf[i+5];
    while(i<read_idx) {
        if(read_buf[i++]=='&') {
            read_buf[i-1]=0; break;
        }
    }
    pwd = &read_buf[i+9];
    strcpy(user_name, name);
    strcpy(user_pwd, pwd);
    // printf(">>> %s %s\n",user_name,user_pwd);
    //check
    MYSQL* sql;
    SqlConnRAII(&sql, SqlConnPool::Instance());
    char order[256] = { 0 };
    MYSQL_RES *res = nullptr;
    snprintf(order, 256, "SELECT username, password FROM user WHERE username='%s'", user_name);
    // printf("%s\n", order);

    if(mysql_query(sql, order)) { 
        mysql_free_result(res); 
        puts("query failed");
        return false; 
    }
    res = mysql_store_result(sql);
    MYSQL_ROW row = mysql_fetch_row(res);

    if(!strcmp(file_path,(char*)"static/REG"))
    {
        if(!row && strlen(user_pwd)>0)
        {
            bzero(order, 256);
            snprintf(order,256,"INSERT INTO user(username, password) VALUES('%s','%s')",user_name,user_pwd);
            printf("%s\n", order);
            if(mysql_query(sql, order)) { 
                puts( "Insert error!");
            }
            strcpy(file_path,(char*)"static/log.html");
        }
        else{
            strcpy(file_path,(char*)"static/registerError.html");
        }
        mysql_free_result(res);
    }
    else if(!strcmp(file_path,(char*)"static/LOG"))
    {
        if(row && !strcmp(row[1],user_pwd))
            strcpy(file_path,(char*)"static/index.html");
        else 
            strcpy(file_path,(char*)"static/logError.html");
        mysql_free_result(res);
    }
    else
    {
        fprintf(stderr, "POST is not login or register\n");
        return false;
    }
    return true;
}

bool HttpConn::process()
{
    dealfile();
    
    if(!strcmp(methed,(char*)"POST"))
    {
        if(!dealuser()) return false;
    }
    /****** file ********/
    if (stat(file_path, &mmFileStat_) < 0)  //TODO
    {
        fprintf(stderr, "file errno is: %d\n", errno);
        return false;
    }
    int fd = open(file_path, O_RDONLY);
    mmFile_ = (char*)mmap(0, mmFileStat_.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);

    /****** response ******/
    char response[]="HTTP/1.1 200 OK\r\nServer:Linux Web Server\r\nContent-length:";
    char type[] = "\r\nContent-type:text/html\r\n\r\n";
    sprintf(write_buf,"%s%ld%s",response,mmFileStat_.st_size,type);
    write_idx = strlen(write_buf);

    iov_[0].iov_base = write_buf;
    iov_[0].iov_len = write_idx;
    iov_[1].iov_base = mmFile_;
    iov_[1].iov_len = mmFileStat_.st_size;
    iovCnt_ = 2;
    bytes_to_send = write_idx + mmFileStat_.st_size;

    // modfd(epollFd_, fd_, EPOLLOUT);
    epoll_event event;
    event.data.fd = fd_;
    event.events = EPOLLOUT | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
    epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd_, &event);
    return true;
}


void modfd(int epollfd, int fd, int ev)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}
