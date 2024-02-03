//
// Created by Jiajun Chen on 2022/9/17.
//

#include <exception>
#include "locker.h"

Locker::Locker() {

    if (pthread_mutex_init(&mutex_, nullptr) != 0) {
        throw std::exception();
    }

}

Locker::~Locker() {
    pthread_mutex_destroy(&mutex_);
}

bool Locker::Lock() {
    return pthread_mutex_lock(&mutex_) == 0;
}

bool Locker::Unlock() {
    return pthread_mutex_unlock(&mutex_) == 0;
}

pthread_mutex_t *Locker::get_mutex() {
    return &mutex_;
}
