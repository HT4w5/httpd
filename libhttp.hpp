#ifndef LIBHTTP_H
#define LIBHTTP_H

#include <cstring>
#include <stdexcept>
#include <fstream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <errno.h>
#include <iostream>
#include <sstream>
#include <filesystem>
#include <ctime>
#include <iomanip>

#include "libhttp.hpp"

#define LIBHTTP_MAX_REQUEST_SIZE 1024 * 1024 * 10
#define LIBHTTP_MAX_HEADER_SIZE 1024 * 1024 * 10

#define LIBHTTP_WRITE_BUFFER_SIZE 4096

#define LIBHTTP_HTTP_VERSION "HTTP/1.1"
#define LIBHTTP_HTTP_SERVER_STRING "HTTPD/0.0.1"

using namespace std;
namespace fs = filesystem;

class http_request
{
public:
    size_t request_size;
    char *method;
    char *path;
    char *version;
    char *headers;
    char *body;

    http_request(int socket_fd);
    ~http_request();
    void parse();

private:
    int socket_fd;

    void fatal_error(const string &message);
    size_t read_from_socket(int fd, char *buffer, size_t size);
};

class http_response
{
public:
    http_response(int socket_fd);
    ~http_response();
    void serve_file();
    void serve_dir();
    void serve_msg();
    void set_status(int status);
    void set_path(const char *path);
    void set_msg(const string &msg);
    void push_header(const char *name, const char *value);

private:
    typedef struct HttpHeader
    {
        char *name;
        char *value;
        struct HttpHeader *next;

    } http_header_t;

    int socket_fd;
    int status;

    char *path;
    const char *str;
    string msg;
    char *version;
    http_header_t *header_queue_head;
    http_header_t *header_queue_tail;

    http_header_t *create_header();
    void enqueue_general_headers();
    void enqueue_server_headers();
    char *get_reason_phrase(int status);
    void send_status_line();
    void send_headers();
    void end_headers();
    void send_file();
    void send_str();
    string http_format_herf(const string &path, const string &filename);
    string http_get_mime_type(const string &extension);
};

#endif