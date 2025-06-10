#ifndef REQUESTPARSER_HPP
# define REQUESTPARSER_HPP

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <map>
#include "utils.hpp"
#include "../Server_2/Server.hpp"

class Server;

std::string trim(const std::string &str);

class RequestParser
{
    private:
        std::string method;
        std::string uri;
        std::string version;
        std::string body;
        std::map<std::string, std::string> headers;
        std::map<std::string, std::string> queryParams;
        void parseQueryParams(const std::string& uri);


    public: 
        RequestParser();
        ~RequestParser();
        bool isvalidMethod(const std::string& method) const;
        bool isvalidVersion(const std::string& version) const;
        bool isvalidHeader(const std::map<std::string, std::string> header) const;
        bool parse(std::string& str, Server *server, std::string &response_html);
        void display_request();
        
        std::string getQueryParamsAsString() const;


        //getter
        const std::string& getMethod() const;
        const std::string& getUri() const;
        const std::string& getVersion() const;
        const std::map<std::string, std::string>& getHeaders() const;
        const std::string& getBody() const;
        const std::map<std::string, std::string>& getQueryParams() const;
};

#endif