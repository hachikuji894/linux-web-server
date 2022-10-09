//
// Created by Alberta on 2022/9/17.
//

#include <exception>
#include "sem.h"

Sem::Sem() {
    if (sem_init(&sem_, 0, 0) != 0) {
        throw std::exception();
    }
}

Sem::Sem(int num) {
    if (sem_init(&sem_, 0, num) != 0) {
        throw std::exception();
    }
}


Sem::~Sem() {
    sem_destroy(&sem_);
}

bool Sem::Wait() {
    return sem_wait(&sem_) == 0;
}

bool Sem::Post() {
    return sem_post(&sem_) == 0;
}


