/*

#include "http_utils.h"

/// Extract the request URI without the query arguments.
std::string uri_path(const char* uri){
    const size_t uri_length = strlen(uri);
    char* sz_path = new char[uri_length + 1];
    size_t i;
    
    for (i = 0; i < uri_length; ++i) {
        if (uri[i] != '?' && uri[i] != '\0') {
            sz_path[i] = uri[i];
        
        } else {
            break;
        }
    }
    
    sz_path[i] = '\0';
    
    std::string path(sz_path);
    delete[] sz_path;

    return path;
}

/// Extract the request URI without the query arguments from
/// an evhttp_request struct.
std::string request_uri_path(struct evhttp_request* request) {
    //return uri_path(evhttp_request_get_uri(request));
}
