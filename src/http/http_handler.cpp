//
// Created by Jiajun Chen on 2022/10/6.
//
#include "http_handler.h"
#include "utils/utils.h"
#include <sys/epoll.h>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <cstdarg>
#include <sys/uio.h>

int HttpHandler::epoll_fd_ = -1;
int HttpHandler::user_count_ = 0;


/**
 * 定义 http 响应的一些状态信息
 */
const char *kOk200Title = "OK";
const char *kError400Title = "Bad Request";
const char *kError400Form = "Your request has bad syntax or is inherently impossible to staisfy.\n";
const char *kError403Title = "Forbidden";
const char *kError403Form = "You do not have permission to get file form this server.\n";
const char *kError404Title = "Not Found";
const char *kError404Form = "The requested file was not found on this server.\n";
const char *kError500Title = "Internal Error";
const char *kError500Form = "There was an unusual problem serving the request file.\n";

HttpHandler::HttpHandler() = default;

HttpHandler::~HttpHandler() = default;

void HttpHandler::Init() {

    check_state_ = CHECK_STATE_REQUEST_LINE;
    linger_ = false;
    method_ = GET;
    url_ = nullptr;
    version_ = nullptr;
    content_length_ = 0;
    host_ = nullptr;
    start_line_ = 0;
    checked_idx_ = 0;
    read_idx_ = 0;
    write_idx_ = 0;
    doc_root_ = get_current_dir_name();

    bytes_to_send_ = 0;
    bytes_have_send_ = 0;

    memset(read_buf_, '\0', READ_BUFFER_SIZE);
    memset(write_buf_, '\0', WRITE_BUFFER_SIZE);
    memset(real_file_, '\0', FILENAME_LEN);

}

void HttpHandler::Init(int sock_fd, const sockaddr_in &address) {

    sock_fd_ = sock_fd;
    address_ = address;

    int reuse = 1;
    setsockopt(sock_fd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof reuse);

    AddFd(epoll_fd_, sock_fd_, true);
    user_count_++;

    Init();

}

void HttpHandler::Close() {

    // 关闭连接，关闭一个连接，客户总量减一
    if (sock_fd_ != -1) {
        RemoveFd(epoll_fd_, sock_fd_);
        sock_fd_ = -1;
        user_count_--;
    }

}

/**
 * 循环读取客户数据，直到无数据可读或对方关闭连接
 * @return
 */
bool HttpHandler::Read() {

    if (read_idx_ >= READ_BUFFER_SIZE) {
        return false;
    }
    int bytes_read = 0;
    while (true) {
        bytes_read = recv(sock_fd_, read_buf_ + read_idx_, READ_BUFFER_SIZE - read_idx_, 0);
        if (bytes_read == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            return false;
        } else if (bytes_read == 0) {
            return false;
        }
        read_idx_ += bytes_read;
    }
    std::cout << read_buf_ << std::endl;
    return true;

}

bool HttpHandler::Write() {

    int temp = 0;

    // 表示要发送的数据长度为 0，响应报文为空，一般不会出现这种情况
    if (bytes_to_send_ == 0) {
        ModFd(epoll_fd_, sock_fd_, EPOLLIN);
        Init();
        return true;
    }

    while (true) {

        // 将响应报文的状态行、消息头、空行和响应正文发送给浏览器端
        temp = writev(sock_fd_, iv_, iv_count_);

        if (temp < 0) {
            // 判断缓冲区是否满了
            if (errno == EAGAIN) {
                // 重新注册写事件
                ModFd(epoll_fd_, sock_fd_, EPOLLOUT);
                return true;
            }
            Unmap();
            return false;
        }

        bytes_have_send_ += temp;
        bytes_to_send_ -= temp;
        if (bytes_have_send_ >= iv_[0].iov_len) {
            iv_[0].iov_len = 0;
            iv_[1].iov_base = file_address_ + (bytes_have_send_ - write_idx_);
            iv_[1].iov_len = bytes_to_send_;
        } else {
            iv_[0].iov_base = write_buf_ + bytes_have_send_;
            iv_[0].iov_len = iv_[0].iov_len - bytes_have_send_;
        }

        // 数据已全部发送完
        if (bytes_to_send_ <= 0) {
            Unmap();
            ModFd(epoll_fd_, sock_fd_, EPOLLIN);

            if (linger_) {
                Init();
                return true;
            } else
                return false;

        }
    }
}

/**
 * 解析 HTTP 请求
 */
void HttpHandler::Process() {

    HTTP_CODE read_ret = ProcessRead();

    // 解析不完整，需要继续接受数据
    if (read_ret == NO_REQUEST) {
        ModFd(epoll_fd_, sock_fd_, EPOLLIN);
        return;
    }

    // 生成响应
    bool write_ret = ProcessWrite(read_ret);
    if (!write_ret) {
        Close();
    }
    // 注册并监听写事件
    ModFd(epoll_fd_, sock_fd_, EPOLLOUT);


}

HttpHandler::HTTP_CODE HttpHandler::ProcessRead() {

    LINE_STATUS line_status = LINE_OK;
    HTTP_CODE ret = NO_REQUEST;
    char *text = nullptr;

    while ((check_state_ == CHECK_STATE_CONTENT && line_status == LINE_OK) ||
           ((line_status = ParseLine()) == LINE_OK)) {
        text = get_line();
        start_line_ = checked_idx_;
        switch (check_state_) {
            case CHECK_STATE_REQUEST_LINE: {
                ret = ParseRequestLine(text);
                if (ret == BAD_REQUEST)
                    return BAD_REQUEST;
                break;
            }
            case CHECK_STATE_HEADER: {
                ret = ParseHeaders(text);
                if (ret == BAD_REQUEST)
                    return BAD_REQUEST;
                else if (ret == GET_REQUEST)
                    return DoRequest();
                break;
            }
            case CHECK_STATE_CONTENT: {
                ret = ParseContent(text);
                if (ret == GET_REQUEST)
                    return DoRequest();
                line_status = LINE_OPEN;
                break;
            }
            default:
                return INTERNAL_ERROR;
        }
    }
    return NO_REQUEST;
}

/**
 * 解析 http 请求行，获得请求方法，目标 url 及 http 版本号
 * @param text 行数据
 * @return
 */
HttpHandler::HTTP_CODE HttpHandler::ParseRequestLine(char *text) {

    // GET /index.html HTTP/1.1
    url_ = strpbrk(text, " \t");
    if (!url_)
        return BAD_REQUEST;

    // GET\0/index.html HTTP/1.1
    *url_++ = '\0';
    char *method = text;

    if (strcasecmp(method, "GET") == 0)
        method_ = GET;
    else if (strcasecmp(method, "POST") == 0)
        method_ = POST;
    else
        return BAD_REQUEST;

    url_ += strspn(url_, " \t");

    // /index.html HTTP/1.1
    version_ = strpbrk(url_, " \t");
    if (!version_)
        return BAD_REQUEST;
    // /index.html\0HTTP/1.1
    *version_++ = '\0';

    version_ += strspn(version_, " \t");
    if (strcasecmp(version_, "HTTP/1.1") != 0)
        return BAD_REQUEST;

    // http://192.168.1.1:6789/index.html
    if (strncasecmp(url_, "http://", 7) == 0) {
        // 192.168.1.1:6789/index.html
        url_ += 7;
        // /index.html
        url_ = strchr(url_, '/');
    }

    if (strncasecmp(url_, "https://", 8) == 0) {
        url_ += 8;
        url_ = strchr(url_, '/');
    }

    if (!url_ || url_[0] != '/')
        return BAD_REQUEST;

    // 请求行处理完毕，将主状态机转移处理请求头
    check_state_ = CHECK_STATE_HEADER;
    return NO_REQUEST;

}

/**
 * 解析 HTTP 请求的一个头部信息
 * @param text
 * @return
 */
HttpHandler::HTTP_CODE HttpHandler::ParseHeaders(char *text) {

    if (text[0] == '\0') {
        if (content_length_ != 0) {
            check_state_ = CHECK_STATE_CONTENT;
            return NO_REQUEST;
        }
        return GET_REQUEST;
    } else if (strncasecmp(text, "Connection:", 11) == 0) {
        text += 11;
        text += strspn(text, " \t");
        if (strcasecmp(text, "keep-alive") == 0) {
            linger_ = true;
        }
    } else if (strncasecmp(text, "Content-length:", 15) == 0) {
        text += 15;
        text += strspn(text, " \t");
        content_length_ = atol(text);
    } else if (strncasecmp(text, "Host:", 5) == 0) {
        text += 5;
        text += strspn(text, " \t");
        host_ = text;
    } else {
        std::cout << "oop! unknown header: " << text << std::endl;
    }
    return NO_REQUEST;

}

/**
 * 解析 HTTP 请求体的消息，判断是否被完整的读入
 * @param text
 * @return
 */
HttpHandler::HTTP_CODE HttpHandler::ParseContent(char *text) {

    if (read_idx_ >= (content_length_ + checked_idx_)) {
        text[content_length_] = '\0';

        string_ = text;
        return GET_REQUEST;
    }
    return NO_REQUEST;
}


/**
 * 从状态机，用于分析出一行内容
 * @return 行的读取状态，有 LINE_OK,LINE_BAD,LINE_OPEN
 */
HttpHandler::LINE_STATUS HttpHandler::ParseLine() {

    char temp;
    for (; checked_idx_ < read_idx_; ++checked_idx_) {
        temp = read_buf_[checked_idx_];
        if (temp == '\r') {
            if ((checked_idx_ + 1) == read_idx_)
                return LINE_OPEN;
            else if (read_buf_[checked_idx_ + 1] == '\n') {
                // 修改 \r 和 \n
                read_buf_[checked_idx_++] = '\0';
                read_buf_[checked_idx_++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        } else if (temp == '\n') {
            if (checked_idx_ > 1 && read_buf_[checked_idx_ - 1] == '\r') {
                read_buf_[checked_idx_ - 1] = '\0';
                read_buf_[checked_idx_++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }
    return LINE_OPEN;
}

HttpHandler::HTTP_CODE HttpHandler::DoRequest() {

    strcpy(real_file_, doc_root_);
    int len = strlen(doc_root_);
    strncpy(real_file_ + len, url_, FILENAME_LEN - len - 1);

    // 获取 real_file_ 文件的相关状态信息，-1 失败 0 成功
    if (stat(real_file_, &file_stat_) < 0)
        return NO_RESOURCE;

    // 判断访问权限
    if (!(file_stat_.st_mode & S_IROTH))
        return FORBIDDEN_REQUEST;

    // 判断是否是目录
    if (S_ISDIR(file_stat_.st_mode))
        return BAD_REQUEST;

    int fd = open(real_file_, O_RDONLY);
    file_address_ = (char *) mmap(nullptr, file_stat_.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    return FILE_REQUEST;

}

/**
 * 根据服务器处理 HTTP 请求结果，决定返回给客户端的内容
 * @param ret
 * @return
 */
bool HttpHandler::ProcessWrite(HttpHandler::HTTP_CODE ret) {

    switch (ret) {
        case INTERNAL_ERROR: {
            AddStatusLine(500, kError500Title);
            AddHeaders(strlen(kError500Form));
            if (!AddContent(kError500Form))
                return false;
            break;
        }
        case BAD_REQUEST: {
            AddStatusLine(404, kError404Title);
            AddHeaders(strlen(kError404Form));
            if (!AddContent(kError404Form))
                return false;
            break;
        }
        case FORBIDDEN_REQUEST: {
            AddStatusLine(403, kError403Title);
            AddHeaders(strlen(kError403Form));
            if (!AddContent(kError403Form))
                return false;
            break;
        }
        case FILE_REQUEST: {
            AddStatusLine(200, kOk200Title);
            if (file_stat_.st_size != 0) {
                AddHeaders(file_stat_.st_size);
                iv_[0].iov_base = write_buf_;
                iv_[0].iov_len = write_idx_;
                iv_[1].iov_base = file_address_;
                iv_[1].iov_len = file_stat_.st_size;
                iv_count_ = 2;
                bytes_to_send_ = write_idx_ + file_stat_.st_size;
                return true;
            } else {
                const char *ok_string = "<html><body></body></html>";
                AddHeaders(strlen(ok_string));
                if (!AddContent(ok_string))
                    return false;
            }
        }
        default:
            return false;
    }
    iv_[0].iov_base = write_buf_;
    iv_[0].iov_len = write_idx_;
    iv_count_ = 1;
    bytes_to_send_ = write_idx_;
    return true;

}

void HttpHandler::Unmap() {

    if (file_address_) {
        munmap(file_address_, file_stat_.st_size);
        file_address_ = nullptr;
    }

}


bool HttpHandler::AddResponse(const char *format, ...) {

    if (write_idx_ >= WRITE_BUFFER_SIZE)
        return false;
    va_list arg_list;
    va_start(arg_list, format);
    int len = vsnprintf(write_buf_ + write_idx_, WRITE_BUFFER_SIZE - 1 - write_idx_, format, arg_list);
    if (len >= (WRITE_BUFFER_SIZE - 1 - write_idx_)) {
        va_end(arg_list);
        return false;
    }
    write_idx_ += len;
    va_end(arg_list);

    std::cout << write_buf_ << std::endl;
    return true;

}

bool HttpHandler::AddContent(const char *content) {

    return AddResponse("%s", content);

}

bool HttpHandler::AddStatusLine(int status, const char *title) {

    return AddResponse("%s %d %s\r\n", "HTTP/1.1", status, title);

}

bool HttpHandler::AddHeaders(int content_length) {

    return AddContentLength(content_length)
           && AddContentType()
           && AddLinger()
           && AddBlankLine();

}

bool HttpHandler::AddContentType() {

    return AddResponse("Content-Type:%s\r\n", "text/html");

}

bool HttpHandler::AddContentLength(int content_length) {

    return AddResponse("Content-Length:%d\r\n", content_length);

}

bool HttpHandler::AddLinger() {

    return AddResponse("Connection:%s\r\n", linger_ ? "keep-alive" : "close");

}

bool HttpHandler::AddBlankLine() {

    return AddResponse("%s", "\r\n");

}