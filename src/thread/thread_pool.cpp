//
// Created by Alberta on 2022/10/6.
//

#include "thread_pool.h"
template<typename T>
thread_pool<T>::thread_pool(int thread_number, int max_request):
        m_thread_number(thread_number), m_max_request(max_request), m_threads(nullptr){

    if((thread_number <= 0 || max_request <= 0))
        throw std::exception();
    m_threads = new pthread_t[m_thread_number];
    if (!m_threads)
        throw std::exception();
    for (int i = 0; i < thread_number; ++i)
    {
        if (pthread_create(m_threads + i, nullptr, worker, this) != 0)
        {
            delete[] m_threads;
            throw std::exception();
        }
        if (pthread_detach(m_threads[i]))
        {
            delete[] m_threads;
            throw std::exception();
        }
    }

}

template<typename T>
thread_pool<T>::~thread_pool() {
    delete[] m_threads;
}

template <typename T>
bool thread_pool<T>::append(T *request)
{
    m_queue_locker.lock();
    if (m_work_queue.size() >= m_max_request)
    {
        m_queue_locker.unlock();
        return false;
    }
    m_work_queue.push_back(request);
    m_queue_locker.unlock();
    m_queue_state.post();
    return true;
}

template<typename T>
void *thread_pool<T>::worker(void *arg) {

    auto *pool = (thread_pool *)arg;
    pool->run();
    return pool;

}

template<typename T>
void thread_pool<T>::run() {


    while (true){

        m_queue_state.wait();
        m_queue_locker.lock();
        if (m_work_queue.empty()){
            m_queue_locker.unlock();
            continue;
        }
        T *request = m_work_queue.front();
        m_work_queue.pop_front();
        m_queue_locker.unlock();
        if (!request)
            continue;

        request->process();

    }


}
