//
// Created by Alberta on 2022/10/6.
//

#ifndef LINUX_WEB_SERVER_HTTP_CONNECT_H
#define LINUX_WEB_SERVER_HTTP_CONNECT_H

#include <netinet/in.h>

class HttpConnect {

public:

    static int epoll_fd_;
    static int user_count_;

    bool Read();

    bool Write();

public:

    HttpConnect();

    ~HttpConnect();

    void Process();

    void Init(int sock_fd, const sockaddr_in &address);

    void Close();

private:

    int sock_fd_{};
    sockaddr_in address_{};

};


#endif //LINUX_WEB_SERVER_HTTP_CONNECT_H
