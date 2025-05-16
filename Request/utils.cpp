#include "RequestParser.hpp"

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