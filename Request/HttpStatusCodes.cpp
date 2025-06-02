#include "HttpStatusCodes.hpp"

std::map<HttpStatusCodes::Code, std::string> HttpStatusCodes::statusMessages;

std::string HttpStatusCodes::getMessage(Code code) {
    if (statusMessages.empty()) {
        statusMessages[CONTINUE] = "Continue";
        statusMessages[SWITCHING_PROTOCOLS] = "Switching Protocols";
        statusMessages[OK] = "OK";
        statusMessages[CREATED] = "Created";
        statusMessages[ACCEPTED] = "Accepted";
        statusMessages[NO_CONTENT] = "No Content";
        statusMessages[MOVED_PERMANENTLY] = "Moved Permanently";
        statusMessages[FOUND] = "Found";
        statusMessages[SEE_OTHER] = "See Other";
        statusMessages[NOT_MODIFIED] = "Not Modified";
        statusMessages[BAD_REQUEST] = "Bad Request";
        statusMessages[UNAUTHORIZED] = "Unauthorized";
        statusMessages[FORBIDDEN] = "Forbidden";
        statusMessages[NOT_FOUND] = "Not Found";
        statusMessages[METHOD_NOT_ALLOWED] = "Method Not Allowed";
        statusMessages[REQUEST_TIMEOUT] = "Request Timeout";
        statusMessages[PAYLOAD_TOO_LARGE] = "Payload Too Large";
        statusMessages[URI_TOO_LONG] = "URI Too Long";
        statusMessages[UNSUPPORTED_MEDIA_TYPE] = "Unsupported Media Type";
        statusMessages[INTERNAL_SERVER_ERROR] = "Internal Server Error";
        statusMessages[NOT_IMPLEMENTED] = "Not Implemented";
        statusMessages[BAD_GATEWAY] = "Bad Gateway";
        statusMessages[SERVICE_UNAVAILABLE] = "Service Unavailable";
        statusMessages[GATEWAY_TIMEOUT] = "Gateway Timeout";
        statusMessages[HTTP_VERSION_NOT_SUPPORTED] = "HTTP Version Not Supported";
    }

    return statusMessages[code];
}
