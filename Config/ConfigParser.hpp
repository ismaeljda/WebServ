#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

#include <string>
#include <vector>
#include <set>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cstdlib>
#include <cctype>
#include "ConfigStruct.hpp"
#include "../Request/utils.hpp"

class ConfigParser {
	private :
		std::vector<ServerConfig> servers;

		void parseServerBlock(std::ifstream &file, std::string &line); 
		void parseDirective(const std::string &line, ServerConfig &server);
		void parseLocationBlock(std::ifstream &file, std::string &line, ServerConfig &server);
		void parseLocationDirective(const std::string &line, LocationConfig &location);

		std::string trim(const std::string &s);
		std::vector<std::string> split(const std::string &s);
		bool startWithWord(const std::string &line, const std::string &prefix);
		
		public :
		ConfigParser(const std::string &path);
		~ConfigParser();
		
		size_t parseSize(const std::string &sizeStr);
		const std::vector<ServerConfig>& getServers() const;

		void validateServers() const;
};

#endif