//
// Created by Alberta on 2022/10/9.
//

#ifndef LINUX_WEB_SERVER_UTILS_H
#define LINUX_WEB_SERVER_UTILS_H


//将内核事件表注册读事件，ET 模式，选择开启 EPOLL ONE SHOT
void AddFd(int epoll_fd, int fd, bool one_shot);

void RemoveFd(int epoll_fd, int fd);

// 信号捕捉
void AddSig(int sig, void(handler)(int));

// 设置非阻塞
void SetNonblocking(int fd);


#endif //LINUX_WEB_SERVER_UTILS_H
