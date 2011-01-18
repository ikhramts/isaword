/* * Copyright 2011 Iouri Khramtsov. * * This software is available under Apache License, Version  * 2.0 (the "License"); you may not use this file except in  * compliance with the License. You may obtain a copy of the * License at * *   http://www.apache.org/licenses/LICENSE-2.0 * * Unless required by applicable law or agreed to in writing, * software distributed under the License is distributed on an * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY * KIND, either express or implied. See the License for the * specific language governing permissions and limitations * under the License. */

#include "http_utils.h"
namespace isaword {
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
    //return uri_path(evhttp_request_get_uri(request));    const struct evhttp_uri* uri = evhttp_request_get_evhttp_uri(request);    const char* sz_path = evhttp_uri_get_path(uri);    std::string path(sz_path);    //evhttp_uri_free(uri);    return path;
}
} /* namespace isaword */
