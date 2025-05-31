#include "ConfigParser.hpp"
#include <iostream>

ConfigParser::ConfigParser(const std::string &path) 
{
	std::ifstream file(path.c_str());
	if (!file.is_open())
		throw std::runtime_error("Impossible d'ouvrir le fichier");
	std::string line;
	while (std::getline(file, line))
	{
		line = trim(line);
		if (line.empty() || line[0] == '#') //ignore les com ou lignes vides
			continue;
		if (line == "server {")
			parseServerBlock(file, line);
		else
			std::cerr << "Erreur : ligne inconnue : " << line << std::endl;  
	}
}

ConfigParser::~ConfigParser() {}

void ConfigParser::parseServerBlock(std::ifstream &file, std::string &line) {
	ServerConfig server;
	while (std::getline(file, line))
	{
		line = trim(line);
		if (line.empty() || line[0] == '#')
			continue;
		if (line == "}")
			break;
		if (startWithWord(line, "location"))
			parseLocationBlock(file, line, server);
		else
			parseDirective(line, server);
	}
	servers.push_back(server);
}

void ConfigParser::parseDirective(const std::string &line, ServerConfig &server) {
	std::vector<std::string> tokens = split(line);
	if (tokens.empty())
		return;
	std::string key = tokens[0];

	if (key == "listen" && tokens.size() >= 2)
		server.listen = std::atoi(tokens[1].c_str());
	else if (key == "server_name" && tokens.size() >= 2)
		server.server_name = tokens[1];
	else if (key == "client_max_body_size" && tokens.size() >= 2)
		server.client_max_body_size = std::atoi(tokens[1].c_str());
	else 
		std::cerr << "Directive inconnue dans server block : " << line << std::endl;
}

void ConfigParser::parseLocationBlock(std::ifstream &file, std::string &line, ServerConfig &server) {
	LocationConfig location;

	std::vector<std::string> tokens = split(line);
	if (tokens.size() < 2) {
		std::cerr << "Erreur: bloc location line : " << line << std::endl;
		return;
	}
	location.path = tokens[1]; //ce qui commence par "/ ..."

	while (std::getline(file, line)) {
		line = trim(line);
		if (line.empty() || line[0] == '#')
			continue;
		if (line == "}")
			break;
		parseLocationDirective(line, location);
	}
	server.locations.push_back(location);
}

void ConfigParser::parseLocationDirective(const std::string &line, LocationConfig &location) {
	std::vector<std::string> tokens = split(line);
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


std::string ConfigParser::trim(const std::string &s) {
	size_t start = s.find_first_not_of(" \t\r\n");
	size_t end = s.find_last_not_of(" \t\r\n");

	if (start == std::string::npos)
		return "";
	return s.substr(start, end - start + 1);
}

std::vector<std::string> ConfigParser::split(const std::string &s) {
	std::vector<std::string> tokens;
	std::istringstream iss(s);
	std::string word;

	while (iss >> word){
		tokens.push_back(word);
		for (size_t i = 0; i < tokens.size(); ++i)
			if (!tokens[i].empty() && tokens[i][tokens[i].size() - 1] == ';')
				tokens[i].erase(tokens[i].size() - 1);
	}
	return tokens;
}

bool ConfigParser::startWithWord(const std::string &line, const std::string &prefix) {
	return line.compare(0, prefix.length(), prefix) == 0;
}
