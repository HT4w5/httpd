#ifndef LIBHTTP_H
#define LIBHTTP_H

#include <cstring>
#include <stdexcept>
#include <fstream>
#include <unistd.h>

using namespace std;

class http_request_handler
{
public:
    size_t request_size;
    char *method;
    char *url;
    char *version;
    char *headers;
    char *body;
    http_request_handler(int socket_fd);
    ~http_request_handler();
    void parse();

private:
    int socket_fd;
    void fatal_error(const string &message);
    size_t read_from_socket(int fd, char *buffer, size_t size);
};

#endif