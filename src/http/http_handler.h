//
// Created by Jiajun Chen on 2022/10/6.
//

#ifndef LINUX_WEB_SERVER_HTTP_HANDLER_H
#define LINUX_WEB_SERVER_HTTP_HANDLER_H

#include <netinet/in.h>
#include <sys/stat.h>

class HttpHandler {

public:

    static int epoll_fd_;
    static int user_count_;
    static const int READ_BUFFER_SIZE = 2048;
    static const int WRITE_BUFFER_SIZE = 2048;
    static const int FILENAME_LEN = 200;
    enum METHOD {
        GET = 0,
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT,
        PATH
    };
    enum CHECK_STATE {
        CHECK_STATE_REQUEST_LINE = 0,
        CHECK_STATE_HEADER,
        CHECK_STATE_CONTENT
    };
    enum HTTP_CODE {
        NO_REQUEST,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDEN_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION
    };
    enum LINE_STATUS {
        LINE_OK = 0,
        LINE_BAD,
        LINE_OPEN
    };


public:

    HttpHandler();

    ~HttpHandler();

    void Process();

    void Init();

    void Init(int sock_fd, const sockaddr_in &address);

    void Close();

    bool Read();

    bool Write();

private:

    int sock_fd_{};
    sockaddr_in address_{};

    char read_buf_[READ_BUFFER_SIZE]{};
    int read_idx_{};
    int checked_idx_{};
    int start_line_{};
    CHECK_STATE check_state_{};

    char *string_{};
    char *doc_root_{};

    char real_file_[FILENAME_LEN]{};
    char *url_{};
    char *version_{};
    char *host_{};
    bool linger_{};
    int content_length_{};
    METHOD method_{};

    char write_buf_[WRITE_BUFFER_SIZE]{};
    int write_idx_{};
    char *file_address_{};
    struct stat file_stat_{};
    // 目标文件的状态
    struct iovec iv_[2]{};
    // 采用 writev 来执行写程序
    int iv_count_{};

    int bytes_to_send_{};
    int bytes_have_send_{};

private:

    char *get_line() { return read_buf_ + start_line_; };

private:

    HTTP_CODE ProcessRead();

    HTTP_CODE ParseRequestLine(char *text);

    HTTP_CODE ParseHeaders(char *text);

    HTTP_CODE ParseContent(char *text);

    LINE_STATUS ParseLine();

    HTTP_CODE DoRequest();

    bool ProcessWrite(HTTP_CODE ret);

    void Unmap();

    bool AddResponse(const char *format, ...);

    bool AddContent(const char *content);

    bool AddStatusLine(int status, const char *title);

    bool AddHeaders(int content_length);

    bool AddContentType();

    bool AddContentLength(int content_length);

    bool AddLinger();

    bool AddBlankLine();
};


#endif //LINUX_WEB_SERVER_HTTP_HANDLER_H
