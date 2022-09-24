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
    bool read();
    bool write();
    bool process();
    void unmap();
    int epollfd;
    int sockfd;
    bool rw;     // 0读 1写
    int read_idx;
    int write_idx;
    char read_buf[2048];
    
    char write_buf[2048];
    struct iovec iv[2];
    int iv_count;
    int bytes_to_send;
    int bytes_sended;

    char methed[20];
    char* file_adr;
    char file_path[50];
    struct stat file_stat;

};
