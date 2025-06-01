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
#include <string>
#include <arpa/inet.h>
#include "../Request/RequestParser.hpp"
#include "../Config/ConfigParser.hpp"

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
        std::vector<pollfd> fds;
        pollfd server_pollfd;
        ServerConfig config;

        std::string response_html;             // pour stocker les headers HTTP
        std::vector<char> response_body;       // pour stocker le corps binaire (image, etc.)

        Server(const Server &copy);
        Server &operator=(const Server &copy);
    public:
        Server(const ServerConfig &conf);
        type handle_request(RequestParser& req);
        ~Server();
        void run();
};

#endif
