/* * Copyright 2011 Iouri Khramtsov. * * This software is available under Apache License, Version  * 2.0 (the "License"); you may not use this file except in  * compliance with the License. You may obtain a copy of the * License at * *   http://www.apache.org/licenses/LICENSE-2.0 * * Unless required by applicable law or agreed to in writing, * software distributed under the License is distributed on an * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY * KIND, either express or implied. See the License for the * specific language governing permissions and limitations * under the License. */
 #include <boost/shared_array.hpp>#include <iostream>#include <time.h>
#include "http_utils.h"
using boost::shared_array;namespace isaword {
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
}    /// Convert time_t to HTTP Date/Time format./// @return time stored in a C string according to RFC 822,/// e.g. "Mon, 24 Jan 2011 21:18:48 GMT" boost::shared_array<char> time_to_string(const time_t t) {    boost::shared_array<char> time_string(new char[40]);    struct tm tm_time;    gmtime_r(&t, &tm_time);    strftime(time_string.get(), 40,"%a, %d %b %Y %H:%M:%S %Z", &tm_time);    return time_string;}/// Parse HTTP Date/Time to time_t./// @return seconds since epoch if time_string defines a valid time;/// 0 otherwise.time_t string_to_time(const char* time_string) {    if (time_string == NULL) {        return 0;    }    struct tm tm_time;    char* has_parsed = NULL;    // Attempt to parse the string according to RFC 822.    has_parsed = strptime(time_string, "%a, %d %b %Y %H:%M:%S %Z", &tm_time);    if (has_parsed) {        return mktime(&tm_time);    }        has_parsed = strptime(time_string, "%d %b %Y %H:%M:%S %Z", &tm_time);    if (has_parsed) {        return mktime(&tm_time);    }        // Attempt to parse the string according to RFC 850, supposed to be obsolete.    has_parsed = strptime(time_string, "%a, %d-%b-%y %H:%M:%S %Z", &tm_time);    if (has_parsed) {        return mktime(&tm_time);    }        // Attempt to parse the string according ANSI C's asctime() format.    has_parsed = strptime(time_string, "%a %b %d %H:%M:%S %Y", &tm_time);    if (has_parsed) {        return mktime(&tm_time);    }        return 0;}
/// Set response to never be cached.void response_set_never_cache(struct evhttp_request* request) {    struct evkeyvalq* response_headers = evhttp_request_get_output_headers(request);    evhttp_add_header(response_headers, "Cache-Control", "public, max-age=0");}/// Set the cache time for the response (in sec).void response_cache_public(struct evhttp_request* request, size_t sec) {    const char * cache_control_template = "public, max-age=%u";    shared_array<char> cache_control_string(new char[30]);    sprintf(cache_control_string.get(), cache_control_template, sec);    struct evkeyvalq* response_headers = evhttp_request_get_output_headers(request);    evhttp_add_header(response_headers, "Cache-Control", cache_control_string.get());}} /* namespace isaword */
