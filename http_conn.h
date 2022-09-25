#include <bits/stdc++.h>
#include <sys/socket.h>
#include <sys/stat.h> 
#include <sys/mman.h>
#include <sys/uio.h>

class http_conn{
public:
    http_conn(){};
    ~http_conn(){};
    
    void init();
    void init(int sockfd, int epollfd);
    // 从socket读到read_buf
    bool read();
    // 调用writev从iv缓冲区发送
    bool write();
    // 解析请求，将响应准备至缓冲区
    bool process();
    bool dealfile();
    bool dealuser();
    void unmap();
    int epollfd;
    int sockfd;
    bool rw;     // 0读 1写

    int read_idx;
    int write_idx;
    char read_buf[2048];
    char write_buf[2048];
    struct iovec iv[2]; //writev缓冲区：iv[0]响应,iv[1]文件
    int iv_count;
    int bytes_to_send;
    int bytes_sended;

    char methed[20];
    char* file_adr; //内存映射地址
    char file_path[50]; //文件请求路径
    struct stat file_stat; //记录文件信息

    char user_name[50];
    char user_pwd[50];

};
