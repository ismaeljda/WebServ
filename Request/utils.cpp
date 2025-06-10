#include "utils.hpp"
#include <cctype>

namespace Utils {

std::string trim(const std::string &s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end = s.find_last_not_of(" \t\r\n");
    
    if (start == std::string::npos)
        return "";
    return s.substr(start, end - start + 1);
}

std::vector<std::string> split(const std::string &s) {
    std::vector<std::string> tokens;
    std::istringstream iss(s);
    std::string word;
    
    while (iss >> word) {
        // Supprimer le point-virgule en fin si présent
        if (!word.empty() && word[word.size() - 1] == ';')
            word.erase(word.size() - 1);
        tokens.push_back(word);
    }
    return tokens;
}

bool startWithWord(const std::string &str, const std::string &prefix) {
    return str.compare(0, prefix.length(), prefix) == 0;
}

std::string extractMimeType(std::string& headerValue) {
    // headerValue attendu sans le "Accept: " au début, ex: "text/css,*/*;q=0.1"
    std::istringstream ss(headerValue);
    std::string token;
    
    // On découpe par ',' et on prend le premier token non vide
    while (std::getline(ss, token, ',')) {
        // enlever espaces devant et derrière
        size_t start = token.find_first_not_of(" \t");
        size_t end = token.find_last_not_of(" \t");
        if (start == std::string::npos || end == std::string::npos)
            continue;
        token = token.substr(start, end - start + 1);
        
        // Supprimer les paramètres de qualité (;q=0.x)
        size_t semicolon = token.find(';');
        if (semicolon != std::string::npos) {
            token = token.substr(0, semicolon);
        }
        
        // Retourner le premier type MIME non vide et valide
        if (!token.empty() && token != "*/*") {
            return token;
        }
    }
    return "";
}

}