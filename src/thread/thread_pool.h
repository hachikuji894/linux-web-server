//
// Created by Alberta on 2022/10/6.
//

#ifndef LINUX_WEB_SERVER_THREAD_POOL_H
#define LINUX_WEB_SERVER_THREAD_POOL_H

#include <pthread.h>
#include <list>
#include <locker/locker.h>
#include <locker/sem.h>

template <typename T>
class thread_pool {
public:
    explicit thread_pool(int thread_number = 8, int max_request = 10000);
    ~thread_pool();
    bool append(T *request);
private:

    static void *worker(void *arg);
    void run();

private:

    // 线程池中的线程数
    int m_thread_number;

    // 请求队列中允许的最大请求数
    int m_max_request;

    // 描述线程池的数组，其大小为m_thread_number
    pthread_t *m_threads;

    // 请求队列
    std::list<T *> m_work_queue;

    // 保护请求队列的互斥锁
    locker m_queue_locker;

    // 是否有任务需要处理
    sem m_queue_state;

};



#endif //LINUX_WEB_SERVER_THREAD_POOL_H
