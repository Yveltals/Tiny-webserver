#include "http_conn.h"
#include "utils.h"

void http_conn::init(int sockfd, int epollfd)
{
    this->sockfd = sockfd;
    this->epollfd = epollfd;
    init();
}
void http_conn::init()
{
    read_idx = 0;
    write_idx = 0;
    bytes_to_send = 0;
    bytes_sended = 0;
    memset(read_buf, '\0', 2048);
    memset(write_buf, '\0', 2048);
}

bool http_conn::read(){
    if(read_idx > 2048) 
    {
        return false;
    }
    int bytes_read = 0;
    while (true)
    {
        bytes_read = recv(sockfd, read_buf+read_idx, 2048-read_idx, 0);
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
    return true;
}

bool http_conn::write(){
    int temp = 0, newadd = 0;
    if (bytes_to_send == 0)
    {
        printf("bytes to send = 0 -> EPOLLIN\n");
        modfd(epollfd, sockfd, EPOLLIN);
        init();
        return true;
    }
    while (1)
    {
        temp = writev(sockfd, iv, iv_count);
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
                if (bytes_sended >= iv[0].iov_len) {
                    iv[0].iov_len = 0;
                    iv[1].iov_base = file_adr + newadd;
                    iv[1].iov_len = bytes_to_send;
                }
                else {
                    iv[0].iov_base = write_buf + bytes_sended;
                    iv[0].iov_len = iv[0].iov_len - bytes_sended;
                }
                modfd(epollfd, sockfd, EPOLLOUT);
                return true;
            }
            fprintf(stderr,"writev error\n");
            unmap();
            return false;
        }
        bytes_to_send -= temp;
    
        if (bytes_to_send <= 0)
        {
            unmap();
            modfd(epollfd, sockfd, EPOLLIN);
            printf("writev finish\n");
            init();
            return true; // TODO浏览器的请求为长连接
        }
    }

    return true;
}

bool http_conn::process(){
    char *p,*space=(char*)" ";
    p = strtok(read_buf,space);
    strcpy(methed,p);
    p = strtok(NULL,space);
    p++;
    strcpy(file_path,(char*)"static/");
    strcat(file_path,p); printf("file_path: %s\n",file_path);
    
    /****** file ********/
    if (stat(file_path, &file_stat) < 0){
        fprintf(stderr, "file errno is: %d\n", errno);
        return false;
    }
    int fd = open(file_path, O_RDONLY);
    file_adr = (char *)mmap(0, file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);

    /****** response ******/
    char response[]="HTTP/1.1 200 OK\r\nServer:Linux Web Server\r\nContent-length:";
    char type[] = "\r\nContent-type:text/html\r\n\r\n";
    sprintf(write_buf,"%s%ld%s",response,file_stat.st_size,type);
    write_idx = strlen(write_buf);

    iv[0].iov_base = write_buf;
    iv[0].iov_len = write_idx;
    iv[1].iov_base = file_adr;
    iv[1].iov_len = file_stat.st_size;
    iv_count = 2;
    bytes_to_send = write_idx + file_stat.st_size;
    modfd(epollfd, sockfd, EPOLLOUT);
    return true;
}

void http_conn::unmap()
{
    if (file_adr)
    {
        munmap(file_adr, file_stat.st_size);
        file_adr = 0;
    }
}
