#include "ConfigParser.hpp"
#include <iostream>

ConfigParser::ConfigParser(const std::string &path) 
{
	std::ifstream file(path.c_str());
	if (!file.is_open())
		throw std::runtime_error("Impossible d'ouvrir le fichier");
}