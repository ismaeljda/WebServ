#include "Server.hpp"
#include "../Request/utils.hpp"

Server::Server(const ServerConfig &conf) : config(conf){
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
        throw std::runtime_error("Erreur: Creation Socket a echoue");

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
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
    if (listen(server_fd, 30) < 0) {
	perror("listen");
	exit(EXIT_FAILURE);
    }
    server_pollfd.fd = server_fd;
    server_pollfd.events = POLLIN;
    server_pollfd.revents = 0;
    fds.push_back(server_pollfd);
}

Server::~Server()
{
    std::cout << "🧹 Destruction du serveur (port " << config.listen << ")..." << std::endl;
    
    if (server_fd >= 0) {
        int result = close(server_fd);
        if (result == -1) {
            std::cerr << "⚠️ Erreur fermeture socket: " << strerror(errno) << std::endl;
        }
        server_fd = -1;
    }
    
    if (!response_body.empty()) {
        response_body.clear();
        std::vector<char>().swap(response_body);
    }
    
    if (!response_html.empty()) {
        response_html.clear();
        std::string().swap(response_html);
    }
    
    // Pour Fermer tous les fd dans fds
    for (std::vector<pollfd>::iterator it = fds.begin(); it != fds.end(); ++it) {
        if (it->fd >= 0 && it->fd != server_fd) {  // Si Pas déjà fermé
            close(it->fd);
            it->fd = -1;
        }
    }
    fds.clear();
    std::vector<pollfd>().swap(fds);  // Pour Force la libération mémoire
    
    std::cout << "✅ Serveur détruit proprement (port " << config.listen << ")" << std::endl;
}

int Server::getServerfd() const 
{
    return server_fd;
}

int Server::getPort() const 
{
    return config.listen;
}
////////constructeur + destructeur + getter ↑↑↑↑   /////////////////////////////////////////////////////////////////////////////////////////////

void Server::acceptClient(std::vector<pollfd> &fds, std::map<int, Server*> &client_to_server) 
{
    sockaddr_in clientAddr;
    socklen_t addrlen = sizeof(clientAddr);
    int client_fd = accept(server_fd, (struct sockaddr*)&clientAddr, &addrlen);

    if (client_fd < 0) {
        std::cerr << "Erreur: accept() a echoue sur le port : " << config.listen << std::endl;
        return; 
    }
    pollfd client_pollfd;
    client_pollfd.fd = client_fd;
    client_pollfd.events = POLLIN;
    client_pollfd.revents = 0;
    fds.push_back(client_pollfd);

    client_to_server[client_fd] = this;
}


void Server::handleClient(int fd) 
{
    std::string requestStr;
    char buffer[4096];
    ssize_t bytesRead;
    size_t total_read = 0;
    size_t content_length = 0;
    bool headerParsed = false;

    while ((bytesRead = read(fd, buffer, sizeof(buffer))) > 0) 
    {
        requestStr.append(buffer, bytesRead);
        total_read += bytesRead;
        if (!headerParsed) {
            size_t headerEnd = requestStr.find("\r\n\r\n");
            if (headerEnd != std::string::npos) 
            {
                headerParsed = true;

                std::string headers = requestStr.substr(0, headerEnd);
                std::istringstream iss(headers);
                std::string line;
                while (std::getline(iss, line)) 
                {
                    if (line.find("Content-Length:") != std::string::npos) 
                    {
                        std::string lenStr = line.substr(line.find(":") + 1);
                        content_length = std::atoi(Utils::trim(lenStr).c_str());
                    }
                }
                if (content_length == 0)
                    break;
            } 
        }
        if (headerParsed) 
        {
            size_t bodyStart = requestStr.find("\r\n\r\n") + 4;
            size_t bodySize = requestStr.size() - bodyStart;
            if (bodySize >= content_length)
                break;
        }
    }
    if (bytesRead < 0) 
    {
        std::cerr << "Error reading fd" << fd << std::endl;
        close(fd);
        return;
    }
    std::cout << "Received request (FD " << fd << ")" << std::endl;
    // std::cout << requestStr.substr(0, requestStr.find("\r\n\r\n") + 4) << std::endl;
    RequestParser request;
    if (!request.parse(requestStr, this, response_html)) 
    {
        send(fd, response_html.c_str(), response_html.size(), 0);
        std::cerr << "Keep-Alive" << std::endl;
        close(fd);
        return;
    }

    const LocationConfig *loc = matchLocation(request.getUri());
    if (loc && request.getBody().size() > config.client_max_body_size) 
    {
        std::string error = makeErrorPage(413);
        send(fd, error.c_str(), error.size(), 0);
        close(fd);
        return;
    }

    type methodType = handle_request(request);
    if (methodType == GET ||  methodType == POST || methodType == DELETE) {
        send(fd, response_html.c_str(), response_html.size(), 0);
        if (!response_body.empty()) 
        {
            send(fd, &response_body[0], response_body.size(), 0);
            response_body.clear();
        }
        response_html.clear();
    }
    close(fd);
}
//////// Fonction pour gerer les nouveau client ↑↑↑↑↑↑↑ /////////////////////////////////////////////////////////////////////////////////////////////

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
/////////// fonction pour trouver le type qui precede le '.' ↑↑↑↑↑///////////////////////////////////////////////////

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
    if (!loc->redirect.empty()) {
        std::ostringstream header;
        header << "HTTP/1.1 301 Moved Permanently\r\n";
        header << "Location: " << loc->redirect << "\r\n";
        header << "Content-Length: 0\r\n";
        header << "Connection: close\r\n";
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
        if (!loc->index.empty()) {
            if (file_path[file_path.size() - 1] != '/')
                file_path += '/';
            file_path += loc->index;
        } else if (loc->directory_listing) {
            response_html = generateAutoindexHTML(file_path, req.getUri());
            return;
        } else {
            response_html = makeErrorPage(403);
            return;
        }
    }

    // Lire et ouvrir le fichier
    std::ifstream file(file_path.c_str(), std::ios::binary);
    if (!file.is_open()) {
        response_html = makeErrorPage(404);
        return;
    }

    std::vector<char> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    std::ostringstream header;
    header << "HTTP/1.1 200 OK\r\n";
    header << "Content-Type: " << getMimeType(file_path) << "; charset=UTF-8\r\n";
    header << "Content-Length: " << buffer.size() << "\r\n";
    header << "Connection: close\r\n";
    header << "\r\n";

    response_html = header.str();
    response_body = buffer;
}

void Server::handle_post(RequestParser &req) 
{
    const LocationConfig* loc = matchLocation(req.getUri());
    if (!loc) 
    {
        response_html = makeErrorPage(404);
        return;
    }
    std::string body = req.getBody();

    if (body.size() > config.client_max_body_size) 
    {
        std::cout << "[413] Payload Too Large (" << body.size() << " > " << config.client_max_body_size << ")" << std::endl;
        response_html = makeErrorPage(413);
        return;
    }
    std::string target_path;
    if (!loc->upload_store.empty())
        target_path = loc->upload_store + "/upload_result.txt";
    else
        target_path = loc->root + "/upload_result.txt";

    std::ofstream out(target_path.c_str(), std::ios::app);
    if (!out.is_open()) 
    {
        response_html = makeErrorPage(500);
        return;
    }

    out << body << "\n";
    out.close();
    std::string response_body_content = "<html><body>POST received and saved.</body></html>";
    std::ostringstream header;
    header << "HTTP/1.1 200 OK\r\n";
    header << "Server: webserv/1.0\r\n";
    header << "Content-Type: text/html\r\n";
    header << "Content-Length: " << response_body_content.size() << "\r\n";
    header << "Connection: close\r\n";
    header << "\r\n";

    response_html = header.str() + response_body_content;
}

void Server::handle_delete(RequestParser &req) 
{
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

    if (access(file_path.c_str(), F_OK) == -1) {
        response_html = makeErrorPage(404);
        return;
    }
    if (remove(file_path.c_str()) == 0) {
    std::string body_content = "<html><body><h1>File deleted successfully.</h1></body></html>";
    std::ostringstream header;
    header << "HTTP/1.1 200 OK\r\n";
    header << "Server: webserv/1.0\r\n";
    header << "Content-Type: text/html\r\n";
    header << "Content-Length: " << body_content.size() << "\r\n";
    header << "Connection: close\r\n";
    header << "\r\n";
    
    response_html = header.str() + body_content;
    }
}

type Server::handle_request(RequestParser &req)
{
    const LocationConfig *loc = matchLocation(req.getUri());
    if (!loc) 
    {
        response_html = makeErrorPage(404);
        return NONE;
    }
    if (req.getMethod() == "HEAD") 
    {
        handle_get(req);
        response_body.clear();
        return GET;
    }
    if (req.getMethod() == "GET")
    {
        if (isCgiRequest(loc, req.getUri()))
        {
            handle_cgi(req);
            return GET;
        }
        handle_get(req);
        return GET;
    }
    if (req.getMethod() == "POST")
    {
        if (isCgiRequest(loc, req.getUri()))
            handle_cgi(req);
        else 
            handle_post(req);
        return POST;
    }
    if (req.getMethod() == "DELETE"){
        handle_delete(req);
        return DELETE;
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
    return NONE;
}
//////// Fonction pour gerer request -> GET / POST / DELETE ↑↑↑↑↑↑↑↑↑ //////////////////////////////////////////////////

std::string Server::generateAutoindexHTML(const std::string &dir_path, const std::string &uri) 
{
	DIR *dir = opendir(dir_path.c_str());
	if (!dir)
		return makeErrorPage(403);

	std::ostringstream body;
	body << "<html><head><title>Index of " << uri << "</title></head><body>";
	body << "<h1>Index of " << uri << "</h1><ul>";

	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL) {
		std::string name(entry->d_name);
		if (name == ".") continue;
		body << "<li><a href=\"" 
     << (uri[uri.size() - 1] == '/' ? uri : uri + "/")
     << name << "\">" << name << "</a></li>";
	}
    closedir(dir);
    body << "</ul></body></html>";

    std::ostringstream header;
    header << "HTTP/1.1 200 ok\r\n";
    header << "Content-Type: text/html\r\n";
    header << "Content-Length: " << body.str().size() << "\r\n";
    header << "\r\n";

    return header.str() + body.str();
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

///////////////////////// fonction utils ↑↑↑↑↑↑↑↑//////////////////////////////////////////////////////////////////////////////
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
    
    if (body.empty()) 
    {
        std::ostringstream ss;
        std::string error_message = "Error";
        
        // Essayer d'obtenir le message d'erreur, avec fallback
        try {
            error_message = HttpStatusCodes::getMessage((HttpStatusCodes::Code)code);
        } catch (...) {
            // Si getMessage échoue ca utilise des messages standard
            switch(code) {
                case 400: error_message = "Bad Request"; break;
                case 403: error_message = "Forbidden"; break;
                case 404: error_message = "Not Found"; break;
                case 405: error_message = "Method Not Allowed"; break;
                case 413: error_message = "Payload Too Large"; break;
                case 500: error_message = "Internal Server Error"; break;
                default: error_message = "Error"; break;
            }
        }
        
        ss << "<html><head><title>" << code << " " << error_message << "</title></head>"
           << "<body><h1>" << code << " " << error_message << "</h1></body></html>";
        body = ss.str();
    }

    std::ostringstream header;
    header << "HTTP/1.1 " << code << " ";
    
    // Message d'erreur sécurisé
    try {
        header << HttpStatusCodes::getMessage((HttpStatusCodes::Code)code);
    } catch (...) {
        switch(code) {
            case 400: header << "Bad Request"; break;
            case 403: header << "Forbidden"; break;
            case 404: header << "Not Found"; break;
            case 405: header << "Method Not Allowed"; break;
            case 413: header << "Payload Too Large"; break;
            case 500: header << "Internal Server Error"; break;
            default: header << "Error"; break;
        }
    }
    
    header << "\r\n";
    header << "Server: webserv/1.0\r\n";
    header << "Content-Type: text/html\r\n";
    header << "Content-Length: " << body.size() << "\r\n";
    header << "Connection: close\r\n";
    header << "\r\n";
    header << body;

    return header.str();
}

////////////////////////// Fonction pour les Pages Erreur pour le serveur ↑↑↑↑↑↑↑↑//////////////////////////////////////////////////////////
//////////////Fonction Pour CGI below : /////////////////////////////////////////////////////////////////

bool Server::isCgiRequest(const LocationConfig *loc, const std::string &uri) 
{
    if (!loc || loc->cgi_pass.empty())
        return false;

    // Si l'extension est explicite
    for (size_t i = 0; i < loc->cgi_extensions.size(); ++i) 
    {
        const std::string &ext = loc->cgi_extensions[i];
        if (uri.size() >= ext.size() &&
            uri.compare(uri.size() - ext.size(), ext.size(), ext) == 0)
            return true;
    }

    // Si URI exactement égal au path d’une location avec cgi_pass
    if (uri == loc->path && !loc->cgi_pass.empty())
        return true;

    return false;
}

void Server::handle_cgi(RequestParser &req) {
    const LocationConfig *loc = matchLocation(req.getUri());
    if (!loc) {
        response_html = makeErrorPage(404);
        return;
    }

    std::string scriptPath = loc->cgi_pass;

    // Construction du chemin du script
    if (loc->cgi_pass[loc->cgi_pass.size() - 1] == '/') {
        std::string relative_path = req.getUri().substr(loc->path.length());
        if (!relative_path.empty() && relative_path[0] == '/')
            relative_path = relative_path.substr(1);
        scriptPath = loc->cgi_pass + relative_path;
    }
    
    // Vérifie que le script existe + exécutable
    if (access(scriptPath.c_str(), X_OK) != 0) {
        std::cerr << "[CGI] Script non trouvé ou non exécutable: " << scriptPath << std::endl;
        response_html = makeErrorPage(404);
        return;
    }

    //Vérifie la taille du body
    const size_t MAX_BODY_SIZE = 10 * 1024 * 1024; // 10MB max
    if (req.getBody().size() > MAX_BODY_SIZE) {
        std::cerr << "[CGI] Body trop volumineux: " << req.getBody().size() << " bytes" << std::endl;
        response_html = makeErrorPage(413);
        return;
    }

    // Prépare variables d'environnement
    std::vector<std::string> env;
    std::ostringstream oss;
    oss << req.getBody().size();
    std::string contentLengthStr = oss.str();

    env.push_back("REQUEST_METHOD=" + req.getMethod());
    env.push_back("QUERY_STRING=" + req.getQueryParamsAsString());
    env.push_back("CONTENT_LENGTH=" + contentLengthStr);
    
    //Accès secure au Content-Type
    std::map<std::string, std::string>::const_iterator ct_it = req.getHeaders().find("Content-Type");
    if (ct_it != req.getHeaders().end()) {
        env.push_back("CONTENT_TYPE=" + ct_it->second);
    } else {
        env.push_back("CONTENT_TYPE=");
    }
    
    env.push_back("SCRIPT_FILENAME=" + scriptPath);
    env.push_back("GATEWAY_INTERFACE=CGI/1.1");
    env.push_back("SERVER_PROTOCOL=HTTP/1.1");
    env.push_back("REDIRECT_STATUS=200");
    env.push_back("SERVER_NAME=" + config.server_name);
    
    std::ostringstream port_oss;
    port_oss << config.listen;
    env.push_back("SERVER_PORT=" + port_oss.str());

    // Conversion en char** pour variable env
    char **envp = new(std::nothrow) char*[env.size() + 1];
    if (!envp) {
        std::cerr << "[CGI] Erreur allocation environnement" << std::endl;
        response_html = makeErrorPage(500);
        return;
    }

    //Protection strdup
    bool allocation_failed = false;
    for (size_t i = 0; i < env.size(); ++i) {
        envp[i] = strdup(env[i].c_str());
        if (!envp[i]) {
            allocation_failed = true;
            // Nettoyer les alloc
            for (size_t j = 0; j < i; ++j) {
                free(envp[j]);
            }
            break;
        }
    }
    
    if (allocation_failed) {
        delete[] envp;
        std::cerr << "[CGI] Erreur allocation variables environnement" << std::endl;
        response_html = makeErrorPage(500);
        return;
    }
    
    envp[env.size()] = NULL;

    // Création pipe
    int in_pipe[2];
    int out_pipe[2];
    if (pipe(in_pipe) == -1 || pipe(out_pipe) == -1) {
        for (size_t i = 0; i < env.size(); ++i) {
            free(envp[i]);
        }
        delete[] envp;
        std::cerr << "[CGI] Erreur création pipes" << std::endl;
        response_html = makeErrorPage(500);
        return;
    }
    
    pid_t pid = fork();
    if (pid == -1) {
        close(in_pipe[0]);
        close(in_pipe[1]);
        close(out_pipe[0]);
        close(out_pipe[1]);
        for (size_t i = 0; i < env.size(); ++i) {
            free(envp[i]);
        }
        delete[] envp;
        std::cerr << "[CGI] Erreur fork" << std::endl;
        response_html = makeErrorPage(500);
        return;
    }

    if (pid == 0) { 
        // PROCESSUS ENFANT
        dup2(in_pipe[0], STDIN_FILENO);
        dup2(out_pipe[1], STDOUT_FILENO);

        // Ferme les file descriptors
        close(in_pipe[0]);
        close(in_pipe[1]);
        close(out_pipe[0]);
        close(out_pipe[1]);

        char *argv[] = { (char*)scriptPath.c_str(), NULL };
        execve(scriptPath.c_str(), argv, envp);
        perror("[CGI] execve failed");
        exit(127);
    }
    else { 
        // PROCESSUS PARENT
        close(in_pipe[0]);
        close(out_pipe[1]);
        
        if (!req.getBody().empty()) {
            const char* body_data = req.getBody().c_str();
            size_t body_size = req.getBody().size();
            size_t written = 0;
            
            while (written < body_size) {
                ssize_t result = write(in_pipe[1], body_data + written, body_size - written);
                if (result <= 0) {
                    std::cerr << "[CGI] Erreur écriture body" << std::endl;
                    break;
                }
                written += result;
            }
        }
        close(in_pipe[1]);

        char buffer[4096]; 
        std::string cgi_output;
        ssize_t bytesRead;
        size_t total_read = 0;
        const size_t MAX_CGI_OUTPUT = 5 * 1024 * 1024; // 5MB max pour réponse CGI
        
        signal(SIGALRM, SIG_DFL);
        alarm(30);
        
        while ((bytesRead = read(out_pipe[0], buffer, sizeof(buffer))) > 0) {
            total_read += bytesRead;
            
            if (total_read > MAX_CGI_OUTPUT) {
                std::cerr << "[CGI] Output trop volumineux (" << total_read << " bytes), arrêt" << std::endl;
                break;
            }
            
            try {
                cgi_output.append(buffer, bytesRead);
            } catch (const std::bad_alloc& e) {
                std::cerr << "[CGI] Erreur mémoire lors de l'append" << std::endl;
                break;
            }
        }
        
        alarm(0); 
        close(out_pipe[0]);

        int status;
        pid_t wait_result = waitpid(pid, &status, WNOHANG);
        if (wait_result == 0) {
            kill(pid, SIGTERM);
            sleep(1);
            waitpid(pid, &status, WNOHANG);
            std::cerr << "[CGI] Processus tué pour timeout" << std::endl;
        }
        
        for (size_t i = 0; i < env.size(); ++i) {
            free(envp[i]);
        }
        delete[] envp;
        
        if (cgi_output.empty()) {
            std::cerr << "[CGI] Aucune sortie du script CGI" << std::endl;
            response_html = makeErrorPage(500);
            return;
        }

        if (cgi_output.size() > MAX_CGI_OUTPUT) {
            std::cerr << "[CGI] Sortie CGI tronquée à " << MAX_CGI_OUTPUT << " bytes" << std::endl;
            cgi_output = cgi_output.substr(0, MAX_CGI_OUTPUT);
        }

        size_t header_end = cgi_output.find("\r\n\r\n");
        size_t skip = 4;
        if (header_end == std::string::npos) {
            header_end = cgi_output.find("\n\n");
            skip = 2;
        }
        if (header_end == std::string::npos) {
            std::cerr << "[CGI] Format de sortie invalide (pas de séparateur headers)" << std::endl;
            response_html = makeErrorPage(500);
            return;
        }

        std::string headers = cgi_output.substr(0, header_end);
        std::string body;
        
        if (header_end + skip < cgi_output.size()) {
            body = cgi_output.substr(header_end + skip);
        }

        std::string contentType = "text/html; charset=UTF-8";
        std::string statusMessage = "200 OK";
        bool hasContentType = false;

        std::istringstream headerStream(headers);
        std::string line;
        while (std::getline(headerStream, line)) {
            if (line.find("Content-Type:") == 0) {
                std::string ct = Utils::trim(line.substr(13));
                if (!ct.empty()) {
                    contentType = ct;
                    hasContentType = true;
                }
            }
            else if (line.find("Status:") == 0) {
                std::string st = Utils::trim(line.substr(7));
                if (!st.empty()) {
                    statusMessage = st;
                }
            }
        }

        // Construction de la réponse finale
        std::ostringstream response;
        response << "HTTP/1.1 " << statusMessage << "\r\n";
        response << "Server: webserv/1.0\r\n";
        response << "Content-Type: " << contentType << "\r\n";
        response << "Content-Length: " << body.size() << "\r\n";
        response << "Connection: close\r\n";
        response << "\r\n";
        response << body;

        response_html = response.str();
        
        std::cout << "[CGI] Succès - " << body.size() << " bytes générés" << std::endl;
    }
}