#include <cstring>
#include <stdexcept>
#include <fstream>
#include <unistd.h>
#include <sys/socket.h>

#include "libhttp.hpp"

#define LIBHTTP_MAX_REQUEST_SIZE 1024 * 1024 * 10
#define LIBHTTP_MAX_HEADER_SIZE 1024 * 1024 * 10

#define LIBHTTP_HTTP_VERSION "HTTP/1.0"

using namespace std;

/****************
 * http_request *
 ****************/

http_request::http_request(int socket_fd)
{
    if (socket_fd < 0)
    {
        throw invalid_argument("Invalid socket file descriptor");
    }
    this->socket_fd = socket_fd;
    this->body = NULL;
    this->headers = NULL;
    this->path = NULL;
    this->version = NULL;
    this->method = NULL;
}

http_request::~http_request()
{
    free(this->method);
    free(this->path);
    free(this->version);
    free(this->headers);
    free(this->body);
}

void http_request::fatal_error(const string &message)
{
    // TODO: Rewrite with cpp exceptions, complete call stack.
    perror(message.c_str());
}

size_t http_request::read_from_socket(int fd, char *buffer, size_t size)
{
    // Make sure read finishes
    size_t bytes_read = 0;
    size_t total_bytes_read = 0;
    char *read_ptr = buffer;
    while (total_bytes_read < size)
    {
        bytes_read = read(fd, read_ptr, size - total_bytes_read);
        if (bytes_read <= 0)
        {
            if (bytes_read < 0)
            {
                perror("Error reading from socket");
            }
            // Reached EOF.
            break;
        }
        total_bytes_read += bytes_read;
        read_ptr += bytes_read;
    }
    this->request_size = total_bytes_read;
    return total_bytes_read;
}

void http_request::parse()
{
    // Read request into buffer.
    char *read_buffer = (char *)malloc(sizeof(char) * (LIBHTTP_MAX_HEADER_SIZE));
    read_from_socket(this->socket_fd, read_buffer, LIBHTTP_MAX_REQUEST_SIZE);

    // Parse request.
    // fp: fast pointer.
    // sp: slow pointer.
    char *fp, *sp;
    size_t read_size;

    // Get HTTP method.
    fp = sp = read_buffer;
    while (*fp >= 'A' && *fp <= 'Z')
    {
        fp++;
    }
    read_size = fp - sp;
    if (read_size == 0)
    {
        free(read_buffer);
        fatal_error("Invalid HTTP request");
    }
    this->method = (char *)malloc(read_size + 1);
    memcpy(this->method, sp, read_size);
    // Null terminate.
    this->method[read_size] = '\0';

    // Discard space.
    fp++;
    if (*fp != ' ')
    {
        free(read_buffer);
        fatal_error("Invalid HTTP request");
    }

    // Get request path.
    sp = fp;
    while (*fp != ' ' && *fp != '\n' && *fp != '\0')
    {
        fp++;
    }
    read_size = fp - sp;
    if (read_size == 0)
    {
        free(read_buffer);
        fatal_error("Invalid HTTP request");
    }
    this->path = (char *)malloc(read_size + 1);
    memcpy(this->path, sp, read_size);
    // Null terminate.
    this->path[read_size] = '\0';

    // Discard space.
    fp++;
    if (*fp != ' ')
    {
        free(read_buffer);
        fatal_error("Invalid HTTP request");
    }

    // Get HTTP version.
    sp = fp;
    while (*fp != ' ' && *fp != '\n' && *fp != '\0')
    {
        fp++;
    }
    read_size = fp - sp;
    if (read_size == 0)
    {
        free(read_buffer);
        fatal_error("Invalid HTTP request");
    }
    this->version = (char *)malloc(read_size + 1);
    memcpy(this->version, sp, read_size);
    // Null terminate.
    this->version[read_size] = '\0';

    // TODO:
    //      1.Parse other elements.
    //      2.Use one function to parse every section.

    free(read_buffer);
}

/*****************
 * http_response *
 *****************/

http_response::http_response(int socket_fd)
{
    if (socket_fd < 0)
    {
        throw invalid_argument("Invalid socket file descriptor");
    }
    this->socket_fd = socket_fd;
    this->headers = NULL;
    this->path = NULL;
    this->body = NULL;
    this->version = LIBHTTP_HTTP_VERSION;
    this->status = 0;
}

http_response::~http_response()
{
}

http_response::http_header_t *http_response::create_header()
{
    http_header_t *new_header = (http_header_t *)malloc(sizeof(http_header_t));
    memset(new_header, 0, sizeof(http_header_t));
    return new_header;
}

void http_response::append_header(const char *name, const char *value)
{
    // Allocate memory and copy value.
    size_t name_len, value_len;
    name_len = strlen(name);
    value_len = strlen(value);

    http_header_t *new_header = create_header();
    new_header->name = (char *)malloc(name_len + 1);
    new_header->value = (char *)malloc(value_len + 1);

    strcpy(new_header->name, name);
    strcpy(new_header->value, value);

    // Push new header into stack.
    new_header->next = this->headers;

    this->headers = new_header;
}

char *http_response::get_reason_phrase(int status)
{
    switch (status)
    {
    case 100:
        return "Continue";
    case 200:
        return "OK";
    case 301:
        return "Moved Permanently";
    case 302:
        return "Found";
    case 304:
        return "Not Modified";
    case 400:
        return "Bad Request";
    case 401:
        return "Unauthorized";
    case 403:
        return "Forbidden";
    case 404:
        return "Not Found";
    case 405:
        return "Method Not Allowed";
    default:
        return "Internal Server Error";
    }
}

void http_response::set_status(int status)
{
    this->status = status;
}

void http_response::send_status_line()
{
    if (this->status == 0)
    {
        throw invalid_argument("Status code not set");
    }
    dprintf(this->socket_fd, "%s %d %s\r\n", this->version, this->status, get_reason_phrase(this->status));
}

void http_response::send_headers()
{
    while (this->headers != NULL)
    {
        
    }
}

void http_response::send()
{
}
