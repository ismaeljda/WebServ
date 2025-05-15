#include "lexer.hpp"
#include <cctype>

/*────────── ctor ──────────*/
Lexer::Lexer(std::istream& in)
: _in(in), _hasLook(false), _line(1), _col(0) {}

/*────────── helpers ──────────*/
int Lexer::_getc()
{
    int ch = _in.get();
    if (ch == '\n') { ++_line; _col = 0; }
    else ++_col;
    return ch;
}
void Lexer::_ungetc(int ch)
{
    _in.unget();
    if (ch == '\n') { --_line; }
    else --_col;
}

/*────────── public API ──────────*/
Token Lexer::peek() const
{
    if (!_hasLook)
    {
        const_cast<Lexer*>(this)->_look = const_cast<Lexer*>(this)->next();
        const_cast<Lexer*>(this)->_hasLook = true;
    }
    return _look;
}
Token Lexer::consume()
{
    if (_hasLook) { _hasLook = false; return _look; }
    return next();
}

/*────────── core scanner ──────────*/
Token Lexer::next()
{
    for (;;)
    {
        int ch = _getc();                         /* skip WS & comments */
        if (ch == '#') { while (ch != '\n' && ch != EOF) ch = _getc(); continue; }
        if (isspace(ch)) continue;

        size_t l = _line, c = _col;
        if (ch == EOF)  return Token(TOK_EOF, "", l, c);
        if (ch == '{')  return Token(TOK_LBRACE, "{", l, c);
        if (ch == '}')  return Token(TOK_RBRACE, "}", l, c);
        if (ch == ';')  return Token(TOK_SEMI,   ";", l, c);

        _ungetc(ch);
        if (ch == '/')          return _scanPath(l, c);
        if (isdigit(ch))        return _scanNumber(l, c);
        if (isalpha(ch))        return _scanIdent(l, c);
                                return _scanString(l, c);
    }
}

/*────────── tiny scanners ──────────*/
Token Lexer::_scanIdent(size_t l, size_t c)
{
    std::string s; int ch;
    while ((ch = _getc()) != EOF && (isalnum(ch) || ch=='_'||ch=='.'))
        s.push_back(char(ch));
    _ungetc(ch);
    return Token(TOK_IDENT, s, l, c);
}
Token Lexer::_scanNumber(size_t l, size_t c)
{
    std::string s; int ch;
    while ((ch = _getc()) != EOF && isdigit(ch))
        s.push_back(char(ch));
    _ungetc(ch);
    return Token(TOK_NUMBER, s, l, c);
}
Token Lexer::_scanPath(size_t l, size_t c)
{
    std::string s; int ch;
    while ((ch = _getc()) != EOF && !isspace(ch) && ch!=';' && ch!='{'&&ch!='}')
        s.push_back(char(ch));
    _ungetc(ch);
    return Token(TOK_PATH, s, l, c);
}
Token Lexer::_scanString(size_t l, size_t c)
{
    std::string s; int ch;
    while ((ch = _getc()) != EOF && !isspace(ch) && ch!=';' && ch!='{'&&ch!='}')
        s.push_back(char(ch));
    _ungetc(ch);
    return Token(TOK_STRING, s, l, c);
}
