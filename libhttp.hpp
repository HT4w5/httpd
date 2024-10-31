#ifndef LIBHTTP_H
#define LIBHTTP_H

#include <string>
#include <fstream>

using namespace std;

class http_request_handler
{
    public:
        string method;
        string url;
        string version;
        string headers;
        string body;
        http_request_handler(int socket_fd);
        ~http_request_handler();
        void parse();

    private:
        int socket_fd;
};



#endif