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

bool http_conn::dealfile(){
    char *p, *space=(char*)" ";
    p = strtok(read_buf,space);
    strcpy(methed,p);
    p = strtok(NULL,space);
    strcpy(file_path,(char*)"static");
    strcat(file_path,p); 
    printf("file_path: %s\n",file_path);
    if(strlen(file_path)==7)
    {
        strcat(file_path,(char*)"index.html");
    }
    return true;
}
bool http_conn::dealuser(){
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
    printf(">>> %s %s\n",user_name,user_pwd);
    //check
    if(!strcmp(file_path,(char*)"static/REG"))
    {
        //not same
        int flag = true;
        if(!strcmp(user_name,(char*)"admin")){
            fputs("user name existed", stderr);
            flag = false;
        }
        //save
        if(flag)
            strcpy(file_path,(char*)"static/log.html");
        else
            strcpy(file_path,(char*)"static/registerError.html");
    }
    else if(!strcmp(file_path,(char*)"static/LOG"))
    {
        //valid
        int flag = false;
        if(!strcmp(user_name,(char*)"root") && !strcmp(user_pwd,(char*)"root")){
            flag = true;
        }
        if(flag)
            strcpy(file_path,(char*)"static/index.html");
        else 
            strcpy(file_path,(char*)"static/logError.html");
    }
    else
    {
        printf("POST is not login or register\n");
        return false;
    }
    return true;
}

bool http_conn::process()
{
    dealfile();
    
    if(!strcmp(methed,(char*)"POST"))
    {
        if(!dealuser()) return false;
    }
    /****** file ********/
    if (stat(file_path, &file_stat) < 0)
    {
        fprintf(stderr, "file errno is: %d\n", errno);
        return false;
    }
    int fd = open(file_path, O_RDONLY);
    file_adr = (char*)mmap(0, file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
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
