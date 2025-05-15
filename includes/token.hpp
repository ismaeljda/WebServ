#ifndef TOKEN_HPP
#define TOKEN_HPP
#include <string>

enum TokenType
{
    TOK_EOF,
    TOK_LBRACE, TOK_RBRACE, TOK_SEMI,
    TOK_IDENT,  TOK_NUMBER,
    TOK_PATH,   TOK_STRING
};

struct Token
{
    TokenType   type;
    std::string value;
    size_t      line;
    size_t      col;

    Token(TokenType t = TOK_EOF,
          const std::string& v = "",
          size_t l = 0,
          size_t c = 0)
        : type(t), value(v), line(l), col(c) {}
};

#endif /* TOKEN_HPP */
