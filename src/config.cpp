#include "config.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include <fstream>
#include <pthread.h>

/*────────────── singleton (thread-safe, C++98) ─────────────*/
static Config*             g_cfg = 0;
static pthread_mutex_t     g_mtx = PTHREAD_MUTEX_INITIALIZER;

/*────────────── ctor ─────────────*/
Config::Config(const std::string& path)
{
    std::ifstream f(path.c_str());
    if (!f) throw ConfigError("cannot open " + path);

    Lexer  lex(f);
    Parser p(lex);
    p.parse(*this);
}

/*────────────── reload / instance ─────────────*/
void Config::reload(const std::string& path)
{
    Config* fresh = new Config(path);      /* may throw – no swap yet */

    pthread_mutex_lock(&g_mtx);
    if (g_cfg) delete g_cfg;
    g_cfg = fresh;
    pthread_mutex_unlock(&g_mtx);
}
const Config& Config::instance()
{
    pthread_mutex_lock(&g_mtx);
    if (!g_cfg) throw ConfigError("Config not loaded");
    Config& ref = *g_cfg;
    pthread_mutex_unlock(&g_mtx);
    return ref;
}

/*────────────── runtime helpers ─────────────*/
const Server& Config::matchServer(const std::string& host,
                                  uint16_t port) const
{
    const Server* fallback = 0;
    for (size_t i=0;i<_servers.size();++i)
    {
        const Server& s=_servers[i];
        if (s.port!=port) continue;

        if (s.server_names.empty()) { if (!fallback) fallback=&s; }
        else
            for (size_t j=0;j<s.server_names.size();++j)
                if (s.server_names[j]==host) return s;
    }
    if (fallback) return *fallback;
    throw ConfigError("no server matches " + host);
}

const Location& Server::matchLocation(const std::string& uri) const
{
    const Location* best=0;
    for (size_t i=0;i<locations.size();++i)
        if (uri.find(locations[i].path)==0)      /* prefix */
            if (!best || locations[i].path.size()>best->path.size())
                best=&locations[i];

    if (best) return *best;
    throw ConfigError("no location matches " + uri);
}
