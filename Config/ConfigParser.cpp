#include "../Config/ConfigParser.hpp"
#include "../Request/utils.hpp"
#include <iostream>

/// Parsing function ///////////////////////////////////////////////////////////////////////////////

ConfigParser::ConfigParser(const std::string &path) 
{
	std::ifstream file(path.c_str());
	if (!file.is_open())
		throw std::runtime_error("Impossible d'ouvrir le fichier");
	std::string line;
	while (std::getline(file, line))
	{
		line = Utils::trim(line);
		if (line.empty() || line[0] == '#') //ignore les com ou lignes vides
			continue;
		if (line == "server {")
			parseServerBlock(file, line);
		else
			std::cerr << "Erreur : ligne inconnue : " << line << std::endl;  
	}
}

ConfigParser::~ConfigParser() {}

void ConfigParser::parseServerBlock(std::ifstream &file, std::string &line) 
{
	ServerConfig server;
	while (std::getline(file, line))
	{
		line = Utils::trim(line);
		if (line.empty() || line[0] == '#')
			continue;
		if (line == "}")
			break;
		if (Utils::startWithWord(line, "location"))
			parseLocationBlock(file, line, server);
		else
			parseDirective(line, server);
	}
	servers.push_back(server);
}

void ConfigParser::parseDirective(const std::string &line, ServerConfig &server) 
{
	std::vector<std::string> tokens = Utils::split(line);
	if (tokens.empty())
		return;
	std::string key = tokens[0];

	if (key == "listen" && tokens.size() >= 2)
		server.listen = parseSize(tokens[1]);
	else if (key == "server_name" && tokens.size() >= 2)
		server.server_name = tokens[1];
	else if (key == "client_max_body_size" && tokens.size() >= 2) {
		if (tokens.size() != 2)
    	    throw std::runtime_error("Erreur : mauvais format pour client_max_body_size : " + line);
		server.client_max_body_size = parseSize(tokens[1]);
	}
	else if (key == "error_page") {
		if (tokens.size() != 3)
			throw std::runtime_error("Erreur: format invalid pour error_page : error_page <code> <path>");
		int code = ::atoi(tokens[1].c_str());
		std::string path = tokens[2];

		if (!path.empty() && path[path.size() - 1] == ';')
			path.erase(path.size() - 1);
		if (code < 400 || code > 599)
			throw std::runtime_error("Erreur: code HTTP invalid -> error_page : " + tokens[1]);
		server.error_pages[code] = path;
	}
	else 
		throw std::runtime_error("Directive inconnue dans server block : " + line );
}

void ConfigParser::parseLocationBlock(std::ifstream &file, std::string &line, ServerConfig &server) 
{
	LocationConfig location;

	std::vector<std::string> tokens = Utils::split(line);
	if (tokens.size() < 2) {
		std::cerr << "Erreur: bloc location line : " << line << std::endl;
		return;
	}
	location.path = tokens[1]; //ce qui commence par "/ ..."

	while (std::getline(file, line)) 
	{
		line = Utils::trim(line);
		if (line.empty() || line[0] == '#')
			continue;
		if (line == "}")
			break;
		parseLocationDirective(line, location);
	}
	server.locations.push_back(location);
}

void ConfigParser::parseLocationDirective(const std::string &line, LocationConfig &location) 
{
	std::vector<std::string> tokens = Utils::split(line);
	if (tokens.empty())
		return;
	
	std::string key = tokens[0];

	if (key == "root" && tokens.size() >= 2)
		location.root = tokens[1];

	else if (key == "index" && tokens.size() >= 2)
		location.index = tokens[1];

	else if (key == "allow_methods" && tokens.size() >= 2)
	{
		for (size_t i = 1; i < tokens.size(); ++i)
			location.allow_methods.push_back(tokens[i]);
	}
	else if (key == "directory_listing" && tokens.size() >= 2){
		if (tokens[1] == "on")
 		   location.directory_listing = true;
		else if (tokens[1] == "off")
		   location.directory_listing = false;
		else
    		std::cerr << "Valeur invalide pour directory_listing : " << tokens[1] << std::endl;
	}

	else if (key == "redirect" && tokens.size() >= 2)
		location.redirect = tokens[1];

	else if (key == "cgi_pass" && tokens.size() >= 2)
		location.cgi_pass = tokens[1];

	else if (key == "cgi_extensions" && tokens.size() >= 2)
	{
		for (size_t i = 1; i < tokens.size(); ++i)
			location.cgi_extensions.push_back(tokens[i]);
	}
	else if (key == "upload_store" && tokens.size() >= 2)
		location.upload_store = tokens[1];
	else
		std::cerr << "Erreur: Directive inconnue : " << line << std::endl;
}

const std::vector<ServerConfig>& ConfigParser::getServers() const {
	return this->servers;
}

/// Utils function /////////////////////////////////////////////////////////////////////////

size_t ConfigParser::parseSize(const std::string &sizeStr) {
    if (sizeStr.empty())
        throw std::runtime_error("Erreur : client_max_body_size vide.");

    size_t num = 0;
    size_t i = 0;

    // Vérifie que les premiers caractères sont bien des chiffres
    while (i < sizeStr.length() && std::isdigit(sizeStr[i])) 
	{
        num = num * 10 + (size_t)(sizeStr[i] - '0');
        ++i;
    }

    // Si aucun chiffre n'a été trouvé
    if (i == 0)
        throw std::runtime_error("Erreur : aucun chiffre trouvé dans client_max_body_size : " + sizeStr);

    // Si une unité est présente
    if (i < sizeStr.length()) 
	{
        char unit = std::toupper(sizeStr[i]);

        // Vérifie qu'il n'y a pas de caractères supplémentaires
        if (i + 1 != sizeStr.length())
            throw std::runtime_error("Erreur : unité invalide ou trop longue dans client_max_body_size : " + sizeStr);

        switch (unit) 
		{
            case 'K': return num * 1024;
            case 'M': return num * 1024 * 1024;
            case 'G': return num * 1024 * 1024 * 1024;
            default:
                throw std::runtime_error("Erreur : unité inconnue pour client_max_body_size : " + std::string(1, unit));
        }
    }

    // Vérification de la taille minimale (optionnel mais recommandé)
    if (num < 512)
        throw std::runtime_error("Erreur : client_max_body_size trop petit (< 512 octets)");

    return num;
}

///// Validate function pour ServerConfig///////////////////////////////////////////////////////////////////////////////////////////

void ConfigParser::validateServers() const 
{
	std::set<int> usedPorts;

	for (size_t i = 0; i < servers.size(); ++i)
	{
		const ServerConfig &server = servers[i];
		
		if (server.listen < 1 || server.listen > 65535)
			throw std::runtime_error("port invalide");
		if (usedPorts.count(server.listen))
			throw std::runtime_error("port dupliqué");
		usedPorts.insert(server.listen);

		bool hasRootLocation = false;
		for (size_t j = 0; j < server.locations.size(); ++j) 
		{
			if (server.locations[j].path == "/") {
				hasRootLocation = true;
				break;
			}
		}
		if (!hasRootLocation)
			throw std::runtime_error("Erreur: le serveur n'a pas de location /");
		
		for (std::map<int, std::string>::const_iterator it = server.error_pages.begin(); it != server.error_pages.end(); ++it) 
		{
			if (access(it->second.c_str(), F_OK) == -1)
				throw std::runtime_error("Erreur: fichier error_page introuvable : " + it->second);
		}

		for (size_t j = 0; j < server.locations.size(); ++j) 
		{
			const LocationConfig &loc = server.locations[j];

			for (size_t k = 0; k < loc.allow_methods.size(); ++k) 
			{
				const std::string &method = loc.allow_methods[k];
				if (method != "GET" && method != "POST" && method != "DELETE")
					throw std::runtime_error("Erreur: methode HTTP invalide : " + method + " dans location " + loc.path);
			}

			bool hasPost = false;
			for (size_t k = 0; k < loc.allow_methods.size(); ++k) 
				if (loc.allow_methods[k] == "POST")
					hasPost = true;
			if (hasPost && loc.upload_store.empty())
				throw std::runtime_error("Erreur: POST sans upload_store defini dans : " + loc.path);
			if (!loc.cgi_pass.empty() && access(loc.cgi_pass.c_str(), X_OK) == -1)
				throw std::runtime_error("Erreur: cgi_pass invalide ou non executable : " + loc.cgi_pass);
		}
	}
	
}