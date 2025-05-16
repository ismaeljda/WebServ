#ifndef SERVER_HPP
# define SERVER_HPP

#include <poll.h>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <string.h>

class Server
{
    private:
        int server_fd;
        int port;
        std::vector<pollfd> fds;
        pollfd server_pollfd;

    public:
        Server(int port);
        ~Server();
        void run();
};

#endif 