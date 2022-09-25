#ifndef THREADPOOL
#define THREADPOOL
#include "../http/http_conn.h"
#include "../locker.h"

using namespace std;

class threadpool{
public:
    threadpool();
    ~threadpool();
    bool append(http_conn* request, bool rw);

private:
    enum {
        thread_num = 50,  //线程池中的线程数
        max_requests = 50 //请求队列中允许的最大请求数
    }; 
    vector<pthread_t> threads;
    list<http_conn*> workqueue; //请求队列
    locker queuelock;  //保护请求队列的互斥锁
    sem queuetask;       //是否有任务需要处理

    void run();
    static void *worker(void *arg);
};

threadpool::threadpool()
{
    threads.assign(thread_num,0);
    for(auto p:threads)
    {
        if (pthread_create(&p, NULL, worker, this) != 0) {
            vector<pthread_t>().swap(threads);
            throw exception();
        }
        if (pthread_detach(p)) {
            vector<pthread_t>().swap(threads);
            throw exception();
        }
    }
}
threadpool::~threadpool()
{
    vector<pthread_t>().swap(threads);
}
bool threadpool::append(http_conn* request, bool rw)
{
    queuelock.lock();
    if (workqueue.size() >= max_requests)
    {
        queuelock.unlock();
        return false;
    }
    request->rw = rw;
    workqueue.push_back(request);
    queuelock.unlock();
    queuetask.post();
    return true;
}

void *threadpool::worker(void *arg)
{
    threadpool *pool = (threadpool*)arg;
    pool->run();
    return pool;
}
void threadpool::run()
{
    while (true)
    {
        queuetask.wait();
        queuelock.lock();
        if (workqueue.empty()) 
        {
            queuelock.unlock(); continue;
        }
        http_conn *request = workqueue.front();
        workqueue.pop_front();
        queuelock.unlock();

        if (!request) continue;
        if (request->rw == 0) 
        { // read
            if (!request->read()) 
            {
                printf("read error!\n");
                epoll_ctl(request->epollfd, EPOLL_CTL_DEL, request->sockfd, 0);
                close(request->sockfd);
            }
            else {
                // printf("%s\n",request->read_buf);
                if(!request->process()){
                    epoll_ctl(request->epollfd, EPOLL_CTL_DEL, request->sockfd, 0);
                    close(request->sockfd);
                }
            }
        }
        else 
        {  // write
            if (!request->write())
            {   //短连接或出错
                printf("close sockfd!\n");
                epoll_ctl(request->epollfd, EPOLL_CTL_DEL, request->sockfd, 0);
                close(request->sockfd);
            }else{ 
                //保持长连接
            }
        }
    }
}

#endif