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
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <string>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <algorithm>
#include <dirent.h>
#include "../Request/RequestParser.hpp"
#include "../Config/ConfigParser.hpp"
#include "../Request/HttpStatusCodes.hpp"

class RequestParser;

std::string extractMimeType(std::string& headerValue);

enum type {
    GET = 1,          
    POST = 2,            
    DELETE = 3,
    NONE =  0  
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
        type handle_request(RequestParser &req);
        void handle_get(RequestParser &req);
        void handle_post(RequestParser &req);
        void handle_delete(RequestParser &req);
        ~Server();
        // void run();
        void acceptClient(std::vector<pollfd> &fds, std::map<int, Server*> &client_to_server);
        void handleClient(int fd);

        const LocationConfig* matchLocation(const std::string &uri) const;
        std::string makeErrorPage(int code) const;
        bool isDirectory(const std::string &path);
        std::string generateAutoindexHTML(const std::string &dir_path, const std::string &uri);

        bool isCgiRequest(const LocationConfig *loc, const std::string &uri);
        void handle_cgi(RequestParser &req);

        int getServerfd() const;
        int getPort() const;
};      

#endif
