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
#include <boost/shared_array.hpp>
#include <boost/regex.hpp>

#include <event2/event.h>
#include <event2/http.h>
#include <event2/http_struct.h>
#include <event2/buffer.h>
#include <event2/util.h>
#include <event2/keyvalq_struct.h>

#include "views.h"
#include "http_server.h"
#include "http_utils.h"
#include "file_cache.h"

using boost::shared_ptr;
using boost::shared_array;

namespace isaword {

/**
 * Initialize.
 * Returns true if there were no problems; false if the initialization
 * failed.
 */
bool PageHandler::initialize() {
    template_cache_ = boost::shared_ptr<FileCache>(new FileCache());
    page_buffer_ = shared_array<char>(new char[page_buffer_size_]);
    
    // Create the descriptions of lists of words to keep track of.
    
    
    //Create the word picker.
    
    //Attach to the server.
    server_->add_url_handler("/", &main_page, (void*) this);
    return true;
}

/*================ Page request handlers =====================*/
/**
 * Load the main page.
 */
void PageHandler::main_page(struct evhttp_request* request, void* page_handler_ptr) {
    PageHandler* this_ = (PageHandler*) page_handler_ptr;
    shared_array<char> page_template;
    size_t chars = 0;
    const bool got_template = 
        this_->template_cache_->get(
                this_->template_path("templates/main-layout.html"), 
                page_template,
                &chars);

    this_->reserve_page_buffer(chars + 50);
    const int page_size = sprintf(this_->page_buffer_.get(), page_template.get(), "Hello!");
    const size_t u_page_size = static_cast<size_t>(page_size);
    
    //Return the page.
    this_->server_->send_response(request, this_->page_buffer_.get(), u_page_size, HTTP_OK);
}

/**
 * Load the About page.
 */
void PageHandler::about(struct evhttp_request* request, void* page_handler_ptr) {
    //TODO: implement.
}

/**
 * Load the Fine Print.
 */
void PageHandler::fine_print(struct evhttp_request* request, void* page_handler_ptr) {
    //TODO: implement.
}

/**
 * Load the words to guess.
 */
void PageHandler::words(struct evhttp_request* request, void* page_handler_ptr) {
    //TODO: implement.
}

/**
 * 404 Not Found page.
 */
void PageHandler::not_found(struct evhttp_request* request, void* page_handler_ptr) {
}

/*================ Useful functions =====================*/

/**
 * Get the JSON object associated with the words to guess.
 * Returns an string containing a JSON object, some of which
 * would be real (in that case they'd have a definition as well),
 * and some of which would be fake.
 */
std::string PageHandler::make_words_to_guess(const std::string& template_path) {
    //TODO: implement.
    std::string empty_string;
    return empty_string;
}

/// Ensure that the page buffer can fit the page to be generated.
void PageHandler::reserve_page_buffer(size_t bytes) {
    if (page_buffer_size_ < 2 * bytes) {
        page_buffer_size_ = 2 * bytes;
    }
    
    page_buffer_ = shared_array<char>(new char[page_buffer_size_]);
}

} /* namespace isaword */

