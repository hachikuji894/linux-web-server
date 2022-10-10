//
// Created by Hachikuji on 2022/10/9.
//

#include "utils.h"
#include <csignal>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstdio>
#include <cstdlib>

void AddFd(int epoll_fd, int fd, bool one_shot) {

    epoll_event event{};
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLRDHUP;

    if (one_shot)
        event.events |= EPOLLONESHOT;

    // 将监听文件描述符加入实例
    int ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event);
    if (ret == -1) {
        perror("epoll_ctl");
        exit(-1);
    }

    SetNonblocking(fd);

}


void AddSig(int sig, void(handler)(int)) {

    struct sigaction sa{};
    sa.sa_handler = handler;
    sigfillset(&sa.sa_mask);
    sigaction(sig, &sa, nullptr);

}

void RemoveFd(int epoll_fd, int fd) {

    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, nullptr);
    close(fd);

}

// 对文件描述符设置非阻塞
void SetNonblocking(int fd) {

    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);

}

void ModFd(int epoll_fd, int fd, int ev) {

    epoll_event event{};
    event.data.fd = fd;
    event.events = ev | EPOLLIN | EPOLLRDHUP;
    int ret = epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &event);
    if (ret == -1) {
        perror("epoll_ctl");
        exit(-1);
    }

}
