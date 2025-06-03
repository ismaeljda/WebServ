#include "../Config/ConfigParser.hpp"
#include "../Server_2/Server.hpp"
#include <iostream>


int main(int argc, char **argv)
{
	if (argc != 2) {
		std::cerr << "Usage: ./webserv <config file>" << std::endl;
		return 1;
	}

	try {
		ConfigParser parser(argv[1]);
		parser.validateServers();
		const std::vector<ServerConfig>& servers = parser.getServers();

		std::vector<Server*> serverInstances;
		for (size_t i = 0; i < servers.size(); ++i)
			serverInstances.push_back(new Server(servers[i]));

		for (size_t i = 0; i < serverInstances.size(); ++i)
			serverInstances[i]->run();

		for (size_t i = 0; i < serverInstances.size(); ++i)
			delete serverInstances[i];

	} catch (const std::exception& e) {
		std::cerr << "Erreur: " << e.what() << std::endl;
		return 1;
	}
	return 0;
}
