#ifndef SERVER_HPP
# define SERVER_HPP

#include <poll.h>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <fstream>
#include <unistd.h>
#include <string.h>
#include "../Request/RequestParser.hpp"
std::string extractMimeType(std::string& headerValue);

enum type {
    GET = 0,          
    POST = 1,            
    DELETE = 2     
}; 
class Server
{
    private:
        int server_fd;
        // int port;
        std::vector<pollfd> fds;
        pollfd server_pollfd;
        std::string response_html;

    public:
        Server(int port);
        type handle_request(RequestParser& req);
        ~Server();
        void run();
};

#endif 