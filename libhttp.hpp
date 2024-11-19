#ifndef LIBHTTP_H
#define LIBHTTP_H

#include <cstring>
#include <stdexcept>
#include <fstream>
#include <unistd.h>

using namespace std;

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
    void send();
    void set_status(int status);
    void set_path(const char *path);
    void append_header(const char *name, const char *value);

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
    char *version;
    http_header_t *headers;

    http_header_t *create_header();
    char *get_reason_phrase(int status);
    void send_status_line();
    void send_headers();
    void end_headers();
    void send_file();
    void send_buff(const char *);
};

#endif