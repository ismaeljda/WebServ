#include "Server.hpp"

Server::Server(const ServerConfig &conf) : config(conf){
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
        throw std::runtime_error("Erreur: Creation Socket a echoue");

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    std::cout << "le port config.listen est " << conf.listen << std::endl;
    addr.sin_port = htons(conf.listen);
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
	    perror("setsockopt");
	    exit(EXIT_FAILURE);
    }
    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
	perror("bind");
	exit(EXIT_FAILURE);
    }
    std::cout << "bind OK" << std::endl;
    if (listen(server_fd, 10) < 0) {
	perror("listen");
	exit(EXIT_FAILURE);
    }
    std::cout << "listen OK" << std::endl;
    std::cout << "Server lancé sur le port : " << conf.listen << std::endl;

    server_pollfd.fd = server_fd;
    server_pollfd.events = POLLIN;
    server_pollfd.revents = 0;
    fds.push_back(server_pollfd);
}

Server::~Server()
{
    close(server_fd);
}

void Server::run()
{
    std::cout << "Le serveur entre dans run() sur le port : " << config.listen << std::endl;
    std::cout << "fds[0].fd = " << fds[0].fd << ", attendu : " << server_fd << std::endl;

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
    ConfigParser parser("config.conf");
    parser.validateServers();
    const std::vector<ServerConfig> &servers = parser.getServers();
    std::cout << "Nombre de serveurs parsés : " << servers.size() << std::endl;

    std::vector<Server*> serverInstances;
    for (size_t i = 0; i < servers.size(); ++i)
        serverInstances.push_back(new Server(servers[i]));

    for (size_t i = 0; i < serverInstances.size(); ++i)
        serverInstances[i]->run();

    for (size_t i = 0; i < serverInstances.size(); ++i)
        delete serverInstances[i];
}