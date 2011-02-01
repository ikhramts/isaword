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

#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>

#include <fstream>
#include <string>
#include <sstream>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>
#include <boost/regex.hpp>

#include <event2/event.h>
#include <event2/http.h>
#include <event2/http_struct.h>
#include <event2/buffer.h>
#include <event2/util.h>
#include <event2/keyvalq_struct.h>

#include "file_handler.h"
#include "http_server.h"
#include "http_utils.h"
#include "file_cache.h"

using boost::shared_ptr;
using boost::shared_array;

namespace isaword {

FileHandler::~FileHandler() {
    delete [] file_read_buffer_;
}

/**
 * Set the directory to serve the files from.
 */
FileHandler::FileRootStatusCode FileHandler::initialize(const std::string& file_root) {
    //Attempt to look up the file root.
    struct stat stat_buffer;
    int status = stat(file_root.c_str(), &stat_buffer);
    
    if (status != 0) {
        return FileHandler::FILE_ROOT_NOT_FOUND;
    }
    
    //Check whether it's a directory.
    if (!S_ISDIR(stat_buffer.st_mode)) {
        return FileHandler::FILE_ROOT_NOT_A_DIRECTORY;
    }
    
    file_root_ = file_root;
    
    //Make sure that the file root ends with '/'.
    if (file_root_[file_root_.size() - 1] != '/') {
        file_root_ += '/';
    }
    
    //Initialize the file buffer if it hasn't yet been.
    if (!file_read_buffer_) {
        file_read_buffer_ = new char[file_read_buffer_size_];
    }
    
    allowed_path_pattern_ = 
        boost::regex("^[a-zA-Z0-9_-]+(\\.[a-zA-Z0-9_-]*)*(/[a-zA-Z0-9_-]+(\\.[a-zA-Z0-9_-]*)*)*$");
    
    // Initialize the file cache.
    file_cache_ = shared_ptr<FileCache>(new FileCache(file_root));
    file_cache_->set_expiration_period(cache_period_sec_);
    std::stringstream cache_control;
    cache_control << "public, max-age=" << cache_period_sec_;
    cache_control_ = cache_control.str();
    
    return FileHandler::FILE_ROOT_OK;
}

/**
 * Add the file handler to the HTTP server, setting it to handle files
 * from a certain url root (e.g. www.someplace.com/url_root/).
 * Should do proper error checking; for now, it doesn't.
 */
FileHandler::ServerAttachStatusCode 
FileHandler::attach_to_server(boost::shared_ptr<HttpServer> server, 
                              const std::string& url_root) {
    //Do a bit of error checking.
    if (is_attached_) {
        return FileHandler::ALREADY_ATTACHED;
    
    } else if (file_root_ == "") {
        return FileHandler::NO_FILE_ROOT_SET;
    }
    
    
    url_root_ = url_root;
    
    //Make sure that the URL root ends with "/".
    if (url_root_[url_root_.size() - 1] != '/') {
        url_root_ += '/';
    }
    
    //Attach the handler to the server.
    std::string url_pattern = url_root_ + ".*";
    server->add_url_handler(url_pattern, &FileHandler::handler_callback, (void*) this);
    
    is_attached_ = true;
    server_ = server;
    return FileHandler::ATTACHED_OK;
}

/**
 * Handle a file request.
 */
void FileHandler::handle_request(struct evhttp_request* request) {
    //Figure out which file to get.
    std::string uri(request_uri_path(request));
    std::string relative_file_path(uri.substr(url_root_.size()));
    
    //Check whether the user entered a valid file path.
    if (!this->is_permitted_file_path(relative_file_path)) {
        // Invalid request.  Cannot find the file.
        server_->send_response(request, std::string(""), HTTP_NOTFOUND);
        return;
    }
    
    //Attempt to load the file.
    //std::string full_path = file_root_ + relative_file_path;
    CachedFilePtr cached_file;
    const bool has_loaded = file_cache_->get_cached_object(relative_file_path, cached_file);
    
    if (!has_loaded) {
        // No such file.
        server_->send_response(request, std::string(""), HTTP_NOTFOUND);
        return;
    }
    
    // Start writing the response.
    // Add Last-Modified response header.
    const time_t last_modified = cached_file->last_modified();
    struct evkeyvalq* response_headers = evhttp_request_get_output_headers(request);
    shared_array<char> sz_last_modified(time_to_string(last_modified));
    evhttp_add_header(response_headers, "Last-Modified", sz_last_modified.get());
    
    // Add Cache Control response header.
    if (!cache_control_.empty()) {
        evhttp_add_header(response_headers, "Cache-Control", cache_control_.c_str());
    }
    
    // Set the Content-Type header.
    std::string extension;
    size_t extension_start = relative_file_path.find_last_of(".");
    const char* content_type = NULL;
    
    if (extension_start != std::string::npos) {
        extension = relative_file_path.substr(extension_start + 1);
    }
    
    if      (extension == "css")    { content_type = "text/css";} 
    else if (extension == "js")     { content_type = "text/javascript"; }
    else if (extension == "png")    { content_type = "image/png"; }
    else if (extension == "jpeg")   { content_type = "image/jpeg"; }
    else if (extension == "jpg")    { content_type = "image/jpeg"; }
    else if (extension == "gif")    { content_type = "image/gif"; }
    else if (extension == "ico")    { content_type = "image/vnd.microsoft.icon"; }
    else if (extension == "txt")    { content_type = "text/plain"; }
    else if (extension == "h")      { content_type = "text/plain"; }
    else if (extension == "cpp")    { content_type = "text/plain"; }
    else if (extension == "hpp")    { content_type = "text/plain"; }
    else if (extension == "html")   { content_type = "text/html"; }
    else                            { content_type = "text/html"; }
    
    evhttp_add_header(response_headers, "Content-Type", content_type);
    
    // Check whether the user agent already has the right version of the file.
    // TODO: need to implement handling "If-None-Match" header.
    struct evkeyvalq* request_headers = evhttp_request_get_input_headers(request);
    const char* if_modified_since = evhttp_find_header(request_headers, "If-Modified-Since");
    const time_t t_if_modified_since = string_to_time(if_modified_since);
    
    if (t_if_modified_since >= cached_file->last_modified()) {
        server_->send_response(request, std::string(""), HTTP_NOTMODIFIED);
        return;
    }
    
    // Respond with the file data..
    shared_array<char> file_data = cached_file->data();
    size_t data_size = cached_file->data_size();
    server_->send_response_data(request, file_data.get(), data_size, HTTP_OK);
}

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
bool FileHandler::read_file(const std::string& relative_file_path, 
                            char* buffer,
                            const std::streamsize buffer_size, 
                            size_t& bytes_read) {
    //Check whether the path is permitted (i.e. does not include backtracking
    //through the directory tree or absolute paths).
    if (!this->is_permitted_file_path(relative_file_path)) {
        return false;
    }
    
    //Get the full file path.
    std::string file_path = file_root_ + relative_file_path;
    
    //Attempt to open the file.
    std::ifstream file(file_path.c_str());
    if (!file.good()) {
        file.close();
        bytes_read = 0;
        return false;
    }
    
    bytes_read = static_cast<size_t>(file.readsome(buffer, buffer_size));
    file.close();
    return true;
}
    
/**
 * Check whether the file path is a permitted path (e.g. not
 * an absolute path, not a path leading up in the directory tree, 
 * etc).
 */
bool FileHandler::is_permitted_file_path(const std::string& file_path) const {
    return regex_match(file_path, allowed_path_pattern_);
}

}