/*
 * @Author: HT4w5 example@example.com
 * @Date: 2024-12-11 10:48:58
 * @LastEditors: HT4w5 example@example.com
 * @LastEditTime: 2024-12-11 11:05:15
 * @FilePath: /httpd/httpd.cpp
 * @Description: Simple http server.
 */

#include <iostream>
#include <cstring>
#include <cstdlib>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>

// Server opmodes.
#define SRV_FILE (1 << 0)
#define SRV_DIR (1 << 1)
#define SRV_PROX (1 << 2)

using namespace std;

void error_die(const string &msg);
int server_init(unsigned short port);
int listen(int server_fd, unsigned short port, char opmode);
void handle_request(int client_fd, char opmode);
int handle_file_request(int client_fd, char opmode);
int handle_proxy_request(int client_fd);


int main(int argc, char *argv)
{
    // TODO: argparse.

    // Command line arguments.
    string path;
    unsigned short port;
    char opmode;

    // Temporarily set arguments manually.
    path = "www/";
    port = 8080;
    opmode = SRV_FILE;

    // Change directory if not in proxy mode.
    if (!(opmode & SRV_PROX))
    {
        chdir(path.c_str());
    }

    // Init server.
    int server_fd;
    server_fd = server_init(port);
    if (server_fd < 0)
    {
        error_die("Failed to initialize server");
    }

    // Listen for requests.
    listen(server_fd, opmode);

    // Close socket.
    if (close(server_fd) < 0)
    {
        cerr << "Failed to close server socket (ignoring)" << endl;
    }
    return 0;
}

void error_die(const string &msg)
{
    cerr << "Fatal error: " << msg << endl;
    exit(EXIT_FAILURE);
}

int server_init(unsigned short port)
{
    // Create socket.
    int server_fd;
    server_fd = socket(AF_INET, SOCK_STREAM, 0); // IPv4, TCP.
    if (server_fd < 0)
    {
        cerr << "Socket creation failed" << endl;
        return -1;
    }

    // Configure socket.
    const int sockopt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof(sockopt)) < 0)
    {
        cerr << "Failed to configure socket" << endl;
        return -1;
    }

    // Set bind arguments.
    // Create socket struct.
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // Bind address to socket.
    if (bind(server_fd, (struct sockaddr *)&server_addr, (socklen_t)sizeof(server_addr)) < 0)
    {
        cerr << "Failed to bind address to socket" << endl;
        return -1;
    }

    return 0;
}

int listen(int server_fd, unsigned short port, char opmode)
{
    // Listen on socket.
    if (listen(server_fd, SOMAXCONN) < 0)
    {
        cerr << "Listen failed on port " << port << endl;
        return -1;
    }

    cout << "Listening on port " << port << endl;

    // Wait for requests.
    sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int client_fd;
    while (true)
    {
        // Accept connection.
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_fd < 0)
        {
            cerr << "Error accepting socket" << endl;
            continue;
        }

        cout << "Accepted connection from "
             << inet_ntoa(client_addr.sin_addr)
             << " on port "
             << client_addr.sin_port << endl;

        // Create new handler thread.
        thread request_handler(handle_request, client_fd, opmode);
    }

    if (shutdown(server_fd, SHUT_RDWR) < 0)
    {
        cerr << "Failed to shutdown all connections (ignoring)" << endl;
    }
    return 0;
}

void handle_request(int client_fd, char opmode)
{
    // Check opmode.
    // Assumes opmode combination is valid.
    // Please validate opmode when parsing arguments.
    if (opmode && SRV_PROX) // Proxy mode.
    {
        handle_proxy_request(client_fd);
    }
    else // File serve mode.
    {
        /**
         * Reserve opmode to determine whether or not to 
         * send directory list.
         */
        handle_file_request(client_fd, opmode);
    }
}

int handle_file_request(int client_fd, char opmode)
{
    
}


int handle_proxy_request(int client_fd)
{
    // To be implemented.
    return 0;
}