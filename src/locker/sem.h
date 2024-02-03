//
// Created by Jiajun Chen on 2022/9/17.
//

#ifndef LINUX_WEB_SERVER_SEM_H
#define LINUX_WEB_SERVER_SEM_H

#include <semaphore.h>

/**
 * 信号量
 */
class Sem {

public:
    Sem();

    explicit Sem(int num);

    ~Sem();

    bool Wait();

    bool Post();

private:

    sem_t sem_{};

};

#endif //LINUX_WEB_SERVER_SEM_H
