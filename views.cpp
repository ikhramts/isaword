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

#include <algorithm>
#include <fstream>
#include <string>
#include <vector>

#include <boost/lexical_cast.hpp>
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
#include "word_picker.h"

using boost::shared_ptr;
using boost::shared_array;
using boost::lexical_cast;
using boost::bad_lexical_cast;

namespace isaword {

/// Default minimum word length.
const size_t PageHandler::kDefaultMinWordLength;

/// Default maximum word length.
const size_t PageHandler::kDefaultMaxWordLength;

/**
 * Initialize.
 * Returns true if there were no problems; false if the initialization
 * failed.
 */
bool PageHandler::initialize(const std::string& resource_root) {
    template_cache_ = boost::shared_ptr<FileCache>(new FileCache(resource_root));
    page_buffer_ = shared_array<char>(new char[page_buffer_size_]);
    
    // Create the descriptions of lists of words to keep track of.
    shared_ptr<WordIndexDescription> j_words(
        new WordIndexDescription("j_words", "J words", "^.*J.*$"));
    shared_ptr<WordIndexDescription> q_words(
        new WordIndexDescription("q_words", "Q words", ".*Q.*"));
    shared_ptr<WordIndexDescription> q_witout_u_words(
        new WordIndexDescription("q_withoutt_u_words", "Q without U words", "^(.*Q[^U].*)|(.*Q)$"));
    shared_ptr<WordIndexDescription> x_words(
        new WordIndexDescription("x_words", "X words", "^.*X.*$"));
    shared_ptr<WordIndexDescription> z_words(
        new WordIndexDescription("z_words", "Z words", "^.*Z.*$"));
    shared_ptr<WordIndexDescription> consonants(
        new WordIndexDescription("consonants", "Consonants Only", "^[^AEIOU]*$"));
    shared_ptr<WordIndexDescription> all_vowels_but_one(
        new WordIndexDescription("all_vowels_but_one", "All vowels but one", "^[AEIOU]*[^AEIOU][AEIOU]*$"));
    shared_ptr<WordIndexDescription> out_words(
        new WordIndexDescription("out_words", "OUT- words", "^OUT.*$"));
    shared_ptr<WordIndexDescription> re_words(
        new WordIndexDescription("re_words", "RE- words", "^RE.*$"));
    
    index_descriptions_.push_back(j_words);
    index_descriptions_.push_back(q_words);
    index_descriptions_.push_back(q_witout_u_words);
    index_descriptions_.push_back(x_words);
    index_descriptions_.push_back(z_words);
    index_descriptions_.push_back(consonants);
    index_descriptions_.push_back(all_vowels_but_one);
    index_descriptions_.push_back(out_words);
    index_descriptions_.push_back(re_words);
    
    //Create the word picker.
    word_picker_ = shared_ptr<WordPicker>(new WordPicker(index_descriptions_));
    bool has_initialized_word_picker = 
        word_picker_->initialize(resource_root + "dictionaries/owl2.txt");
    
    //Build vairous templates.
    main_page_template_ =  this->build_main_page_template();
    about_page_ = this->insert_into_main_layout("templates/about.html");
    fine_print_page_ = this->insert_into_main_layout("templates/fine-print.html");
    not_found_template_ = this->insert_into_main_layout("templates/404.html");
    
    //Attach to the server.
    server_->add_url_handler("/", &main_page, (void*) this);
    server_->add_url_handler("/about/?", &about, (void*) this);
    server_->add_url_handler("/fine_print/?", &fine_print, (void*) this);
    server_->add_url_handler("/words/[a-z0-9/_]+", &words, (void*) this);
    server_->set_not_found_handler(&not_found, this);
    return has_initialized_word_picker;
}

/*================ Page request handlers =====================*/
/**
 * Display the main page.
 */
void PageHandler::main_page(struct evhttp_request* request, void* page_handler_ptr) {
    PageHandler* this_ = (PageHandler*) page_handler_ptr;

    //Compose the main page.
    std::string words("var words = ");
    words += this_->make_words_to_guess("/") + ';';
    const int page_size = 
        sprintf(this_->page_buffer_.get(), this_->main_page_template_.c_str(), words.c_str());
    const size_t u_page_size = static_cast<size_t>(page_size);
    
    //Return the page.
    response_set_never_cache(request);
    this_->server_->send_response_data(request, 
                                       this_->page_buffer_.get(), 
                                       u_page_size, 
                                       HTTP_OK);
}

/**
 * Display the About page.
 */
void PageHandler::about(struct evhttp_request* request, void* page_handler_ptr) {
    PageHandler* this_ = (PageHandler*) page_handler_ptr;
    response_set_never_cache(request);
    this_->server_->send_response(request, this_->about_page_, HTTP_OK);
}

/**
 * Display the Fine Print.
 */
void PageHandler::fine_print(struct evhttp_request* request, void* page_handler_ptr) {
    PageHandler* this_ = (PageHandler*) page_handler_ptr;
    response_set_never_cache(request);
    this_->server_->send_response(request, this_->fine_print_page_, HTTP_OK);
}

/**
 * Display the words to guess.
 */
void PageHandler::words(struct evhttp_request* request, void* page_handler_ptr) {
    //Get the words to send in JSON format.
    PageHandler* this_ = (PageHandler*) page_handler_ptr;
    std::string uri = request_uri_path(request);
    std::string words = this_->make_words_to_guess(uri.substr(6));
    
    //Set the proper Content-Type header
    struct evkeyvalq* response_headers = evhttp_request_get_output_headers(request);
    evhttp_add_header(response_headers, "Content-Type", "application/json");
    
    response_set_never_cache(request);
    this_->server_->send_response(request, words, HTTP_OK);
}

/**
 * Display the 404 Not Found page.
 */
void PageHandler::not_found(struct evhttp_request* request, void* page_handler_ptr) {
    PageHandler* this_ = (PageHandler*) page_handler_ptr;
    std::string uri = request_uri_path(request);
    
    //Compose the not found page.
    shared_array<char> escaped_uri(evhttp_htmlescape(uri.c_str()));
    const int page_size = 
        sprintf(this_->page_buffer_.get(), this_->not_found_template_.c_str(), escaped_uri.get());
    const size_t u_page_size = static_cast<size_t>(page_size);
    
    response_cache_public(request, 3600 /* sec */);
    this_->server_->send_response_data(request, 
                                       this_->page_buffer_.get(), 
                                       u_page_size, 
                                       HTTP_OK);
}

/*================ Useful functions =====================*/

/**
 * Get the JSON object associated with the words to guess.
 * Returns an string containing a JSON object, some of which
 * would be real (in that case they'd have a definition as well),
 * and some of which would be fake.
 *
 * @param description_uri the part of uri that describes what kind of 
 * words to make; should be of the format 
 * "/<dictionary>/<num_words>/<index|length>/<index_name|length_from[/length_to]>", e.g.:
 *     "/owl2/10/index/q_words"
 *     "/owl2/20/length/2/4" 
 */
std::string PageHandler::make_words_to_guess(const std::string& description_uri) {
    std::stringstream result;
    result << "[";
    
    //Parse the description uri.
    const size_t max_parts = 5;
    std::vector<std::string> description;
    size_t end = 0;
    size_t part_number = 1;
    
    while ((end + 1) < description_uri.length()) {
        const size_t start = end + 1;
        end = description_uri.find_first_of('/', start);
        
        if (end == std::string::npos) {
            description.push_back(description_uri.substr(start));
            break;
        
        } else {
            description.push_back(description_uri.substr(start, end - start));
        }    
        
        if (part_number >= max_parts) {
            break;
        }
        
        part_number++;
    };
    
    //Ignore the dictionary; it'll always be owl2.
//    if (description[0] != "owl2") {
//        return result;
//    }
    shared_ptr<WordPicker> word_picker(word_picker_);
    
    //Find the number of words to generate.
    const size_t max_num_words = 40;
    const size_t default_num_words = 10;
    size_t num_words = 0;
    
    if (description.size() < 2) {
        num_words = default_num_words;
        
    } else {
        try {
            num_words = lexical_cast<size_t, std::string>(description[1]);
            num_words = std::max(std::min(num_words, max_num_words), 1u);
        
        } catch (bad_lexical_cast&) {
            num_words = default_num_words;
        }
    }
    
    // Proceed differently depending on the what needs to be generated.
    std::vector<WordDescriptionPtr> words;
    
    if (description.size() < 4 || description[2] != "index" ) {
        // Generate the word based on length.
        const size_t default_from = kDefaultMinWordLength;
        const size_t default_to = kDefaultMaxWordLength;
        const size_t min_word_length = 2;
        const size_t max_word_length = 15;
        
        //Get the "from" argument.
        size_t from;
        
        if (description.size() < 4) {
            from = default_from;
            
        } else {
            try {
                from  = lexical_cast<size_t, std::string>(description[3]);
                from = std::max(std::min(from, max_word_length), min_word_length);
                
            } catch (bad_lexical_cast&) {
                from = default_from;
            }
        }
        
        // Get the "to" argument.
        size_t to;
        
        if (description.size() < 5) {
            to = std::max(default_to, from);
            
        } else {
            try {
                to = lexical_cast<size_t, std::string>(description[4]);
                to = std::max(std::min(to, max_word_length), from);

            } catch (bad_lexical_cast&) {
                to = std::max(default_to, from);
            }
        }
        
        words = word_picker->get_words_by_length(from, to, num_words);
        
    } else {
        //Pick words from an index.
        //Find the index to use.  Since we made it this far, we're 
        //guaranteed to have the fourth element in description vector.
        //Use the first index by default.
        std::string index_name = description[3];
        size_t index_num = 0;
        
        for (size_t i = 0; i < index_descriptions_.size(); i++) {
            if (index_descriptions_[i]->name() == index_name) {
                index_num = i;
                break;
            }
        }
        
        words = word_picker->get_words_from_index(index_num, num_words);
    }
    
    //Compose the JSON object.
    for (size_t i = 0; i < words.size(); i++) {
        result << "\n\t{";
        result << "\n\t\t\"word\": \"" << words[i]->word << "\",";
        result << "\n\t\t\"description\": \"" << words[i]->description << "\",";
        result << "\n\t\t\"is_real\": " << (words[i]->is_real ? "true" : "false") << "";
        result << "\n\t}";
        
        if (i != words.size() - 1) {
            result << ",";
        }
    }
        
    result << "\n]";
    return result.str();
}

/// Ensure that the page buffer can fit the page to be generated.
void PageHandler::reserve_page_buffer(size_t bytes) {
    if (page_buffer_size_ < 2 * bytes) {
        page_buffer_size_ = 2 * bytes;
    }
    
    page_buffer_ = shared_array<char>(new char[page_buffer_size_]);
}

/// Build the template for the main page.
std::string PageHandler::build_main_page_template() {
    std::string empty_string;
    
    // Build the word type menu.  Allow for 20 character index names and 50
    // character index descriptions.
    const size_t extra_chars = 20 + 20 + 20 + 50;

    //Find the template for producing word type selectors.
    size_t index_template_chars = 0;
    shared_array<char> type_selection_template;
    
    const bool got_index_template = this->template_cache_->get(
                            this->template_path("templates/index-description.html"), 
                            type_selection_template,
                            &index_template_chars);
    
    
    if (!got_index_template) {
        std::cout << "Could not find template at templates/index-description.html"
                  << std::endl;
        return empty_string;
    }
    
    shared_array<char> word_type_piece(new char[index_template_chars + extra_chars]);
    
    //Put all word type selectors together.
    std::string word_type_selector;
    
    for (size_t i = 0; i < index_descriptions_.size(); ++i) {
        shared_ptr<WordIndexDescription> word_type = index_descriptions_[i];
        sprintf(word_type_piece.get(), 
                type_selection_template.get(),
                word_type->name().c_str(),
                word_type->name().c_str(),
                word_type->name().c_str(),
                word_type->description().c_str());
        
        word_type_selector += word_type_piece.get();
    }
    
    // Insert the wort type selector piece into the content for the
    // main page.
    shared_array<char> main_content_template;
    size_t main_content_chars;
    const bool got_content_template = this->template_cache_->get(
                            this->template_path("templates/main.html"), 
                            main_content_template,
                            &main_content_chars);
    
    if (!got_content_template) {
        std::cout << "Could not find template at templates/main.html"
                  << std::endl;
        return empty_string;
    }
    
    shared_array<char> main_content(new char[main_content_chars + word_type_selector.length() + 1]);
    sprintf(main_content.get(), 
            main_content_template.get(), 
            kDefaultMinWordLength,
            kDefaultMaxWordLength,
            word_type_selector.c_str()); 
    
    // Insert the content of the main page into the general page layout.
    return this->insert_into_main_layout("%s", main_content.get());
}

/// Insert page content into the main page layout.
std::string PageHandler::insert_into_main_layout(const std::string& extra_scripts,
                                                 const std::string& content) {
    size_t layout_chars = 0;
    shared_array<char> layout_template;

    const bool got_layout_template = this->template_cache_->get(
                            this->template_path("templates/main-layout.html"), 
                            layout_template,
                            &layout_chars);
    
    if (!got_layout_template) {
        std::cout << "Could not find template at templates/main-layout.html"
                  << std::endl;
        std::string empty_string;
        return empty_string;
    }
    
    const size_t page_length = layout_chars + extra_scripts.length() + content.length();
    shared_array<char> sz_page(new char[page_length]);
    sprintf(sz_page.get(), 
            layout_template.get(), 
            extra_scripts.c_str(), 
            content.c_str()); 
    
    std::string page(sz_page.get());
    return page;
}

/// Insert page content into the main page layout from a content file.
std::string PageHandler::insert_into_main_layout(const std::string& content_file) {
    shared_array<char> content;
    const bool found = template_cache_->get(this->template_path(content_file), 
                                            content,
                                            NULL /* bytes of data output */);
    
    if (!found) {
        std::cout << "Could not find page content at " << content_file
                  << std::endl;
        std::string empty_string;
        return empty_string;
    }
    
    return this->insert_into_main_layout("", content.get());
}

} /* namespace isaword */

