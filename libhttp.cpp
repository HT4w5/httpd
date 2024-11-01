#include <cstring>
#include <stdexcept>
#include <fstream>

#include "libhttp.hpp"

#define MAX_REQUEST_SIZE 1024 * 1024 * 10
#define MAX_HEADER_SIZE 1024 * 1024 * 10

using namespace std;

http_request_handler::http_request_handler(int socket_fd)
{
    if (socket_fd < 0)
    {
        throw invalid_argument("Invalid socket file descriptor");
    }
    this->socket_fd = socket_fd;
}

http_request_handler::~http_request_handler()
{
}

void http_request_handler::parse()
{
}

char *http_request_handler::read_to_buffer()
{
    char *read_buffer=(char*)malloc(sizeof(char)*MAX_REQUEST_SIZE);

    //

}