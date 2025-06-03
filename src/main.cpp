#include "../Config/ConfigParser.hpp"
#include "../Server_2/Server.hpp"
#include <iostream>


// int main(int argc, char **argv)
// {
// 	if (argc != 2) {
// 		std::cerr << "Usage: ./webserv <config file>" << std::endl;
// 		return 1;
// 	}

// 	try {
// 		ConfigParser parser(argv[1]);
// 		parser.validateServers();
// 		const std::vector<ServerConfig>& servers = parser.getServers();

// 		std::vector<Server*> serverInstances;
// 		for (size_t i = 0; i < servers.size(); ++i)
// 			serverInstances.push_back(new Server(servers[i]));

// 		for (size_t i = 0; i < serverInstances.size(); ++i)
// 			serverInstances[i]->run();

// 		for (size_t i = 0; i < serverInstances.size(); ++i)
// 			delete serverInstances[i];

// 	} catch (const std::exception& e) {
// 		std::cerr << "Erreur: " << e.what() << std::endl;
// 		return 1;
// 	}
// 	return 0;
// }
int main(int argc, char** argv)
{
	if (argc != 2)
	{
		std::cerr << "Usage: ./webserv <config_file>" << std::endl;
		return 1;
	}

	std::vector<Server*> servers;
	std::vector<pollfd> fds;
	std::map<int, Server*> client_to_server;

	// 🔧 Parser le fichier de config et créer chaque Server
	std::vector<ServerConfig> configs = ConfigParser(argv[1]).getServers(); // à adapter à ton parser
	for (size_t i = 0; i < configs.size(); ++i)
	{
		Server* srv = new Server(configs[i]); // ton constructeur Server(config)
		servers.push_back(srv);

		// Ajouter le socket d'écoute dans pollfd
		pollfd pfd;
		pfd.fd = srv->getServerfd();
		pfd.events = POLLIN;
		pfd.revents = 0;
		fds.push_back(pfd);
	}

	std::cout << "✅ Serveurs initialisés. En attente de connexions..." << std::endl;

	// 🔁 Boucle principale
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
				// 1. Est-ce un server_fd ?
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
					// 2. Sinon, c’est un client
					Server* srv = client_to_server[fds[i].fd];
					if (srv)
						srv->handleClient(fds[i].fd);

					// Supprimer le client_fd de pollfd + de la map
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