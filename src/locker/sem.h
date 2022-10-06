//
// Created by Alberta on 2022/9/17.
//

#ifndef LINUX_WEB_SERVER_SEM_H
#define LINUX_WEB_SERVER_SEM_H

#include <semaphore.h>

/**
 * 信号量
 */
class sem {

public:
    sem();
    explicit sem(int num);
    ~sem();
    bool wait();
    bool post();

private:

    sem_t m_sem{};

};


#endif //LINUX_WEB_SERVER_SEM_H
