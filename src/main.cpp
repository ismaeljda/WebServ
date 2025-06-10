#include "../Config/ConfigParser.hpp"
#include "../Server_2/Server.hpp"
#include "string.h"
#include <iostream>

int main(int argc, char** argv)
{
	if (argc != 2 || strcmp(argv[1], "config.conf") != 0) 
	{
		std::cerr << "Wrong input: must be ./WebServ <config_file>" << std::endl;
		exit(1);
	}

	std::vector<Server*> servers;
	std::vector<pollfd> fds;
	std::map<int, Server*> client_to_server;

	//  Parser le fichier de config et créer chaque Server
	std::vector<ServerConfig> configs = ConfigParser(argv[1]).getServers();
	for (size_t i = 0; i < configs.size(); ++i)
	{
		Server* srv = new Server(configs[i]);
		servers.push_back(srv);
		pollfd pfd;
		pfd.fd = srv->getServerfd();
		pfd.events = POLLIN;
		pfd.revents = 0;
		fds.push_back(pfd);
	}

	std::cout << "✅ Serveurs initialisés. En attente de connexions..." << std::endl;
	while (true)
	{
		int ret = poll(&fds[0], fds.size(), -1);
		if (ret < 0)
		{
			std::cerr << "Erreur : poll() a échoué" << std::endl;
			break;
		}

		for (size_t i = 0; i < fds.size(); ++i)
		{
			if (fds[i].revents & POLLIN)
			{
				// Check si c est un fd de server
				bool is_server_fd = false;
				for (size_t j = 0; j < servers.size(); ++j)
				{
					if (fds[i].fd == servers[j]->getServerfd())
					{
						servers[j]->acceptClient(fds, client_to_server);
						is_server_fd = true;
						break;
					}
				}

				if (!is_server_fd)
				{
					// si c est pas server c est client
					Server* srv = client_to_server[fds[i].fd];
					if (srv)
						srv->handleClient(fds[i].fd);

					// Clean le client fd dans le vector
					close(fds[i].fd);
					client_to_server.erase(fds[i].fd);
					fds.erase(fds.begin() + i);
					--i;
				}
			}
		}
	}

	// 🧹 Cleanup final
	for (size_t i = 0; i < servers.size(); ++i)
		delete servers[i];

	return 0;
}