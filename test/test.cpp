#include "test.hpp"

Server::Server(int port)
{
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) 
    {
        std::cout << "Failed to create socket." << std::endl;
        exit(EXIT_FAILURE);
    }
    sockaddr_in sockaddr; // Ici sockaddr_in pour stocker les infos des addresses ip
    sockaddr.sin_family = AF_INET; //pour ip_V4
    sockaddr.sin_addr.s_addr = INADDR_ANY; //INADDR_ANY pour lier la socket a toutes les addresses disponibles sur le reseau
    sockaddr.sin_port = htons(port); // On ecoute sur le port 9999et htons c'est pour convertir le port en un format reseau

    //Ici on va associer la socket a une addresse ip et un port pour qu'elle puisse ecouter les connexions a partir de cette addresse et de ce port local
    if (bind(sockfd, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) < 0) 
    {
        std::cout << "Failed to bind to port " << port << std::endl;
        exit(EXIT_FAILURE);
    }
    //Ici pour accepter les connexions sur la socket avec un max de 10 connexions dans la file d'attente
    if (listen(sockfd, 10) < 0) 
    {
        std::cout << "Failed to listen on socket." << std::endl;
        exit(EXIT_FAILURE);
    }
}
Server::~Server()
{
    close(sockfd);
}

void Server::run()
{
    sockaddr_in clientAddr;
    socklen_t addrlen = sizeof(clientAddr);
    int connection = accept(sockfd, (struct sockaddr*)&clientAddr, &addrlen);
    if (connection < 0) 
    {
        std::cout << "Failed to grab connection" << std::endl;
        exit(EXIT_FAILURE);
    }
    char buffer[100];
    size_t bytesRead = read(connection, buffer, 100);
    std::cout << "The message was: " << buffer;

    std::string response = "Good talking to you\n";
    send(connection, response.c_str(), response.size(), 0);

    close(connection);
}
int main()
{
    Server serv(9999);
    serv.run();
}