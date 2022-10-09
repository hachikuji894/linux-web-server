#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cerrno>
#include <cstdlib>
#include <sys/epoll.h>
#include <csignal>
#include <cstdio>

#include "thread/thread_pool.cpp"
#include "http/http_connect.h"
#include "utils/utils.h"

#define MAX_FD 65535
#define MAX_EVENT_NUMBER 10000


int main(int argc, char *argv[]) {

    if (argc <= 1) {
        std::cout << "please input the argument of socket port" << std::endl;
        exit(-1);
    }

    int port = atoi(argv[1]);

    std::cout << "socket port is " << port << std::endl;

    // 对 SIGPIPE 信号进行处理
    AddSig(SIGPIPE, SIG_IGN);

    ThreadPool<HttpConnect> *pool;
    try {
        pool = new ThreadPool<HttpConnect>();
    } catch (...) {
        exit(-1);
    }

    auto *users = new HttpConnect[MAX_FD];

    int listen_fd = socket(PF_INET, SOCK_STREAM, 0);
    int reuse = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof reuse);

    // 绑定
    struct sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    int ret = bind(listen_fd, (struct sockaddr *) &address, sizeof address);

    if (ret == -1) {
        perror("bind");
        exit(-1);
    }

    // 监听
    // epoll创建内核事件表
    epoll_event events[MAX_EVENT_NUMBER];
    int epoll_fd = epoll_create(5);

    // 监听的文件描述符添加到 epoll 对象中
    AddFd(epoll_fd, listen_fd, false);
    HttpConnect::epoll_fd_ = epoll_fd;

    while (true) {

        int number = epoll_wait(epoll_fd, events, MAX_EVENT_NUMBER, -1);
        if (number < 0 && errno != EINTR) {
            std::cout << "epoll failure" << std::endl;
            break;
        }

        for (int i = 0; i < number; i++) {
            int sock_fd = events[i].data.fd;
            // 处理新到的客户连接
            if (sock_fd == listen_fd) {
                struct sockaddr_in client_address{};
                socklen_t client_address_len = sizeof client_address;
                int conn_fd = accept(listen_fd, (struct sockaddr *) &client_address, &client_address_len);

                if (conn_fd < 0) {
                    // std::cout << "errno is: " << errno << std::endl;
                    break;
                }
                if (HttpConnect::user_count_ >= MAX_FD) {
                    std::cout << "internal server busy" << std::endl;
                    break;
                }
                users[conn_fd].Init(conn_fd, client_address);
            } else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                // 异常断开
                users[sock_fd].Close();
            } else if (events[i].events & EPOLLIN) {
                // 处理信号
                if (users[sock_fd].Read()) {
                    pool->Append(users + sock_fd);
                } else {
                    users[sock_fd].Close();
                }
            } else if (events[i].events & EPOLLOUT) {
                if (!users[sock_fd].Write()) {
                    users[sock_fd].Close();
                }
            }
        }
    }

    close(epoll_fd);
    close(listen_fd);
    delete[] users;
    delete pool;
    return 0;
}
