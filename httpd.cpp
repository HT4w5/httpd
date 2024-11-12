#include <string>
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <cstdio>
#include <cstdlib>
using namespace std;

/******************************
* TODO:
* Reimplement multithreading with c++ threads.
* Refactor error handing with exceptions.
******************************/

// Server properties.
string server_dir = "";
string origin_url = "";
int server_port = 80; // Default port.
int num_threads = 0;  // Unspecified.

// Print usage.
void print_usage()
{
    cout << "Usage:" << endl;
}

// Die on error.
void error_die(const string &msg)
{
    perror(msg.c_str());
    exit(1);
}

void *handle_file_request(void *client_fd)
{
    // TODO: Implement file request handling.
}

void *handle_proxy_request(void *client_fd)
{
    // TODO: Implement proxy request handling.
}

// Server initialization.
int server_init(int server_port)
{
    struct sockaddr_in server_addr;
    int server_fd;

    // Create socket.
    server_fd = socket(AF_INET, SOCK_STREAM, 0); // IPv4, TCP.
    if (server_fd < 0)
    {
        error_die("Socket creation failed.");
    }

    // Set socket option to reuse address.
    int socket_option = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &socket_option, sizeof(socket_option)) < 0)
    {
        error_die("Socket option setting failed.");
    }

    // Setup server address.
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(server_port);

    // Bind socket to address.
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        error_die("Socket binding failed.");
    }

    // Listen for incoming connections.
    if (listen(server_fd, SOMAXCONN) < 0)
    {
        error_die("Listen failed on port" + to_string(server_port) + ".");
    }

    return server_fd;
}

// Listen for incoming connections.
void listen_forever(int server_fd, void *(*request_handler)(void *))
{
    // Declare client address and length.
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd;

    // Declare thread arguments.
    pthread_t thread_id;

    // Accept incoming connections.
    while (1)
    {
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0)
        {
            error_die("Accept failed.");
        }
        // Create thread to handle request.
        if (pthread_create(&thread_id, NULL, request_handler, (void *)&client_fd) != 0)
        {
            perror("Thread creation failed.");
        }
    }
}

int main(int argc, char *argv[])
{
    void *(*request_handler)(void *) = NULL;

    // No args specified.
    if (argc == 1)
    {
        print_usage();
        return 1;
    }
    else if (argv[1] == "-h" || argv[1] == "--help")
    {
        print_usage();
        return 0;
    }

    // Process args.
    for (int i = 1; i < argc; ++i)
    {
        if (argv[i] == "--files" || argv[i] == "-f") // Files arguement.
        {
            server_dir = argv[i + 1];
            ++i;
        }
        else if (argv[i] == "--port" || argv[i] == "-p") // Port arguement.
        {
            try
            {
                server_port = stoi(argv[i + 1]);
            }
            catch (const exception &e)
            {
                cerr << "Invalid port number." << endl;
                return 1;
            }

            if (server_port < 1 || server_port > 65535)
            {
                cerr << "Invalid port number." << endl;
                return 1;
            }
            ++i;
        }
        else if (argv[i] == "--proxy" || argv[i] == "-x") // Proxy arguement.
        {
            origin_url = argv[i + 1];
            // TODO: Implement proxy support.
            ++i;
        }
        else if (argv[i] == "--threads" || argv[i] == "-t") // Threads arguement.
        {
            try
            {
                num_threads = stoi(argv[i + 1]);
            }
            catch (const exception &e)
            {
                cerr << "Invalid number of threads." << endl;
                return 1;
            }
            ++i;
        }
        else // Invalid arguement.
        {
            cerr << "Invalid arguement: " << argv[i] << "\n";
            print_usage();
            return 1;
        }
    }

    // Specified both files and proxy.
    if (!server_dir.empty() && !origin_url.empty())
    {
        cerr << "Cannot specify both files and proxy." << endl;
        return 1;
    }

    // No files or proxy specified.
    if (server_dir.empty() && origin_url.empty())
    {
        cerr << "Must specify either files or proxy." << endl;
        return 1;
    }

    if (!server_dir.empty())
    {
        if (origin_url.empty())
        {
            request_handler = handle_file_request;
        }
        else
        {
            cerr << "Cannot specify both files and proxy." << endl;
            return 1;
        }
    }
    else
    {
        if (!origin_url.empty())
        {
            request_handler = handle_proxy_request;
        }
        else
        {
            cerr << "Must specify either files or proxy." << endl;
            return 1;
        }
    }

    // Initialize server.
    int server_fd = server_init(server_port);
}