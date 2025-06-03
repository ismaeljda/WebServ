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
    if (listen(server_fd, 30) < 0) {
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
                        if(handle_request(request) == GET || handle_request(request) == POST)
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
void Server::handle_get(RequestParser& req)
{
    std::string uri = req.getUri();
    if (uri.empty())
        uri = "/";

    // Match la location la plus adaptée
    const LocationConfig* loc = matchLocation(uri);
    if (!loc) {
        response_html = makeErrorPage(404);
    }

    // Construction du chemin absolu à partir du root de la location
    std::string relative_path = uri.substr(loc->path.length());
    std::string file_path = loc->root;
    if (!relative_path.empty() && relative_path[0] != '/')
        file_path += '/';
    file_path += relative_path;

    // Si c’est un dossier ou qu’il finit par '/', on ajoute index
    struct stat sb;
    if (stat(file_path.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode)) {
        if (loc->index.empty()) {
            response_html = makeErrorPage(403); // pas de fichier index et directory_listing désactivé
        }
        if (file_path[file_path.size() - 1] != '/')
            file_path += '/';
        file_path += loc->index;
    }
    std::cout << "[DEBUG] URI: " << uri << std::endl;
    std::cout << "[DEBUG] Location matched: " << loc->path << std::endl;
    std::cout << "[DEBUG] Root: " << loc->root << std::endl;
    std::cout << "[DEBUG] File path final: " << file_path << std::endl;


    // Lire le fichier cible
    std::ifstream file(file_path.c_str(), std::ios::binary);
    if (!file.is_open()) {
        response_html = makeErrorPage(404);
    }

    std::vector<char> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    std::string mime = getMimeType(file_path);
    std::cout << "[DEBUG] MIME type: " << mime << std::endl;
    std::ostringstream header;
    header << "HTTP/1.1 200 OK\r\n";
    header << "Content-Type: " << mime << "\r\n";
    header << "Content-Length: " << buffer.size() << "\r\n";
    header << "\r\n";

    response_html = header.str();
    response_body = buffer;
}

void Server::handle_post(RequestParser& req) {
    const LocationConfig* loc = matchLocation(req.getUri());
    if (!loc) {
        response_html = makeErrorPage(404);
        return;
    }

    std::string body = req.getBody(); // récupère le body parsé
    std::string target_path = loc->root + "/upload_result.txt";

    std::ofstream out(target_path.c_str(), std::ios::app);
    if (!out.is_open()) {
        response_html = makeErrorPage(500);
        return;
    }

    out << body << "\n";
    out.close();

    std::string response_body = "<html><body>POST received and saved.</body></html>";
    std::ostringstream header;
    header << "HTTP/1.1 200 OK\r\n";
    header << "Content-Type: text/html\r\n";
    header << "Content-Length: " << response_body.size() << "\r\n";
    header << "\r\n";

    response_html = header.str() + response_body;
}

type Server::handle_request(RequestParser& req)
{
    if (req.getMethod() == "GET")
    {
        handle_get(req);
        return GET;
    }
    if (req.getMethod() == "POST")
    {
        handle_post(req);
        return POST;
    }
    return NONE;

}

const LocationConfig* Server::matchLocation(const std::string& uri) const
{
    const LocationConfig* best_match = NULL;
    size_t best_length = 0;

    for (size_t i = 0; i < config.locations.size(); ++i) {
        const LocationConfig& loc = config.locations[i];
        const std::string& path = loc.path;

        if (uri.compare(0, path.size(), path) == 0 && 
            (uri.size() == path.size() || uri[path.size()] == '/' || path[path.size() - 1] == '/')) {
            if (path.size() > best_length) {
                best_match = &loc;
                best_length = path.size();
            }
        }
    }

    return best_match;
}

bool Server::isDirectory(const std::string &path) {
    struct stat statbuf;

    if (stat(path.c_str(), &statbuf) != 0)
        return false;
    return S_ISDIR(statbuf.st_mode);
}

std::string Server::makeErrorPage(int code) const {
    std::string body;
    std::string filepath;

    std::map<int, std::string>::const_iterator it = config.error_pages.find(code);
    if (it != config.error_pages.end()) {
        filepath = it->second;
        std::ifstream file(filepath.c_str(), std::ios::binary);
        if (file.is_open()) {
            std::ostringstream ss;
            ss << file.rdbuf();
            body = ss.str();
            file.close();
        }
    }

    if (body.empty()) {
        std::ostringstream ss;
        ss << "<html><head><title>" << code << " Error</title></head>"
           << "<body><h1>" << code << " " << HttpStatusCodes::getMessage((HttpStatusCodes::Code)code) << "</h1></body></html>";
        body = ss.str();
    }

    std::ostringstream header;
    header << "HTTP/1.1 " << code << " " << HttpStatusCodes::getMessage((HttpStatusCodes::Code)code) << "\r\n";
    header << "Content-Type: text/html\r\n";
    header << "Content-Length: " << body.size() << "\r\n";
    header << "\r\n";
    header << body;

    return header.str();
}
