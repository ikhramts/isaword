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

// This file defines handlers for various web pages.
// Much of the work done here would be better accomplished
// by a templating engine.  However, I could not find a 
// good C++ templating engine, and there isn't that much
// work to do, so may as well do it the simple way.

#ifndef ISAWORD_VIEWS_H
#define ISAWORD_VIEWS_H

#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>
#include <event2/event.h>
#include <event2/http.h>

struct evhttp_request;

namespace isaword {

class HttpServer;
class FileCache;
class WordPicker;
class WordIndexDescription;

class PageHandler {
public:
    /// Default minimum word length.
    static const size_t kDefaultMinWordLength = 2;
    
    /// Default maximum word length.
    static const size_t kDefaultMaxWordLength = 8;

    /**
     * Construct an instance of PageHandler.
     */
    PageHandler(const boost::shared_ptr<HttpServer>& server)
    : server_(server),
      template_root_(""),
      page_buffer_size_(kDefaultPageBufferSize){
    }
    
    /**
     * Initialize.
     * Returns true if there were no problems; false if the initialization
     * failed.
     */
    bool initialize();
    
    /*================ Page request handlers =====================*/
    /**
     * Load the main page.
     */
    static void main_page(struct evhttp_request* request, void* page_handler_ptr);
    
    /**
     * Load the About page.
     */
    static void about(struct evhttp_request* request, void* page_handler_ptr);
    
    /**
     * Load the Fine Print.
     */
    static void fine_print(struct evhttp_request* request, void* page_handler_ptr);
    
    /**
     * Load the words to guess.
     */
    static void words(struct evhttp_request* request, void* page_handler_ptr);
    
    /**
     * 404 Not Found page.
     */
    static void not_found(struct evhttp_request* request, void* page_handler_ptr);
    
    /*================ Useful functions =====================*/
    /**
     * Get the JSON object associated with the words to guess.
     * Returns an string containing a JSON object, some of which
     * would be real (in that case they'd have a definition as well),
     * and some of which would be fake.
     */
    std::string make_words_to_guess(const std::string& template_path);
    
    /*================ Getters/setters =====================*/
    /// Get the HttpServer instance associated with this
    /// object.
    boost::shared_ptr<HttpServer> server() const    {return server_;}
    
private:
    /// Default size of the page buffer.
    static const size_t kDefaultPageBufferSize = 51200; //50 kB
    
    /// Ensure that the page buffer can fit the page to be generated.
    void reserve_page_buffer(size_t bytes);
    
    /// Build the template for the main page.
    std::string build_main_page_template();
    
    /// Convert a template path to the path relative to the executable.
    std::string template_path(const std::string& path) {
        return template_root_ + path;
    }

    /// The main server responsible for the requests and responses.
    boost::shared_ptr<HttpServer> server_;
    
    /// All template paths will be resolved relative to this path.
    std::string template_root_;
    
    /// Template cache.
    boost::shared_ptr<FileCache> template_cache_;

    /// A place to temporarily generate web pages.
    boost::shared_array<char> page_buffer_;
    
    /// A piece of memory to use while generating web pages.
    boost::shared_array<char> template_buffer_;
    
    /// Size of the page buffer.
    size_t page_buffer_size_;
    
    /// An object to pick lists of words to guess.
    boost::shared_ptr<WordPicker> word_picker_;
    
    /// A list of indexes for words.
    std::vector<boost::shared_ptr<WordIndexDescription> > index_descriptions_;
    
    /// Cached main page template.
    std::string main_page_template_;
    
    /// Cached About page.
    std::string about_page_;
};

} /* namespace isaword */

#endif