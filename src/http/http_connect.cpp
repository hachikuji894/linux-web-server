//
// Created by Hachikuji on 2022/10/6.
//

#include "http_connect.h"
#include "utils/utils.h"
#include "iostream"

int HttpConnect::epoll_fd_ = -1;
int HttpConnect::user_count_ = 0;

HttpConnect::HttpConnect() = default;

HttpConnect::~HttpConnect() = default;


void HttpConnect::Init(int sock_fd, const sockaddr_in &address) {

    sock_fd_ = sock_fd;
    address_ = address;

    int reuse = 1;
    setsockopt(sock_fd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof reuse);

    AddFd(epoll_fd_, sock_fd_, true);
    user_count_++;

}

void HttpConnect::Close() {

    //关闭连接，关闭一个连接，客户总量减一
    if (sock_fd_ != -1) {
        RemoveFd(epoll_fd_, sock_fd_);
        sock_fd_ = -1;
        user_count_--;
    }

}

bool HttpConnect::Read() {
    std::cout << "读。。" << std::endl;
    return true;
}

bool HttpConnect::Write() {
    std::cout << "写。。" << std::endl;
    return true;
}

void HttpConnect::Process() {

    std::cout << "处理。。" << std::endl;

}