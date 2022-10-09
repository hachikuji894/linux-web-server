//
// Created by Hachikuji on 2022/10/6.
//

#include "thread_pool.h"
#include "iostream"

template<typename T>
ThreadPool<T>::ThreadPool(int thread_number, int max_request):
        thread_number_(thread_number), max_request_(max_request), threads_(nullptr) {

    if ((thread_number <= 0 || max_request <= 0))
        throw std::exception();
    threads_ = new pthread_t[thread_number_];

    for (int i = 0; i < thread_number; ++i) {

        std::cout << "create the " + std::to_string(i) + "th thread" << std::endl;

        if (pthread_create(threads_ + i, nullptr, Worker, this) != 0) {
            delete[] threads_;
            throw std::exception();
        }
        if (pthread_detach(threads_[i])) {
            delete[] threads_;
            throw std::exception();
        }
    }

}

template<typename T>
ThreadPool<T>::~ThreadPool() {
    delete[] threads_;
}

template<typename T>
bool ThreadPool<T>::Append(T *request) {
    queue_locker_.Lock();
    if (work_queue_.size() >= max_request_) {
        queue_locker_.Unlock();
        return false;
    }
    work_queue_.push_back(request);
    queue_locker_.Unlock();
    queue_state_.Post();
    return true;
}

template<typename T>
void *ThreadPool<T>::Worker(void *arg) {

    auto *pool = (ThreadPool *) arg;
    pool->Run();
    return pool;

}

template<typename T>
void ThreadPool<T>::Run() {

    while (true) {

        queue_state_.Wait();
        queue_locker_.Lock();
        if (work_queue_.empty()) {
            queue_locker_.Unlock();
            continue;
        }
        T *request = work_queue_.front();
        work_queue_.pop_front();
        queue_locker_.Unlock();
        if (!request)
            continue;

        request->Process();
    }
}
