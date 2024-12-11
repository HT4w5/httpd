/*
 * @Author: HT4w5 example@example.com
 * @Date: 2024-12-11 10:53:57
 * @LastEditors: HT4w5 example@example.com
 * @LastEditTime: 2024-12-11 12:47:03
 * @FilePath: /httpd/libhttp.cpp
 * @Description: Simple implemention of the http protocol.
 */

#include "libhttp.hpp"
#include <iostream>
#include <string>
#include <unordered_map>
#include <fcntl.h>
#include <unistd.h>

using namespace std;

/**
 * http_request
 */

http_request::http_request(int sockfd)
{
    this->sockfd = sockfd;
}

http_request::~http_request()
{
}

int http_request::parse()
{
    
}