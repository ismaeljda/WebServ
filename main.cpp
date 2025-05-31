#include "ConfigParser.hpp"
#include "ConfigStruct.hpp"
#include <iostream>

int main(int argc, char **argv)
{
    std::string configPath = "config.conf";
    if (argc == 2)
        configPath = argv[1];

    try {
        ConfigParser parser(configPath);
        const std::vector<ServerConfig>& servers = parser.getServers();

        for (size_t i = 0; i < servers.size(); ++i) {
            const ServerConfig& s = servers[i];
            std::cout << "========== SERVER " << i + 1 << " ==========" << std::endl;
            std::cout << "Port: " << s.listen << std::endl;
            std::cout << "Server name: " << s.server_name << std::endl;

            std::cout << "Max body size (octets): " << s.client_max_body_size << std::endl;
            if (s.client_max_body_size < 1024)
                std::cout << "  ➤ ~ " << s.client_max_body_size << " B" << std::endl;
            else if (s.client_max_body_size < 1024 * 1024)
                std::cout << "  ➤ ~ " << s.client_max_body_size / 1024 << " K" << std::endl;
            else
                std::cout << "  ➤ ~ " << s.client_max_body_size / (1024 * 1024) << " M" << std::endl;

            for (size_t j = 0; j < s.locations.size(); ++j) {
                const LocationConfig& loc = s.locations[j];
                std::cout << "  -- Location: " << loc.path << std::endl;
                std::cout << "     root: " << loc.root << std::endl;
                std::cout << "     index: " << loc.index << std::endl;
                std::cout << "     redirect: " << loc.redirect << std::endl;
                std::cout << "     cgi_pass: " << loc.cgi_pass << std::endl;
                std::cout << "     upload_store: " << loc.upload_store << std::endl;
                std::cout << "     directory_listing: " << (loc.directory_listing ? "on" : "off") << std::endl;

                std::cout << "     allow_methods: ";
                for (size_t k = 0; k < loc.allow_methods.size(); ++k)
                    std::cout << loc.allow_methods[k] << " ";
                std::cout << std::endl;

                std::cout << "     cgi_extensions: ";
                for (size_t k = 0; k < loc.cgi_extensions.size(); ++k)
                    std::cout << loc.cgi_extensions[k] << " ";
                std::cout << std::endl;
            }

            std::cout << std::endl;
        }
    } catch (const std::exception &e) {
        std::cerr << "Erreur de parsing : " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
