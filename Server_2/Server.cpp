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

int Server::getServerfd() const {
    return server_fd;
}

int Server::getPort() const {
    return config.listen;
}

void Server::acceptClient(std::vector<pollfd> &fds, std::map<int, Server*> &client_to_server) {
    sockaddr_in clientAddr;
    socklen_t addrlen = sizeof(clientAddr);
    int client_fd = accept(server_fd, (struct sockaddr*)&clientAddr, &addrlen);

    if (client_fd < 0) {
        std::cerr << "Erreur: accept() a echoue sur le port : " << config.listen << std::endl;
        return; 
    }
    std::cout << "✅ Nouveau client connecté sur le port " << config.listen
	          << " (FD = " << client_fd << ")" << std::endl;
    pollfd client_pollfd;
    client_pollfd.fd = client_fd;
    client_pollfd.events = POLLIN;
    client_pollfd.revents = 0;
    fds.push_back(client_pollfd);

    client_to_server[client_fd] = this;
}

void Server::handleClient(int fd) {
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    ssize_t bytesRead = read(fd, buffer, sizeof(buffer) - 1);

    if (bytesRead <= 0) {
        std::cout << "Deconnexion client FD " << fd << std::endl;
        close(fd);
        return;
    }

    std::cout << "Requête reçue (FD " << fd << ") :\n" << buffer << std::endl;
    std::string req(buffer);
    RequestParser request;
    request.parse(req);

    type result = handle_request(request);
    if (result == GET || result == POST || result == DELETE || result == NONE) {
        send(fd, response_html.c_str(), response_html.size(), 0);
        if (!response_body.empty()){
            send(fd, &response_body[0], response_body.size(), 0);
            response_body.clear();
        }
    }
    close(fd);
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
void Server::handle_get(RequestParser &req)
{
    std::string uri = req.getUri();
    if (uri.empty())
        uri = "/";

    // Match la location la plus adaptée
    const LocationConfig* loc = matchLocation(uri);
    if (!loc) {
        response_html = makeErrorPage(404);
        return;
    }
    std::cout << "[REDIRECT] Vers : " << loc->redirect << std::endl;
    if (!loc->redirect.empty()) {
        std::ostringstream header;
        header << "HTTP/1.1 301 Moved Permanently\r\n";
        header << "Location: " << loc->redirect << "\r\n";
        header << "Content Length: 0\r\n";
        header << "\r\n";
        response_html = header.str();
        return;
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
        return;
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

void Server::handle_post(RequestParser &req) {
    const LocationConfig* loc = matchLocation(req.getUri());
    if (!loc) {
        response_html = makeErrorPage(404);
        return;
    }
    std::cout << "[POST] URI reçue : " << req.getUri() << std::endl;

    std::string body = req.getBody(); // récupère le body parsé
    std::string target_path;
    if (!loc->upload_store.empty())
        target_path = loc->upload_store + "/upload_result.txt";
    else
        target_path = loc->root + "/upload_result.txt";
    std::cout << "loc->uploadstore = " << loc->upload_store << std::endl;
    std::cout << "[POST] File will be saved to: " << target_path << std::endl;

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
    std::cout << "[DEBUG] URI: " << req.getUri() << std::endl;
    std::cout << "[DEBUG] Location matched: " << loc->path << std::endl;
    std::cout << "[DEBUG] Root: " << loc->root << std::endl;    
    std::cout << "[DEBUG] UploadStore: " << loc->upload_store << std::endl;

}

void Server::handle_delete(RequestParser &req) {
    const LocationConfig *loc = matchLocation(req.getUri());
    if (!loc) {
        response_html = makeErrorPage(404);
        return;
    }

    std::string uri = req.getUri();
    std::string relative_path = uri.substr(loc->path.length());
    std::string file_path = loc->root;
    if (!relative_path.empty() && relative_path[0] != '/')
        file_path += '/';
    file_path += relative_path;

    std::cout << "DELETE target file -> " << file_path << std::endl;

    if (access(file_path.c_str(), F_OK) == -1) {
        response_html = makeErrorPage(404);
        return;
    }
    if (remove(file_path.c_str()) == 0) {
        std::string body = "<html><body><h1>File deleted successfully.</h1></body></html>";
        std::ostringstream header;
        header << "HTTP/1.1 200 ok\r\n";
        header << "Content-Type: text/html\r\n";
        header << "Content-Length: " << body.size() << "\r\n";
        header << "\r\n";
        
        response_html = header.str() + body; 
    } else {
        response_html = makeErrorPage(403);
    }
}

type Server::handle_request(RequestParser &req)
{
    const LocationConfig *loc = matchLocation(req.getUri());
    if (!loc) {
        response_html = makeErrorPage(404);
        return NONE;
    }

    std::vector<std::string>::const_iterator it = std::find(loc->allow_methods.begin(), loc->allow_methods.end(), req.getMethod());
    if (it == loc->allow_methods.end()) {
        std::ostringstream oss;
        std::string body = "<html><body><h1>405 Method Not Allowed</h1></body></html>";
        oss << "HTTP/1.1 405 Method Not Allowed\r\n";
        oss << "Content-Type: text/html\r\n";
        oss << "Content-Length: " << body.size() << "\r\n";
        oss << "Allow: ";
        for (size_t i = 0; i < loc->allow_methods.size(); ++i) {
            oss << loc->allow_methods[i];
            if (i + 1 < loc->allow_methods.size())
                oss << ", ";
        }
        oss << "\r\n\r\n" << body;
        response_html = oss.str();
        return NONE;
    }
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
    if (req.getMethod() == "DELETE"){
        handle_delete(req);
        return DELETE;
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
