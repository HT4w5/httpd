#include <cstring>
#include <stdexcept>
#include <fstream>
#include <unistd.h>

#include "libhttp.hpp"

#define LIBHTTP_MAX_REQUEST_SIZE 1024 * 1024 * 10
#define LIBHTTP_MAX_HEADER_SIZE 1024 * 1024 * 10

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

size_t http_request_handler::read_from_socket(int fd, char *buffer, size_t size)
{
    // Make sure read finishes
    size_t bytes_read = 0;
    size_t total_bytes_read = 0;
    char *read_ptr = buffer;
    while (this->request_size < size)
    {
        bytes_read = read(fd, read_ptr, size - total_bytes_read);
        if (bytes_read <= 0)
        {
            if(bytes_read < 0)
            {
                perror("Error reading from socket");
            }
            // Connection closed
            break;
        }
        total_bytes_read += bytes_read;
        read_ptr += bytes_read;
    }
    
}

void http_request_handler::parse()
{
    // Read request into buffer.
    char *read_buffer = (char *)malloc(sizeof(char) * (LIBHTTP_MAX_HEADER_SIZE + 1));
}
