//
// Created by Hachikuji on 2022/9/17.
//

#ifndef LINUX_WEB_SERVER_LOCKER_H
#define LINUX_WEB_SERVER_LOCKER_H

#include <pthread.h>

/**
 * 线程同步机制封装类（互斥锁）
 */
class Locker {

public:
    Locker();

    ~Locker();

    bool Lock();

    bool Unlock();

    pthread_mutex_t *get_mutex();

private:
    pthread_mutex_t mutex_{};

};


#endif //LINUX_WEB_SERVER_LOCKER_H
