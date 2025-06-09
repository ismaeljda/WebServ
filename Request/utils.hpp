#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <vector>
#include <sstream>

namespace Utils {
    std::string trim(const std::string &s);
    std::vector<std::string> split(const std::string &s);
    bool startWithWord(const std::string &str, const std::string &prefix);
    std::string extractMimeType(std::string& headerValue);
}

#endif