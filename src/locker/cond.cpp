//
// Created by Hachikuji on 2022/9/17.
//

#include <exception>
#include "cond.h"

Cond::Cond() {

    if (pthread_cond_init(&cond_, nullptr) != 0) {
        throw std::exception();
    }

}

Cond::~Cond() {
    pthread_cond_destroy(&cond_);
}

bool Cond::Wait(pthread_mutex_t *mutex) {
    // wait 之前是存在 pthread_mutex_lock 加锁操作的
    // 当这个函数调用阻塞的时候，会对互斥锁进行解锁，等待条件变量通知
    // 当不阻塞时，会重新加锁，继续向下执行。
    return pthread_cond_wait(&cond_, mutex) == 0;
}

bool Cond::TimedWait(pthread_mutex_t *mutex, struct timespec t) {
    return pthread_cond_timedwait(&cond_, mutex, &t) == 0;
}

bool Cond::Signal() {
    return pthread_cond_signal(&cond_) == 0;
}

bool Cond::Broadcast() {
    return pthread_cond_broadcast(&cond_) == 0;
}
