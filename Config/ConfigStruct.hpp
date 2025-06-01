#ifndef CONFIGSTRUCT_HPP
#define CONFIGSTRUCT_HPP

#include <vector>
#include <map>
#include <string>

struct LocationConfig {
	std::string path;
	std::string root;
	std::string index;
	std::vector<std::string> allow_methods;
	bool directory_listing;
	std::string redirect;
	std::string cgi_pass;
	std::vector<std::string> cgi_extensions;
	std::string upload_store;

	LocationConfig() : directory_listing(false) {}
};

struct ServerConfig {
	int listen;
	std::string server_name;
	size_t client_max_body_size;
	std::vector<LocationConfig> locations;
	std::map<int, std::string> error_pages;

	ServerConfig() : listen(80), client_max_body_size(0) {}
};

#endif