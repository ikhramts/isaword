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

#ifndef ISAWORD_HTTP_SERVER_H
#define ISAWORD_HTTP_SERVER_H
 
// A generic HTTP server based on libevent http server.
#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/regex.hpp>
#include <event2/event.h>
#include <event2/http.h>
#include <event2/http_struct.h>
#include <event2/buffer.h>
#include <event2/util.h>
#include <event2/keyvalq_struct.h>

namespace isaword {

class UriHandler;

/*---------------------------------------------------------
                    HttpServer class.
----------------------------------------------------------*/
/// The main server class.
class HttpServer {
public:
    /// Maximum number of simultaneous requests.
    static const size_t kMaxRequests = 500;
    
    /// Possible overload handling strategies.
    enum OverloadHandlingStrategy {
        DO_NOTHING,
        RETURN_503,
        DROP_REQUEST
    };
    
    /// Create an HTTP server.
    HttpServer()
    : event_base_(NULL), 
      server_(NULL), 
      not_found_handler_() {
    }
    
    ///Initialize the server.
    void initialize();
    
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
    bool add_url_handler(const std::string& pattern,
                         void(*callback)(struct evhttp_request*, void*),
                         void* data = NULL);
    
    /** 
     * Set handler for requests that do not match any URL pattern.
     * @param callback a function that will handle the requests
     * @param data additional data to pass to the callback.
     */
    void set_not_found_handler(void(*callback)(struct evhttp_request*, void*),
                         void* data = NULL);
    
    /** 
     * Start serving on a specified IP address and port.
     * @param address a string with IP address to listen on
     * @param port the port number to listen on
     * @return true if succeeded, false if not.
     */
    bool serve(const std::string& address, u_short port);
    
    /**
     * Send a string response.  This is appropriate for text-type responses.
     * For responses containing binary data use send_response/4 function.
     */
    void send_response(struct evhttp_request* request, 
                       const std::string& response,
                       int response_code = HTTP_OK);
    
    /**
     * Send a response containing binary data.
     */
    void send_response_data(struct evhttp_request* request, 
                            const char* response,
                            size_t response_size,
                            int response_code = HTTP_OK);
    
    /// Callback for the evhttp event handler to be provided to the 
    /// evhttp object.  Not for external use.
    static void event_handler(struct evhttp_request* request, void* server) {
        ((HttpServer*)server)->handle_request(request);
    }
    
    /*============= Getters/setters ================*/
    event_base* ev_base() const         {return event_base_;}
    evhttp* ev_server() const           {return server_;}
    
    std::vector<boost::shared_ptr<UriHandler> > uri_handlers() { return uri_handlers_;}
    
    boost::shared_ptr<UriHandler> not_found_handler() {return not_found_handler_;}
    
private:
    /**
     * Create a response string from a response code.
     */
    const char* response_string(int response_code) const;

    /// Handle the request event.
    void handle_request(struct evhttp_request* request);
    
    /// Libevent event base.
    struct event_base* event_base_;
    
    /// Libevent HTTP server instance.
    struct evhttp* server_;
    
    /// Routing patterns.
    std::vector<boost::shared_ptr<UriHandler> > uri_handlers_;
    
    /// Handler for requests that have not matched any pattern.
    boost::shared_ptr<UriHandler> not_found_handler_;
};

/*---------------------------------------------------------
                    UriHandler class.
----------------------------------------------------------*/
/**
 * This class keeps URI patterns and corresponding handlers.
 */
class UriHandler {
public:
    UriHandler() 
    : pattern_(), request_handler_(NULL), handler_data_(NULL) {
    }
    
    ///Initialize the URI handler.  Throws an exception if the
    ///URI is not a valid regex expression.
    void initialize(const std::string& uri_pattern,
               void(*request_handler)(struct evhttp_request*, void*),
               void* handler_data = NULL);
    
    /// Handle the request if the URI matches the pattern.
    /// Returns true if the pattern matches and the callback was called;
    /// false otherwise.
    bool handle_if_matched(const std::string& uri, struct evhttp_request* request);
    
    /// Handle the request.
    void handle(struct evhttp_request* request);
    
    /*============ Getters/setters ==================*/
    /// Get the regex pattern which this handler is responsible for.
    boost::regex pattern() const        {return pattern_;}
    
    /// Get the pointer to the function that will handle requests to this
    /// URI.
    void* handler() const       {return (void*) request_handler_;}
    
    /// Set the request handler.
    void set_handler (void(*request_handler)(struct evhttp_request*, void*)) {
        request_handler_ = request_handler;
    }
    
    /// Get the data to be passed to the handler.
    void* handler_data() const  {return handler_data_;}
    
    /// Set data argument for the request handler.
    void set_handler_data(void* data)   {handler_data_ = data;}
    
private:
    /// The pattern to use when matching;
    boost::regex pattern_;
    
    /// The callback to call if the pattern is matched.
    void(*request_handler_)(struct evhttp_request*, void*);
    
    /// An additional argument to provide with the callback.
    void* handler_data_;
};

} /* namespace isaword */

#endif

