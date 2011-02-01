/*
 * Copyright 2011 Iouri Khramtsov.
 *
 * This software is available under Apache License, Version 
 * 2.0 (the "License"); you may not use this file except in 
 * compliance with the License. You may obtain a copy of the
 * License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <boost/shared_ptr.hpp>
#include <string>
#include <sstream>
#include <vector>
#include <iostream>

#include <event2/event.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/util.h>
#include <event2/keyvalq_struct.h>
#include "http_server.h"
#include "http_utils.h"

namespace isaword {

using boost::shared_ptr;

/*---------------------------------------------------------
                    HttpServer class.
----------------------------------------------------------*/
void HttpServer::initialize() {
    event_base_ = event_base_new();
    server_ = evhttp_new(event_base_);
    evhttp_set_gencb(server_, &HttpServer::event_handler, (void*)this);
    not_found_handler_ = boost::shared_ptr<UriHandler>(new UriHandler());
}

/**
 * Add a url handler.  Whenever a server receives a request, it will
 * be compared against each URI regular expression in order of their
 * addition, and will run the handler corresponding to the first 
 * matching URI pattern.
 * 
 * @param pattern a regular expression pattern for the URL to match.
 * @param callback a function that will handle the requests
 * @param data additional data to pass to the callback.
 */
bool HttpServer::add_url_handler(const std::string& pattern,
                     void(*callback)(struct evhttp_request*, void*),
                     void* data) {
    shared_ptr<UriHandler> handler(new UriHandler());
    handler->initialize(pattern, callback, data);
    uri_handlers_.push_back(handler);
    return false;
}

/** 
 * Set handler for requests that do not match any URL pattern.
 * @param callback a function that will handle the requests
 * @param data additional data to pass to the callback.
 */
void HttpServer::set_not_found_handler(void(*callback)(struct evhttp_request*, void*),
                                       void* data) {
    not_found_handler_ = shared_ptr<UriHandler>(new UriHandler());
    not_found_handler_->set_handler(callback);
    not_found_handler_->set_handler_data(data);
}

/** 
 * Start serving on a specified IP address and port.
 * @param address a string with IP address to listen on
 * @param port the port number to listen on
 * @return true if succeeded, false if not.
 */
bool HttpServer::serve(const std::string& address, u_short port) {
    if (!evhttp_bind_socket(server_, address.c_str(), port)) {
        event_base_dispatch(event_base_);
        return true;
    }
    
    return false;
}

/// Handle the request event.
void HttpServer::handle_request(struct evhttp_request* request) {
    //Find a callback associated with the URI request.
    std::string uri(request_uri_path(request));
    bool has_handled_request = false;
    
    // Check the request URI and invoke the correct handler.
    for (size_t i = 0; i < uri_handlers_.size(); ++i) {
        if (uri_handlers_[i]->handle_if_matched(uri, request)) {
            has_handled_request = true;
            break;
        }
    }
    
    // No handler is registered for the current URI; return 404.
    if (!has_handled_request) {
        //Check whether the user has provided a 404 handler.
        if (not_found_handler_->handler() != NULL) {
            not_found_handler_->handle(request);
            has_handled_request = true;
        
        } else {
            //Handle the request on our own.
            std::stringstream message;
            message << "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" "
                    << "\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">";
            message << "<html><head><title>Not Found</title></head><body>";        
            message << "<h1>Error 404: Not Found</h1>";
            message << "<p>We could not find the resource you requested at <em>"
                    << uri << "</em>.</p>";
            message << "<p> You are seeing this message because the 404 error "
                    << "handler was not set.  You can set it using "
                    << "HttpServer::set_not_found_handler().</p>";
            message << "<p><em>Since IE and Chrome don't display custom 404 pages "
                    << "undper 512 bytes in length, we have to add this text to "
                    << "make sure that this page is displayed.</em></p>";
            message << "</body></html>";
            
            this->send_response(request, message.str(), HTTP_NOTFOUND);
        }
    }
}

/**
 * Send a string response.  This is appropriate for text-type responses.
 * For responses containing binary data use send_response/4 function.
 */
void HttpServer::send_response(struct evhttp_request* request, 
                               const std::string& response,
                               int response_code) {
    this->send_response_data(request, response.c_str(), response.length(), response_code);
}

/**
 * Send a response containing binary data.
 */
void HttpServer::send_response_data(struct evhttp_request* request, 
                                    const char* response,
                                    size_t response_size,
                                    int response_code) {
    //Prepare the response body.
    struct evbuffer* buffer = evbuffer_new();
    evbuffer_add(buffer, response, response_size);
    
    //Prepare the status string.
    const char* status_string = this->response_string(response_code);
    evhttp_send_reply(request, response_code, status_string, buffer);
    evbuffer_free(buffer);
}

/**
 * Create a response string from a response code.
 */
const char* HttpServer::response_string(int response_code) const {
    const char* status_string = NULL;
    
    switch (response_code) {
        case HTTP_BADREQUEST:   status_string = "Bad Request";          break;
        case HTTP_MOVEPERM:     status_string = "Moved Permanently";    break;
        case HTTP_MOVETEMP:     status_string = "Moved Temporarily";    break;
        case HTTP_NOCONTENT:    status_string = "No Content";           break;
        case HTTP_NOTFOUND:     status_string = "Not Found";            break;
        case HTTP_NOTMODIFIED:  status_string = "Not Modified";         break;
        case HTTP_OK:           status_string = "OK";                   break;
        case HTTP_SERVUNAVAIL:  status_string = "Service Unavailable";  break;
        //default: 
            //TODO: crash and/or report an error.
            //return;
    }
    
    return status_string;
}

/*---------------------------------------------------------
                    UriHandler class.
----------------------------------------------------------*/
///Initialize the URI handler.  Throws an exception if the
///URI is not a valid regex expression.
void UriHandler::initialize(const std::string& uri_pattern,
           void(*request_handler)(struct evhttp_request*, void*),
           void* handler_data) {
    //TODO: Implement
    pattern_ = boost::regex(uri_pattern);
    request_handler_ = request_handler;
    handler_data_ = handler_data;
}

/// Handle the request if the URI matches the pattern.
/// Returns true if the pattern matches and the callback was called;
/// false otherwise.
bool UriHandler::handle_if_matched(const std::string& uri, struct evhttp_request* request) {
    if (regex_match(uri, pattern_)) {
        (*request_handler_)(request, handler_data_);
        return true;
    }
    
    return false;
}

/// Handle the request.
void UriHandler::handle(struct evhttp_request* request) {
    (*request_handler_)(request, handler_data_);
}
} /* namespace isaword */

