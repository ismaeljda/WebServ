#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <stdint.h>

/*──────────────────── Data structures ────────────────────*/

struct Location
{
    std::string                 path;          // "/img/"
    std::string                 root;          // resolved
    std::vector<std::string>    index_files;   // {"index.html"}
    bool                        autoindex;     // false by default
    std::vector<std::string>    allow_methods; // {"GET","POST"}
    bool                        has_redirect;
    int                         redirect_code;
    std::string                 redirect_url;
    std::string                 cgi_pass;      // empty = none
    std::string                 upload_store;  // empty = none

    Location() : autoindex(false), has_redirect(false),
                 redirect_code(0) {}
};

struct Server
{
    std::string                      host;          // "0.0.0.0"
    uint16_t                         port;          // 8080
    std::vector<std::string>         server_names;
    std::map<int,std::string>        error_pages;   // 404 -> "/err/404.html"
    size_t                           client_max_body; // bytes
    std::vector<Location>            locations;

    Server() : port(0), client_max_body(1 << 20) {}

    const Location& matchLocation(const std::string& uri) const; // longest-prefix
};

/*──────────────────── Singleton API ────────────────────*/

class Config
{
public:
    explicit        Config(const std::string& path);        // throws on error

    static void     reload(const std::string& path);        // atomically replace
    static const Config& instance();                        // read-only view

    const Server&   matchServer(const std::string& host,
                                uint16_t port) const;

private:                        // non-copyable
    Config();
    Config(const Config&);
    Config& operator=(const Config&);

    std::vector<Server>   _servers;
    friend class Parser;
};

/*──────────────────── Exceptions ────────────────────*/
struct ConfigError : public std::runtime_error
{ ConfigError(const std::string& m) : std::runtime_error(m) {} };

#endif /* CONFIG_HPP */
