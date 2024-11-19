#include <cstring>
#include <stdexcept>
#include <fstream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <errno.h>
#include <iostream>
#include <sstream>

#include "libhttp.hpp"

#define LIBHTTP_MAX_REQUEST_SIZE 1024 * 1024 * 10
#define LIBHTTP_MAX_HEADER_SIZE 1024 * 1024 * 10

#define LIBHTTP_WRITE_BUFFER_SIZE 4096

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
    stringstream err_msg;
    ssize_t bytes_read = 0;
    ssize_t total_bytes_read = 0;
    char *read_ptr = buffer;
    while (total_bytes_read < size)
    {
        bytes_read = read(fd, read_ptr, size - total_bytes_read);
        if (bytes_read <= 0)
        {
            if (bytes_read < 0)
            {
                err_msg << "Error reading from socket: " << this->socket_fd << " (" << strerror(errno) << ")" << endl;
                throw runtime_error(err_msg.str());
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
    char *read_buffer;
    if ((read_buffer = (char *)malloc(sizeof(char) * (LIBHTTP_MAX_HEADER_SIZE))) < 0)
    {
        throw bad_alloc();
    }
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
        fatal_error("Invalid HTTP request"); //TODO: send bad request.
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
    this->version = LIBHTTP_HTTP_VERSION;
    this->status = 0;
}

http_response::~http_response()
{
    free(this->path);
    // TODO: free headers stack.
}

http_response::http_header_t *http_response::create_header()
{
    http_header_t *new_header = (http_header_t *)malloc(sizeof(http_header_t));
    if (new_header == NULL)
    {
        throw bad_alloc();
    }
    memset(new_header, 0, sizeof(http_header_t));
    return new_header;
}

void http_response::append_header(const char *name, const char *value)
{
    // Allocate memory and copy value.
    size_t name_len, value_len;
    name_len = strlen(name);
    value_len = strlen(value);

    http_header_t *new_header;
    new_header = create_header();

    if ((new_header->name = (char *)malloc(name_len + 1)) < 0)
    {
        throw bad_alloc();
    }
    if ((new_header->value = (char *)malloc(value_len + 1)) < 0)
    {
        throw bad_alloc();
    }

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
    if (dprintf(this->socket_fd, "%s %d %s\r\n", this->version, this->status, get_reason_phrase(this->status)) < 0)
    {
        throw runtime_error("Send status line failed");
    }
}

// Send header fields and values to socket fd.
// Headers are stored in stack and sent LIFO.
// Doesn't do anything if stack is empty.
void http_response::send_headers()
{
    http_header_t *current = NULL;
    while (this->headers != NULL)
    {
        // Check header presence.
        if (this->headers->name == NULL || this->headers->value == NULL)
        {
            throw invalid_argument("Invalid header");
        }
        // Send header line.
        if (dprintf(this->socket_fd, "%s: %s\r\n", this->headers->name, this->headers->value) < 0)
        {
            throw runtime_error("Send header failed");
        }

        // Pop stack.
        current = this->headers;
        this->headers = this->headers->next;
        free(current);
    }
}

void http_response::end_headers()
{
    if (dprintf(this->socket_fd, "\r\n") < 0)
    {
        throw runtime_error("Send header failed");
    }
}

// Open fd from this.path and send copy content to socket fd.
// Only send file content, headers should be handled before invoke.
// Assuming the path leads to a valid file.
void http_response::send_file()
{
    // Get file descriptor.
    stringstream err_msg;
    int file_fd = open(this->path, O_RDONLY, S_IRUSR);
    if (file_fd < 0)
    {
        err_msg << "Error opening file: " << this->path << " (" << strerror(errno) << ")" << endl;
        throw runtime_error(err_msg.str());
    }

    // Write to socket.
    char write_buffer[LIBHTTP_WRITE_BUFFER_SIZE];
    char *write_buffer_ptr = NULL;
    ssize_t n_buffered, n_written;

    while (1)
    {
        // Repeatedly read and write until EOF.
        n_buffered = read(file_fd, write_buffer, LIBHTTP_WRITE_BUFFER_SIZE);
        if (n_buffered < 0)
        {
            err_msg.str("");
            err_msg << "Error reading from file: " << this->path << strerror(errno) << endl;
            throw runtime_error(err_msg.str());
        }
        else if (n_buffered == 0)
        {
            // EOF reached.
            break;
        }

        // Repeatedly write to guarantee completion.
        // Reset pointer pos.
        write_buffer_ptr = write_buffer;

        while (n_buffered > 0)
        {
            n_written = write(this->socket_fd, write_buffer_ptr, n_buffered);
            if (n_written < 0)
            {
                err_msg.str("");
                err_msg << "Error writing to socket: " << this->socket_fd << strerror(errno) << endl;
                throw runtime_error(err_msg.str());
            }
            n_buffered -= n_written;
            write_buffer_ptr += n_written;
        }
    }
}

void http_response::send_buff(const char *)
{
}

// Copy path to this->path.
// Does not validate path.
// format: "path/to/file".
void http_response::set_path(const char *path)
{
    size_t path_len;
    path_len = strlen(path);
    if ((this->path = (char *)malloc(path_len + 1)) < 0)
    {
        throw bad_alloc();
    }
    strcpy(this->path, path);
}

// Only support sending files and file lists for now.
// Assumes path is correct in format, and file exists.
// format: "path/to/file".
void http_response::send()
{
    try
    {
        send_status_line();
        send_headers();
        end_headers();
        send_file();
    }
    catch (const runtime_error &e)
    {
        cerr << "Runtime error caught: " << e.what() << endl;
        throw runtime_error("libhttp: send request failed");
    }
    catch (const invalid_argument &e)
    {
        cerr << "Invalid argument caught: " << e.what() << endl;
        throw runtime_error("libhttp: send request failed");
    }
    catch (const exception &e)
    {
        cerr << "Unknown error caught: " << e.what() << endl;
        throw runtime_error("libhttp: send request failed");
    }
}
