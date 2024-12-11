/*
 * @Author: HT4w5 example@example.com
 * @Date: 2024-12-11 10:48:58
 * @LastEditors: HT4w5 example@example.com
 * @LastEditTime: 2024-12-11 12:47:04
 * @FilePath: /httpd/libhttp.hpp
 * @Description: Simple implemention of the http protocol.
 */

#ifndef LIB_HTTP_H
#define LIB_HTTP_H

#include <iostream>
#include <string>
#include <unordered_map>
#include <fcntl.h>
#include <unistd.h>

using namespace std;

// HTTP methods.
#define HTTP_GET (1 << 0)
#define HTTP_POST (1 << 1)
#define HTTP_PUT (1 << 2)
// To be finished.

#define MAX_RECV_BUF

class http_request
{
private:
    // Classes.

    class http_request_headers
    {
    public:
        void append(const string &name, const string &value);
        string query(const string &name);

    private:
        unordered_map<string, string> headers;
    };

    // Methods.

    http_request(int sockfd);
    ~http_request();

    // Variables.

    int sockfd;

public:
    // Methods.
    int parse();
    // Variables.
    char method;
    string path;
    string version;
};

#endif