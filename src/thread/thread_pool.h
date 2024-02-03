//
// Created by Jiajun Chen on 2022/10/6.
//

#ifndef LINUX_WEB_SERVER_THREAD_POOL_H
#define LINUX_WEB_SERVER_THREAD_POOL_H

#include <pthread.h>
#include <list>
#include "locker/locker.h"
#include "locker/sem.h"

template<typename T>
class ThreadPool {

public:

    explicit ThreadPool(int thread_number = 8, int max_request = 10000);

    ~ThreadPool();

    bool Append(T *request);

private:

    static void *Worker(void *arg);

    void Run();

private:

    // 线程池中的线程数
    int thread_number_{};

    // 请求队列中允许的最大请求数
    int max_request_{};

    // 描述线程池的数组，其大小为m_thread_number
    pthread_t *threads_{};

    // 请求队列
    std::list<T *> work_queue_;

    // 保护请求队列的互斥锁
    Locker queue_locker_;

    // 是否有任务需要处理
    Sem queue_state_;

};


#endif //LINUX_WEB_SERVER_THREAD_POOL_H
