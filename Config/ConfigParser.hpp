#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include "ConfigStruct.hpp"

class ConfigParser {
	private :
		std::vector<ServerConfig> server;

		void parseServerBlock(std::ifstream &file, std::string &line); 
		void parseLocationBlock(std::ifstream &file, std::string &line, ServerConfig &server);
		void parseDirective(const std::string &line, ServerConfig &server);
		void parseLocationDirective(const std::string &line, LocationConfig &location);

		std::string trim(const std::string &s);
		std::vector<std::string> split(const std::string &s);
		bool startWithWord(const std::string &line, const std::string &prefix);

	public :
		ConfigParser(const std::string &path);
		~ConfigParser();

		const std::vector<ServerConfig>& getServer() const;
};

#endif