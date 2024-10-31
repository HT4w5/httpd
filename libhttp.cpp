#include <string>
#include <stdexcept>

#include "libhttp.hpp"

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
    // TODO: Implement this method.
    // With FILE* stream or with syscall?
}