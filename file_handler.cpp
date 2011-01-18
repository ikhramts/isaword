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
#include <vector>

#include <boost/shared_ptr.hpp>
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
    
    //Read the file.
    size_t bytes_read;
    bool has_succeeded = 
        this->read_file(relative_file_path, 
                        file_read_buffer_, 
                        file_read_buffer_size_, 
                        bytes_read);
    
    //Start writing the response.
    //Add Cache Control to the response headers.
    struct evkeyvalq* response_headers = evhttp_request_get_output_headers(request);
    
    if (!cache_control_.empty()) {
        evhttp_add_header(response_headers, "Cache-Control", cache_control_.c_str());
    }
    
    //Set the proper content type.
    std::string extension;
    size_t extension_start = relative_file_path.find_last_of(".");
    const char* content_type = NULL;
    
    if (extension_start != std::string::npos) {
        extension = relative_file_path.substr(extension_start + 1);
    }
    
    if      (extension == "css")    { content_type = "text/css";} 
    else if (extension == "js")     { content_type = "text/javascript"; }
    else if (extension == "jpeg")   { content_type = "image/jpeg"; }
    else if (extension == "jpg")    { content_type = "image/jpeg"; }
    else if (extension == "png")    { content_type = "image/png"; }
    else if (extension == "gif")    { content_type = "image/gif"; }
    else if (extension == "ico")    { content_type = "image/vnd.microsoft.icon"; }
    else if (extension == "txt")    { content_type = "text/plain"; }
    else if (extension == "h")      { content_type = "text/plain"; }
    else if (extension == "cpp")    { content_type = "text/plain"; }
    else if (extension == "hpp")    { content_type = "text/plain"; }
    else if (extension == "html")   { content_type = "text/html"; }
    else                            { content_type = "text/html"; }
    
    evhttp_add_header(response_headers, "Content-Type", content_type);
    
    //Send the response.
    const int response_code = has_succeeded ? HTTP_OK : HTTP_NOTFOUND;
    server_->send_response(request, file_read_buffer_, bytes_read, response_code);
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
    
    //Check the file size, and whether it exists.
    //TODO: deal with cases when the files are large.
//    struct stat stat_buffer;
//    int status = stat(file_path.c_str(), &stat_buffer);
//   
//    if (status != 0 || S_ISDIR(stat_buffer.st_mode)) {
//        return false;
//    }
//    
//    size_t file_size = stat_buffer.st_size;
    
    //Attempt to open the file.
    std::ifstream file(file_path.c_str());
    if (!file.good()) {
        file.close();
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