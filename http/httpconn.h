#ifndef HTTP_CONN_H
#define HTTP_CONN_H

#include <sys/types.h>
#include <sys/uio.h>     // readv/writev
#include <arpa/inet.h>   // sockaddr_in
#include <stdlib.h>      // atoi()
#include <errno.h>
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
    // ssize_t read(int* saveErrno);
    // ssize_t write(int* saveErrno);
    void Close();
    
    bool read();
    bool write();
    bool process();
    bool dealfile();
    bool dealuser();
    void modfd(int, int, int);
    int ToWriteBytes() { 
        return iov_[0].iov_len + iov_[1].iov_len; 
    }
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
    int bytes_to_send;
    int bytes_sended;

    char methed[20];
    // char* file_adr; //内存映射地址
    char file_path[50]; //文件请求路径
    char* mmFile_; 
    struct stat mmFileStat_; //记录文件信息

    char user_name[50];
    char user_pwd[50];
};


#endif //HTTP_CONN_H