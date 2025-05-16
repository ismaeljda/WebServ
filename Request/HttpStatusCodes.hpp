#ifndef HTTP_STATUS_CODES_HPP
#define HTTP_STATUS_CODES_HPP

#include <string>
#include <map>

// Classe pour centraliser les codes d'erreur HTTP
class HttpStatusCodes {
public:
    enum Code {
        // 1xx Informational
        CONTINUE = 100,
        SWITCHING_PROTOCOLS = 101,
        
        // 2xx Success
        OK = 200,
        CREATED = 201,
        ACCEPTED = 202,
        NO_CONTENT = 204,
        
        // 3xx Redirection
        MOVED_PERMANENTLY = 301,
        FOUND = 302,
        SEE_OTHER = 303,
        NOT_MODIFIED = 304,
        
        // 4xx Client Error
        BAD_REQUEST = 400,
        UNAUTHORIZED = 401,
        FORBIDDEN = 403,
        NOT_FOUND = 404,
        METHOD_NOT_ALLOWED = 405,
        REQUEST_TIMEOUT = 408,
        PAYLOAD_TOO_LARGE = 413,
        URI_TOO_LONG = 414,
        UNSUPPORTED_MEDIA_TYPE = 415,
        
        // 5xx Server Error
        INTERNAL_SERVER_ERROR = 500,
        NOT_IMPLEMENTED = 501,
        BAD_GATEWAY = 502,
        SERVICE_UNAVAILABLE = 503,
        GATEWAY_TIMEOUT = 504,
        HTTP_VERSION_NOT_SUPPORTED = 505
    };
    
    // Obtenir le message correspondant à un code
    static std::string getMessage(Code code);
    
    // Map des codes et messages pour l'initialisation
    static std::map<Code, std::string> statusMessages;
};

#endif