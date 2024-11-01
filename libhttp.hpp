#ifndef LIBHTTP_H
#define LIBHTTP_H

#include <cstring>
#include <stdexcept>
#include <fstream>

using namespace std;

class http_request_handler
{
public:
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
    char *read_to_buffer();
    void fatal_error();
};

#endif