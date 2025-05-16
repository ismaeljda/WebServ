#include "RequestParser.hpp"
#include <fstream>

#include <fstream>

void createSampleHTTPRequest() {
    std::ofstream file("parse.txt");
    file << "GET /index.html?category=cpp&tag=webserver&page=2 HTTP/1.1\r\n";
    file << "Host: localhost:8080\r\n";
    file << "User-Agent: curl/7.64.1\r\n";
    file << "Accept: */*\r\n";
    file << "\r\n";
    file << "hello";
    file.close();
}

std::string readFile(const std::string& path) 
{
    std::ifstream file(path);
    if (!file) 
    {
        throw std::runtime_error("Impossible d'ouvrir le fichier : " + path);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}
int main() 
{
    createSampleHTTPRequest();
    RequestParser rqs;
    std::string file_content = readFile("parse.txt");
    rqs.parse(file_content);
    std::map<std::string, std::string> queryParams = rqs.getQueryParams();
    for (std::map<std::string, std::string>::iterator it = queryParams.begin(); it != queryParams.end(); ++it)
    {
        std::cout << "Key: " << it->first << ", Value: " << it->second << std::endl;
    }
}