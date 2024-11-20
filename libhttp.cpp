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
    ostringstream err_msg;
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
    if ((read_buffer = (char *)malloc(sizeof(char) * (LIBHTTP_MAX_HEADER_SIZE))) == NULL)
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
        throw invalid_argument("Bad HTTP request");
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
        throw invalid_argument("Bad HTTP request");
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
        throw invalid_argument("Bad HTTP request");
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
        throw invalid_argument("Bad HTTP request");
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
        throw invalid_argument("Bad HTTP request");
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
    this->header_queue_head = NULL;
    this->header_queue_tail = NULL;
    this->path = NULL;
    this->str = NULL;
    this->version = LIBHTTP_HTTP_VERSION;
    this->status = 0;
}

http_response::~http_response()
{
    free(this->path);
    // Free headers queue.
    http_header_t *current = NULL;
    while (this->header_queue_tail != NULL)
    {
        current = this->header_queue_tail;
        this->header_queue_tail = this->header_queue_tail->next;
        free(current);
    }
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

void http_response::push_header(const char *name, const char *value)
{
    // Allocate memory and copy value.
    size_t name_len, value_len;
    name_len = strlen(name);
    value_len = strlen(value);

    http_header_t *new_header;
    new_header = create_header();

    if ((new_header->name = (char *)malloc(name_len + 1)) == NULL)
    {
        throw bad_alloc();
    }
    if ((new_header->value = (char *)malloc(value_len + 1)) == NULL)
    {
        throw bad_alloc();
    }

    strcpy(new_header->name, name);
    strcpy(new_header->value, value);

    // Push new header into queue.
    if (this->header_queue_tail == NULL)
    {
        this->header_queue_tail = new_header;
    }
    this->header_queue_head->next = new_header;
    this->header_queue_head = new_header;
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
    case 501:
        return "Not Implemented";
    case 505:
        return "HTTP Version Not Supported";
    case 500:
    default:
        return "Internal Server Error"; // Last resort.
    }
}

void http_response::set_status(int status)
{
    this->status = status;
}

void http_response::send_status_line()
{
    if (dprintf(this->socket_fd, "%s %d %s\r\n", this->version, this->status, get_reason_phrase(this->status)) < 0)
    {
        throw runtime_error("Send status line failed");
    }
}

// Send header fields and values to socket fd.
// Headers are stored in queue and sent FIFO.
// Doesn't do anything if queue is empty.
void http_response::send_headers()
{
    http_header_t *current = NULL;
    while (this->header_queue_tail != NULL)
    {
        // Check header presence.
        if (this->header_queue_tail->name == NULL || this->header_queue_tail->value == NULL)
        {
            throw invalid_argument("Invalid header");
        }
        // Send header line.
        if (dprintf(this->socket_fd, "%s: %s\r\n", this->header_queue_tail->name, this->header_queue_tail->value) < 0)
        {
            throw runtime_error("Send header failed");
        }

        // Pop queue.
        current = this->header_queue_tail;
        this->header_queue_tail = this->header_queue_tail->next;
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

void http_response::enqueue_general_headers()
{
    // Connection.
    push_header("Connection", "close");

    // Date.
    // Sun, 06 Nov 1994 08:49:37 GMT
    ostringstream date_str;
    time_t now = time(nullptr);
    tm *gmt = gmtime(&now);
    date_str << put_time(gmt, "%a, %d %b %Y %H:%M:%S GMT");
    push_header("Date", date_str.str().c_str());
}

void http_response::enqueue_server_headers()
{
    // Server.
    push_header("Server", LIBHTTP_HTTP_SERVER_STRING);
}

// Open fd from this.path and send copy content to socket fd.
// Only send file content, headers should be handled before invoke.
// Assuming the path leads to a valid file.
void http_response::send_file()
{
    // Get file descriptor.
    ostringstream err_msg;
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
                err_msg << "Error writing to socket: " << this->socket_fd << " (" << strerror(errno) << ")" << endl;
                throw runtime_error(err_msg.str());
            }
            n_buffered -= n_written;
            write_buffer_ptr += n_written;
        }
    }
}

void http_response::send_str()
{
    if (this->str == NULL)
    {
        throw invalid_argument("Null string pointer");
    }

    if (dprintf(this->socket_fd, this->str) < 0)
    {
        throw runtime_error("Error writing to socket: " + to_string(this->socket_fd) + " (" + strerror(errno) + ")");
    }
}

// Copy path to this->path.
// Does not validate path.
// format: "path/to/file".
void http_response::set_path(const char *path)
{
    size_t path_len;
    path_len = strlen(path);
    if ((this->path = (char *)malloc(path_len + 1)) == NULL)
    {
        throw bad_alloc();
    }
    strcpy(this->path, path);
}

void http_response::set_msg(const string &msg)
{
    this->msg = msg;
}

// Only support sending files and file lists for now.
// Assumes path is correctly parsed.
// format: "path/to/file".
void http_response::serve_file()
{
    // Check whether file exists.
    if (!fs::exists(this->path))
    {
        throw invalid_argument("Nonexistent file");
    }
    if (!fs::is_regular_file(this->path))
    {
        throw invalid_argument("Not regular file");
    }
    if (fs::is_directory(this->path))
    {
        char *index_path = (char *)malloc(strlen(this->path) + 10);
        if (fs::exists(index_path))
        {
            set_path(index_path);
            free(index_path);
            serve_file();
            return;
        }
        else
        {
            serve_dir();
            return;
        }
    }

    // Set status code 200 OK.
    set_status(200);

    // Enqueue headers.
    enqueue_general_headers();
    enqueue_server_headers();

    // Enqueue entity headers.
    size_t size = fs::file_size(this->path);
    push_header("Content-Length", to_string(size).c_str());

    push_header("Content-Type", http_get_mime_type(fs::path(this->path).extension()).c_str());

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

// Assumes path is correctly parsed.
void http_response::serve_dir()
{
    // Check whether file exists.
    if (!fs::exists(this->path))
    {
        throw invalid_argument("Nonexistent directory");
    }
    if (!fs::is_directory(this->path))
    {
        throw invalid_argument("Not directory");
    }

    ostringstream sstr;
    sstr << "<html>\n";
    sstr << "<head><title>Index of /" << this->path << "</title></head>\n";
    sstr << "<body>\n";
    sstr << "<h1>Index of /" << this->path << "</h1><hr><pre>" << "<a href=\"../\">../</a>\n";

    try
    {
        // Open directory and query files.
        for (const auto &entry : fs::directory_iterator(this->path))
        {
            if (entry.path().filename() == ".")
            {
                continue;
            }
            sstr << http_format_herf(this->path, entry.path().filename()) << "\n";
        }
    }
    catch (const fs::filesystem_error &e)
    {
        cerr << "Filesystem error caught: " << this->path << " (" << e.what() << ")" << endl;
        throw runtime_error("libhttp: send request failed");
    }

    sstr << "</pre><hr></body>\n";
    sstr << "</html>\n";

    this->str = sstr.str().c_str();

    // Set status 200 OK.
    set_status(200);

    // Enqueue headers.
    enqueue_general_headers();
    enqueue_server_headers();

    // Enqueue entity headers.
    push_header("Content-Length", to_string(strlen(this->str)).c_str());

    push_header("Content-Type", "text/html");

    try
    {
        send_status_line();
        send_headers();
        end_headers();
        send_str();
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

// Send msg in this->msg.
// Does not set default status code.
// Use http_response::set_status() to set status code before invoke.
void http_response::serve_msg()
{
    ostringstream smsg;
    smsg << "<html>\n";
    smsg << "<body>\n";
    smsg << "<p>\n";
    smsg << this->msg;
    smsg << "</p>\n";
    smsg << "</body>\n";
    smsg << "</html>\n";

    this->str = smsg.str().c_str();

    // Enqueue headers.
    enqueue_general_headers();
    enqueue_server_headers();

    // Enqueue entity headers.
    push_header("Content-Length", to_string(strlen(this->str)).c_str());

    push_header("Content-Type", "text/html");

    try
    {
        send_status_line();
        send_headers();
        end_headers();
        if (!this->msg.empty())
        {
            send_str();
        }
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

string http_response::http_format_herf(const string &path, const string &filename)
{
    return "<a href=\"" + path + "/" + filename + "\">" + filename + "</a>";
}

string http_response::http_get_mime_type(const string &extension)
{
    if (extension.empty())
    {
        return "text/plain";
    }
    else if (extension == ".txt")
    {
        return "text/plain";
    }
    else if (extension == ".html")
    {
        return "text/html";
    }
    else if (extension == ".jpg")
    {
        return "image/jpeg";
    }
    else if (extension == ".jpeg")
    {
        return "image/jpeg";
    }
    else if (extension == ".png")
    {
        return "image/png";
    }
    else if (extension == ".css")
    {
        return "text/css";
    }
    else if (extension == ".js")
    {
        return "application/javascript";
    }
    else if (extension == ".jpg")
    {
        return "application/pdf";
    }
    else
    {
        return "text/plain";
    }
}