//
// Created by Hachikuji on 2022/9/17.
//

#ifndef LINUX_WEB_SERVER_LOCKER_H
#define LINUX_WEB_SERVER_LOCKER_H

#include <pthread.h>

/**
 * 线程同步机制封装类（互斥锁）
 */
class locker {

public:
    locker();
    ~locker();
    bool lock();
    bool unlock();
    pthread_mutex_t * get();

private:
    pthread_mutex_t m_mutex{};

};


#endif //LINUX_WEB_SERVER_LOCKER_H
