#include "parser.hpp"
#include <cstdlib>   // strtol
#include <sstream>

/*──────────────── handy util ───────────────*/
static size_t toBytes(const std::string& s)
{
    char* end = 0;
    long  val = std::strtol(s.c_str(), &end, 10);
    if (end == s.c_str() || val < 0) throw ConfigError("bad size: " + s);

    size_t mul = 1;
    if (*end=='k'||*end=='K') mul = 1024;
    else if (*end=='m'||*end=='M') mul = 1024*1024;
    else if (*end=='g'||*end=='G') mul = 1024*1024*1024;
    return size_t(val) * mul;
}

/*──────────────── ctor ─────────────────────*/
Parser::Parser(Lexer& lex) : _lex(lex) {}

/*──────────────── top-level ────────────────*/
void Parser::parse(Config& cfg)
{
    while (peek().type != TOK_EOF)
        parseServer(cfg);
}

/*──────────────── server block ─────────────*/
void Parser::parseServer(Config& cfg)
{
    expect(TOK_IDENT, "expected 'server'");
    expect(TOK_LBRACE, "'{' missing after server");

    Server s;
    while (peek().type != TOK_RBRACE)
    {
        Token h = consume();
        if (h.value=="listen")            parseListen(s);
        else if (h.value=="server_name")  parseServerName(s);
        else if (h.value=="error_page")   parseErrorPage(s);
        else if (h.value=="client_max_body_size")
                                          parseClientMaxBody(s);
        else if (h.value=="location")     parseLocation(s);
        else throw ConfigError("unknown directive " + h.value);
    }
    consume();                  /* eat '}' */
    cfg._servers.push_back(s);
}

/*──── server-level directives ────*/
void Parser::parseListen(Server& s)
{
    Token v = expect(TOK_STRING,"listen value");
    expect(TOK_SEMI,"';' missing");

    std::string hp = v.value;
    size_t col = hp.find(':');
    if (col==std::string::npos) { s.host="0.0.0.0"; s.port=atoi(hp.c_str()); }
    else { s.host=hp.substr(0,col); s.port=atoi(hp.substr(col+1).c_str()); }
}
void Parser::parseServerName(Server& s)
{
    for (Token t=expect(TOK_STRING,"server_name"); t.type!=TOK_SEMI; t=consume())
        s.server_names.push_back(t.value);
}
void Parser::parseErrorPage(Server& s)
{
    Token codeTok = expect(TOK_NUMBER,"error code");
    Token pathTok = expect(TOK_PATH,"error path");
    expect(TOK_SEMI,"';' missing");
    s.error_pages[atoi(codeTok.value.c_str())] = pathTok.value;
}
void Parser::parseClientMaxBody(Server& s)
{
    Token t = expect(TOK_STRING,"size");
    expect(TOK_SEMI,"';' missing");
    s.client_max_body = toBytes(t.value);
}

/*──── location block ────*/
void Parser::parseLocation(Server& s)
{
    Token p = expect(TOK_PATH,"location path");
    expect(TOK_LBRACE,"'{' missing after location");

    Location l; l.path = p.value;

    while (peek().type != TOK_RBRACE)
        parseLocationDirective(l, consume());

    consume();               /* eat '}' */
    s.locations.push_back(l);
}
void Parser::parseLocationDirective(Location& l, const Token& h)
{
    if (h.value=="root")
    {
        l.root = expect(TOK_PATH,"root path").value;
        expect(TOK_SEMI,"';' missing");
    }
    else if (h.value=="index")
    {
        for (Token t = expect(TOK_STRING,"index value"); t.type!=TOK_SEMI; t=consume())
            l.index_files.push_back(t.value);
    }
    else if (h.value=="autoindex")
    {
        Token v = expect(TOK_IDENT,"autoindex on/off");
        expect(TOK_SEMI,"';' missing");
        l.autoindex = (v.value=="on");
    }
    else if (h.value=="allow_methods")
    {
        for (Token t = expect(TOK_IDENT,"verb"); t.type!=TOK_SEMI; t=consume())
            l.allow_methods.push_back(t.value);
    }
    else if (h.value=="return")
    {
        Token code = expect(TOK_NUMBER,"status");
        Token url  = expect(TOK_STRING,"url");
        expect(TOK_SEMI,"';' missing");
        l.has_redirect = true;
        l.redirect_code = atoi(code.value.c_str());
        l.redirect_url  = url.value;
    }
    else if (h.value=="cgi_pass")
    {
        l.cgi_pass = expect(TOK_PATH,"cgi path").value;
        expect(TOK_SEMI,"';' missing");
    }
    else if (h.value=="upload_store")
    {
        l.upload_store = expect(TOK_PATH,"dir").value;
        expect(TOK_SEMI,"';' missing");
    }
    else
        throw ConfigError("unknown directive " + h.value + " in location");
}

/*──── helpers ────*/
Token Parser::consume()         { return _lex.consume(); }
Token Parser::peek() const      { return _lex.peek(); }
Token Parser::expect(TokenType t,const char* m)
{
    Token tok = consume();
    if (tok.type!=t) throw ConfigError(m);
    return tok;
}
