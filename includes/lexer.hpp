#ifndef LEXER_HPP
#define LEXER_HPP

#include <istream>
#include "token.hpp"

class Lexer
{
public:
    explicit    Lexer(std::istream& in);

    Token       peek() const;    // look-ahead (one token)
    Token       consume();       // take current token
    Token       next();          // low-level, public just for tests

private:
    int         _getc();
    void        _ungetc(int ch);

    Token       _scanIdent(size_t l, size_t c);
    Token       _scanNumber(size_t l, size_t c);
    Token       _scanPath(size_t l, size_t c);
    Token       _scanString(size_t l, size_t c);

    std::istream&   _in;
    mutable Token   _look;
    mutable bool    _hasLook;
    size_t          _line;
    size_t          _col;
};

#endif /* LEXER_HPP */
