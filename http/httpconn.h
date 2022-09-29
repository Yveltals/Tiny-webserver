#ifndef HTTP_CONN_H
#define HTTP_CONN_H

#include <sys/types.h>
#include <sys/uio.h>     // readv/writev
#include <arpa/inet.h>   // sockaddr_in
#include <stdlib.h>      // atoi()
#include <errno.h>
#include <atomic>
#include <string.h> 
#include <sys/epoll.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h> 

#include "../server/epoller.h"
#include "../pool/sqlconnRAII.h"

class HttpConn {
public:
    HttpConn();
    ~HttpConn();
    void init();
    void init(int epollFd ,int sockFd, const sockaddr_in& addr);
    void Close();
    
    bool read();
    bool write();
    bool process();
    bool dealfile();
    bool dealuser();
    void modfd(int, int, int);
    void UnmapFile();
    
    int epollFd_;
    int fd_;
    struct sockaddr_in addr_;
    bool isClose_;
    
    int read_idx;
    int write_idx;
    char read_buf[2048];
    char write_buf[2048];
    int iovCnt_;
    struct iovec iov_[2];
    size_t bytes_to_send;
    size_t bytes_sended;

    char methed[20];
    char file_path[50]; //文件请求路径
    char* mmFile_; 
    struct stat mmFileStat_; //记录文件信息

    char user_name[50];
    char user_pwd[50];
    
    static std::atomic<int> userCount;
};


#endif //HTTP_CONN_H