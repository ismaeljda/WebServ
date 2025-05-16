#include "Server.hpp"

Server::Server(int port)
{
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) 
    {
        std::cout << "Failed to create socket." << std::endl;
        exit(EXIT_FAILURE);
    }
    sockaddr_in sockaddr; // Ici sockaddr_in pour stocker les infos des addresses ip
    sockaddr.sin_family = AF_INET; //pour ip_V4
    sockaddr.sin_addr.s_addr = INADDR_ANY; //INADDR_ANY pour lier la socket a toutes les addresses disponibles sur le reseau
    sockaddr.sin_port = htons(port); // On ecoute sur le port 9999et htons c'est pour convertir le port en un format reseau

    //Ici on va associer la socket a une addresse ip et un port pour qu'elle puisse ecouter les connexions a partir de cette addresse et de ce port local
    if (bind(server_fd, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) < 0) 
    {
        std::cout << "Failed to bind to port " << port << std::endl;
        exit(EXIT_FAILURE);
    }

    //Ici pour accepter les connexions sur la socket avec un max de 10 connexions dans la file d'attente
    if (listen(server_fd, 10) < 0) 
    {
        std::cout << "Failed to listen on socket." << std::endl;
        exit(EXIT_FAILURE);
    }
    // initialisation du poll
    server_pollfd.fd = server_fd; // on associe la socket du serveur au poll pour qu'il puisse ecouter
    server_pollfd.events = POLLIN; // On surveille une possible nouvelle connexion, en gros ca previent si il y a une nouvelle connection sur le socket du server
    server_pollfd.revents = 0;     // on initalise a 0, ce champs sera rempli par poll pour dire ce qu'il s est passe

    fds.push_back(server_pollfd);
}

Server::~Server()
{
    close(server_fd);
}

void Server::run()
{
    while (true)
    {
        int ret = poll(&fds[0], fds.size(), -1);
        if (ret < 0)
        {
            std::cerr << "poll failed" << std::endl;
            break;
        }
        for (size_t i = 0; i < fds.size() ; ++i)
        {
            if (fds[i].revents & POLLIN) // si il y a qqch qui s est passe sur la socket et que y a qqch a lire sur la socket(POLLIN)
            {
                if (fds[i].fd == server_fd) // si le socket qui a hit c'est celle du server c est qu'il y a une nouvelle connection
                {
                    //On creer la socket du client
                    sockaddr_in clientAddr;
                    socklen_t addrlen = sizeof(clientAddr);
                    int client_fd = accept(server_fd, (struct sockaddr*)&clientAddr, &addrlen);
                    if (client_fd < 0) 
                    {
                        std::cout << "Failed to grab connection" << std::endl;
                        continue;
                    }
                    std::cout << "New client connected: FD = " << client_fd << std::endl;
                    // On ajoute la socket client au tableau de poll
                    pollfd client_pollfd;
                    client_pollfd.fd = client_fd;
                    client_pollfd.events = POLLIN;
                    client_pollfd.revents = 0;
                    fds.push_back(client_pollfd);
                }
                else
                {
                    char buffer[1024];
                    memset(buffer, 0, sizeof(buffer));
                    int bytesRead = read(fds[i].fd, buffer, sizeof(buffer));
                    if (bytesRead <= 0)
                    {
                        std::cout << "Client disconnected: FD = " << fds[i].fd << std::endl;
                        close(fds[i].fd);
                        fds.erase(fds.begin() + i);
                        --i;
                    }
                    else
                    {
                        std::cout << "Message from client: " << buffer;

                        std::string body = "<!DOCTYPE html><html><body><h1>Hello, world!</h1></body></html>";
                        std::string html = 
                            "HTTP/1.1 200 OK\r\n"
                            "Content-Type: text/html\r\n"
                            "Content-Length: " + std::to_string(body.size()) + "\r\n"
                            "\r\n" +
                            body;

                        send(fds[i].fd, html.c_str(), html.size(), 0);
                        close(fds[i].fd);
                        fds.erase(fds.begin() + i);
                        --i;
                    }
                }
            }
        }
    }
}

int main()
{
    Server serv(9996);
    serv.run();
}