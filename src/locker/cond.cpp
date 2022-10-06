//
// Created by Hachikuji on 2022/9/17.
//

#include <exception>
#include "cond.h"

cond::cond() {

    if (pthread_cond_init(&m_cond, nullptr) != 0) {
        throw std::exception();
    }

}

cond::~cond() {
    pthread_cond_destroy(&m_cond);
}

bool cond::wait(pthread_mutex_t *mutex) {
    // wait 之前是存在 pthread_mutex_lock 加锁操作的
    // 当这个函数调用阻塞的时候，会对互斥锁进行解锁，等待条件变量通知
    // 当不阻塞时，会重新加锁，继续向下执行。
    return pthread_cond_wait(&m_cond, mutex) == 0;
}

bool cond::timed_wait(pthread_mutex_t *mutex, struct timespec t) {
    return pthread_cond_timedwait(&m_cond, mutex, &t) == 0;
}

bool cond::signal() {
    return pthread_cond_signal(&m_cond) == 0;
}

bool cond::broadcast() {
    return pthread_cond_broadcast(&m_cond) == ;
}
