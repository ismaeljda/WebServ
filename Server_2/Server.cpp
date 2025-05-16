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
                        RequestParser request;
                        std::string req = buffer;
                        request.parse(req);
                        // request.display_request();
                        if(handle_request(request) == GET)
                        {
                            // envoi header
                            send(fds[i].fd, response_html.c_str(), response_html.size(), 0);
                            // envoi corps binaire
                            if (!response_body.empty())
                            {
                                send(fds[i].fd, &response_body[0], response_body.size(), 0);
                                response_body.clear();
                            }
                        }
                        close(fds[i].fd);
                        fds.erase(fds.begin() + i);
                        --i;
                    }
                }
            }
        }
    }
}

std::string getMimeType(const std::string& path) {
    if (path.find(".html") != std::string::npos)
        return "text/html";
    else if (path.find(".css") != std::string::npos)
        return "text/css";
    else if (path.find(".js") != std::string::npos)
        return "application/javascript";
    else if (path.find(".jpg") != std::string::npos || path.find(".jpeg") != std::string::npos)
        return "image/jpeg";
    else if (path.find(".png") != std::string::npos)
        return "image/png";
    else if (path.find(".gif") != std::string::npos)
        return "image/gif";
    else if (path.find(".svg") != std::string::npos)
        return "image/svg+xml";
    else
        return "application/octet-stream"; // par défaut
}

type Server::handle_request(RequestParser& req)
{
    if (req.getMethod() == "GET")
    {
        std::string uri = req.getUri();
        if (uri == "/")
            uri = "/index.html";  // page par défaut

        std::string path = "www" + uri;

        std::ifstream file(path.c_str(), std::ios::binary);
        if (!file.is_open())
        {
            // fichier non trouvé : envoyer une 404 simple
            std::string not_found_response = 
                "HTTP/1.1 404 Not Found\r\n"
                "Content-Type: text/html\r\n"
                "Content-Length: 13\r\n"
                "\r\n"
                "404 Not Found\n";
            response_html = not_found_response;
            return GET;
        }

        // lire le fichier en binaire
        std::vector<char> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();

        std::string mime_type = getMimeType(path);

        std::stringstream header;
        header << "HTTP/1.1 200 OK\r\n";
        header << "Content-Type: " << mime_type << "\r\n";
        header << "Content-Length: " << buffer.size() << "\r\n";
        header << "\r\n";

        response_html = header.str();

        // maintenant il faut envoyer la réponse complète (header + contenu binaire)
        // mais ici dans run() tu envoies directement response_html (string) => il faut modifier run() pour envoyer aussi les données binaires

        // Pour simplifier, on va stocker aussi le contenu binaire dans un membre de Server, par ex std::vector<char> response_body;
        response_body = buffer; // ajoute ce membre dans ta classe Server

        return GET;
    }
    else
    {
        return POST;
    }
}

int main()
{
    Server serv(9988);
    serv.run();
}