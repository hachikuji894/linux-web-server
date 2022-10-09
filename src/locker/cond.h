//
// Created by Hachikuji on 2022/9/17.
//

#ifndef LINUX_WEB_SERVER_COND_H
#define LINUX_WEB_SERVER_COND_H

#include <pthread.h>

/**
 * 条件变量
 */
class Cond {

public:
    Cond();

    ~Cond();

    bool Wait(pthread_mutex_t *mutex);

    bool TimedWait(pthread_mutex_t *mutex, struct timespec t);

    bool Signal();

    bool Broadcast();

private:
    pthread_cond_t cond_{};

};


#endif //LINUX_WEB_SERVER_COND_H
