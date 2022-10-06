//
// Created by Hachikuji on 2022/9/17.
//

#ifndef LINUX_WEB_SERVER_COND_H
#define LINUX_WEB_SERVER_COND_H

#include <pthread.h>

/**
 * 条件变量
 */
class cond {

public:
    cond();
    ~cond();
    bool wait(pthread_mutex_t *mutex);
    bool timed_wait(pthread_mutex_t *mutex,struct timespec t);
    bool signal();
    bool broadcast();

private:
    pthread_cond_t m_cond{};

};


#endif //LINUX_WEB_SERVER_COND_H
