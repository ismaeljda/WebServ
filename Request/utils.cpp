#include "RequestParser.hpp"
#include "../Server/Server.hpp"

std::string trim(const std::string &str) {
    size_t start = 0;
    while (start < str.length() && (str[start] == ' ' || str[start] == '\t')) 
    {
        start++;
    }
    size_t end = str.length();
    while (end > start && (str[end - 1] == ' ' || str[end - 1] == '\t')) 
    {
        end--;
    }
    if (start >= end) 
    {
        return "";
    }
    return str.substr(start, end - start);
}


std::string extractMimeType(std::string& headerValue)
{
    // headerValue attendu sans le "Accept: " au début, ex: "text/css,*/*;q=0.1"
    std::istringstream ss(headerValue);
    std::string token;

    // On découpe par ',' et on prend le premier token non vide
    while (std::getline(ss, token, ','))
    {
        // enlever espaces devant et derrière
        size_t start = token.find_first_not_of(" \t");
        size_t end = token.find_last_not_of(" \t");
        if (start == std::string::npos || end == std::string::npos)
            continue;
        token = token.substr(start, end - start + 1);

        // ici on peut retourner le premier type mime "text/css", etc.
        return token;
    }
    return ""; // rien trouvé
}