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

// This file describes an object responsible for handling requests
// asking to get a file from disk.

#ifndef ISAWORD_FILE_HANDLER_H
#define ISAWORD_FILE_HANDLER_H

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

/// Predeclared request struct.
struct evhttp_request;

namespace isaword {

class HttpServer;

class FileHandler {
public:
    enum FileRootStatusCode {
        FILE_ROOT_OK = 0,
        FILE_ROOT_NOT_FOUND = 1,
        FILE_ROOT_NOT_A_DIRECTORY = 2,
    };
    
    enum ServerAttachStatusCode {
        ATTACHED_OK = 0,
        ALREADY_ATTACHED = 1,
        BAD_URL = 2,
        NO_FILE_ROOT_SET = 3,
    };
    
    /**
     * Create a file request handler that will get files from a 
     * specified root directory.
     */
    FileHandler()
    : is_attached_(false),
    file_read_buffer_(NULL) {
    }
    
    ~FileHandler();
    
    /**
     * Set the directory to serve the files from.
     */
    FileRootStatusCode initialize(const std::string& file_root);
    
    /**
     * Add the file handler to the HTTP server, setting it to handle files
     * from a certain url root (e.g. www.someplace.com/url_root/).
     * Should do proper error checking; for now, it doesn't.
     */
    ServerAttachStatusCode attach_to_server(boost::shared_ptr<HttpServer> server, 
                         const std::string& url_root);
    
    /**
     * Callback function for HttpServer.
     */
    static void handler_callback(struct evhttp_request* request, void* file_handler) {
        ((FileHandler*) file_handler)->handle_request(request);
    }
    
    /**
     * Handle a file request.
     */
    void handle_request(struct evhttp_request* request);
    
    /**
     * Read a file.
     * @param relative_file_path the path of the file to read, 
     * relative to the file_root().
     * @param buffer where the file contents will be placed.
     * @param buffer_size available buffer size.
     * @param bytes_read a variable where the total number of 
     * bytes read will be placed.
     * @return true on success, false if the file did not exist or 
     * was not accessible.
     */
    bool read_file(const std::string& relative_file_path, 
                   char* buffer, 
                   const std::streamsize buffer_size, 
                   size_t& bytes_read);
    
    /**
     * Check whether the file path is a permitted path (e.g. not
     * an absolute path, not a path leading up in the directory tree, 
     * etc).
     */
    bool is_permitted_file_path(const std::string& file_path) const;
     
    
    /*=============== Getters/setters ==============*/
    /// Set cache control response header.
    void set_cache_control(const std::string& cache_control) {
        cache_control_ = cache_control;
    }
    
    /// Get the cache control response header.
    std::string cache_control() const       {return cache_control_;}
    
    /// Check whether the handler is attached to a server.
    bool is_attached() const                {return is_attached_;}
    
    /// Get the URL root
    std::string url_root() const            {return url_root_;}
    
    /// Get the directory to serve the files from.
    std::string file_root() const           {return file_root_;}
    
private:
    std::string file_root_;
    bool is_attached_;
    std::string url_root_;
    std::string cache_control_;
    
    /// Number of bytes to read from file in a single read.
    /// This is a pretty arbitrary value, not sure whether this is best.
    static const std::streamsize file_read_buffer_size_ = 524288; // 0.5 MB
    
    /// Buffer for reading files.
    char* file_read_buffer_;
    
    /// The HttpServer to which this is attached.
    boost::shared_ptr<HttpServer> server_;
    
    /// The regex expression for permitted file paths.
    boost::regex allowed_path_pattern_;
    
    /// File buffer
};


} /* namespace isaword */
#endif