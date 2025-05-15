#ifndef PARSER_HPP
#define PARSER_HPP

#include "lexer.hpp"
#include "config.hpp"

class Parser
{
public:
    explicit        Parser(Lexer& lex);

    void            parse(Config& cfg); // fills cfg._servers

private:
    /* server-level */
    void            parseServer(Config& cfg);
    void            parseListen(Server& s);
    void            parseServerName(Server& s);
    void            parseErrorPage(Server& s);
    void            parseClientMaxBody(Server& s);

    /* location-level */
    void            parseLocation(Server& s);
    void            parseLocationDirective(Location& loc,
                                           const Token& head);

    /* helpers */
    Token           consume();
    Token           expect(TokenType t, const char* msg);
    Token           peek() const;

    Lexer&          _lex;
};

#endif /* PARSER_HPP */
