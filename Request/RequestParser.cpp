#include "RequestParser.hpp"
#include <iostream>
#include <sstream>

RequestParser::RequestParser() 
{

}

RequestParser::~RequestParser() 
{

}
bool RequestParser::isvalidMethod(const std::string& method) const
{
    return method == "GET" || method == "POST" || method == "DELETE";
}

bool RequestParser::isvalidVersion(const std::string& version) const
{
    return version == "HTTP/1.1";
}

bool RequestParser::isvalidHeader(const std::map<std::string, std::string> header) const {
    for (std::map<std::string, std::string>::const_iterator it = header.begin(); it != header.end(); ++it) {
        std::string key = it->first;
        for (size_t i = 0; i < key.size(); ++i)
            key[i] = std::tolower(key[i]);
        if (key == "host")
            return true;
    }
    return false;
}

bool RequestParser::parse(std::string& str) 
{
    size_t header_end = str.find("\r\n\r\n");
    if (header_end == std::string::npos)
        return false;

    std::string header_part = str.substr(0, header_end);

    // Lire la première ligne (méthode, URI, version)
    std::istringstream stream(header_part);
    std::string first_line;
    std::getline(stream, first_line);
    
    std::istringstream line_stream(first_line);
    line_stream >> method >> uri >> version;

    parseQueryParams(uri);

    if (!RequestParser::isvalidMethod(method)) {
        std::cerr << "Error: unsupported HTTP method: " << method << std::endl;
        return false;
    }
    if (!RequestParser::isvalidVersion(version)) {
        std::cerr << "Error: not an HTTP version supported: " << version << std::endl;
        return false;
    }

    // Headers
    std::string line;
    while (std::getline(stream, line)) {
        if (line.empty() || line == "\r")
            continue;

        size_t colon = line.find(':');
        if (colon == std::string::npos)
            continue; // ligne mal formée : on ignore

        std::string key = trim(line.substr(0, colon));
        std::string value = trim(line.substr(colon + 1));
        if (!key.empty())
            headers[key] = value;
    }
    if (!RequestParser::isvalidHeader(headers)) 
    {
        std::cerr << "Error: invalid header:" << std::endl;
        return false; // Faudra set un code d'erreur ici
    }
    // Mettre le body dans une string body
    if (header_end + 4 < str.length()) 
    {
        body = str.substr(header_end + 4);
    }
    
    return true;
}

void RequestParser::parseQueryParams(const std::string& uri) 
{
    size_t questionMark = uri.find('?');
    if (questionMark != std::string::npos) 
    {
        std::string params = uri.substr(questionMark + 1);
        std::istringstream paramStream(params);
        std::string param;
        
        while (std::getline(paramStream, param, '&')) 
        {
            size_t equals = param.find('=');
            if (equals != std::string::npos) 
            {
                std::string key = param.substr(0, equals);
                std::string value = param.substr(equals + 1);
                queryParams[key] = value;
            } 
            else 
            {
                queryParams[param] = "";
            }
        }
    }
}

void RequestParser::display_request()
{
    std::cout << "Method used is: " << method << std::endl;
    std::cout << "uri is: " << uri << std::endl;
    std::cout << "Headers are: " << std::endl;
    std::map<std::string, std::string>::iterator it;
    for (it = headers.begin(); it != headers.end(); ++it) 
    {
        std::cout << "Key : " << it->first << ", Value : " << it->second << std::endl;
    }
    std::cout << "Body is: " << body << std::endl;
}

//Getter

const std::string& RequestParser::getMethod() const { return method; }
const std::string& RequestParser::getUri() const { return uri; }
const std::string& RequestParser::getVersion() const { return version; }
const std::map<std::string, std::string>& RequestParser::getHeaders() const { return headers; }
const std::string& RequestParser::getBody() const { return body; }
const std::map<std::string, std::string>& RequestParser::getQueryParams() const {return queryParams;}