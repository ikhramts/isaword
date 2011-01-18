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

// Test module for PseudowordGenerator.
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE isaword_server

#include <sstream>
#include <vector>
#include <string>
#include <boost/test/unit_test.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/regex.hpp>
#include <boost/regex/pattern_except.hpp>
#include "http_utils.h"
#include "http_server.h"
#include "file_handler.h"

using namespace isaword;
using boost::shared_ptr;
/*---------------------------------------------------------
                    http_utils tests.
----------------------------------------------------------*/
BOOST_AUTO_TEST_SUITE(http_utils_tests)

BOOST_AUTO_TEST_CASE(uri_path_simple_request) {
    std::string uri("http://www.test.com/someplace");
    std::string path(uri_path(uri.c_str()));
    
    BOOST_CHECK_EQUAL(uri, path);
}
    
BOOST_AUTO_TEST_CASE(uri_path_with_query_args) {
    std::string uri("http://www.test.com/someplace?q=blah&df=10");
    std::string expected_path("http://www.test.com/someplace");
    std::string path(uri_path(uri.c_str()));
    
    BOOST_CHECK_EQUAL(expected_path, path);
}

BOOST_AUTO_TEST_SUITE_END()

/*---------------------------------------------------------
                    UriHandler tests.
----------------------------------------------------------*/
// Test callback function and some values to work with.
int gValueToChange = 0;
int gChangeValueTo = 456;
const int kDefaultToValue = 846;

void change_test_value(struct evhttp_request* /* ignore */, void* to_value) {
    if (to_value == NULL) {
        gValueToChange = kDefaultToValue;
        
    } else {
        gValueToChange = *((int*)to_value);
    }
}

void reset_global_values() {
    gValueToChange = 0;
    gChangeValueTo = 456;
}

BOOST_AUTO_TEST_SUITE(UriHandler_tests)

BOOST_AUTO_TEST_CASE(constructor) {
    UriHandler uri_handler;
    
    boost::regex empty_regex;
    
    BOOST_CHECK_EQUAL(uri_handler.pattern().compare(empty_regex), 0);
    BOOST_CHECK_EQUAL(uri_handler.handler(), (void*)NULL);
    BOOST_CHECK_EQUAL(uri_handler.handler_data(), (void*)NULL);
}

BOOST_AUTO_TEST_CASE(initialize_test_assignment) {
    std::string pattern_string("fo(o)*");
    boost::regex pattern(pattern_string);
    
    UriHandler uri_handler;
    uri_handler.initialize(pattern_string, &change_test_value, &gChangeValueTo);
    
    BOOST_CHECK_EQUAL(uri_handler.pattern().compare(pattern), 0);
    BOOST_CHECK_EQUAL(uri_handler.handler(), &change_test_value);
    BOOST_CHECK_EQUAL(uri_handler.handler_data(), &gChangeValueTo);
}

BOOST_AUTO_TEST_CASE(inoke_callback_with_default_arg) {
    reset_global_values();
    std::string pattern_string("fo(o)*");
    boost::regex pattern(pattern_string);
    
    UriHandler uri_handler;
    uri_handler.initialize(pattern_string, &change_test_value);
    
    struct evhttp_request* no_request = NULL;
    const bool has_called = uri_handler.handle_if_matched("foo", no_request);

    BOOST_CHECK(has_called);
    BOOST_CHECK_EQUAL(gValueToChange, kDefaultToValue);
}

BOOST_AUTO_TEST_CASE(inoke_callback_with_custom_arg) {
    reset_global_values();
    std::string pattern_string("fo(o)*");
    boost::regex pattern(pattern_string);
    
    UriHandler uri_handler;
    uri_handler.initialize(pattern_string, &change_test_value, &gChangeValueTo);
    
    struct evhttp_request* no_request = NULL;
    const bool has_called = uri_handler.handle_if_matched("foo", no_request);

    BOOST_CHECK(has_called);
    BOOST_CHECK_EQUAL(gValueToChange, gChangeValueTo);
}

BOOST_AUTO_TEST_CASE(inoke_callback_no_match) {
    reset_global_values();
    const int starting_value = gValueToChange;
    std::string pattern_string("fo(o)*");
    boost::regex pattern(pattern_string);
    
    UriHandler uri_handler;
    uri_handler.initialize(pattern_string, &change_test_value, &gChangeValueTo);

    struct evhttp_request* no_request = NULL;
    const bool has_called = uri_handler.handle_if_matched("bar", no_request);

    BOOST_CHECK(!has_called);
    BOOST_CHECK_EQUAL(gValueToChange, starting_value);
}
BOOST_AUTO_TEST_SUITE_END()

/*---------------------------------------------------------
                    HttpServer tests.
----------------------------------------------------------*/
BOOST_AUTO_TEST_SUITE(HttpServer_tests)

BOOST_AUTO_TEST_CASE(constructor) {
    HttpServer http_server;
    
    BOOST_CHECK_EQUAL(http_server.ev_base(), ((struct event_base*)NULL));
    BOOST_CHECK_EQUAL(http_server.ev_server(), ((struct evhttp*)NULL));
}

BOOST_AUTO_TEST_CASE(initialize) {
    HttpServer http_server;
    shared_ptr<UriHandler> empty_handler;

    http_server.initialize();
    BOOST_CHECK_NE(http_server.not_found_handler(), empty_handler);

}

BOOST_AUTO_TEST_SUITE_END()

/*---------------------------------------------------------
                    FileHandler tests.
----------------------------------------------------------*/
class FileHandlerFixture {
public:
    FileHandlerFixture() {}
    
    ~FileHandlerFixture() {}

    FileHandler file_handler;
};

class FileHandlerWithRootFixture {
public:
    FileHandlerWithRootFixture() {
        file_handler = shared_ptr<FileHandler>(new FileHandler());
        file_handler->initialize(".");
        server = shared_ptr<HttpServer>(new HttpServer());
        server->initialize();
        
        buffer = new char[buffer_size];
        memset(buffer, '\0', buffer_size);
    }
    
    ~FileHandlerWithRootFixture() {
        delete [] buffer;
    }
    
    char* buffer;
    static const std::streamsize buffer_size = 2048;
    shared_ptr<FileHandler> file_handler;
    shared_ptr<HttpServer> server;
};

/* ============ set_file_root tests ==============*/
BOOST_FIXTURE_TEST_SUITE(FileHandler_initialize_tests, FileHandlerFixture)

BOOST_AUTO_TEST_CASE(set_file_root_ok) {
    BOOST_CHECK_EQUAL(file_handler.initialize("."), FileHandler::FILE_ROOT_OK);
    BOOST_CHECK_EQUAL(file_handler.file_root(), "./");
}

BOOST_AUTO_TEST_CASE(set_file_root_ok_backslash) {
    BOOST_CHECK_EQUAL(file_handler.initialize("./"), FileHandler::FILE_ROOT_OK);
    BOOST_CHECK_EQUAL(file_handler.file_root(), "./");
}

BOOST_AUTO_TEST_CASE(set_file_root_not_found) {
    BOOST_CHECK_EQUAL(file_handler.initialize("no_such_dir"), 
                      FileHandler::FILE_ROOT_NOT_FOUND);
    BOOST_CHECK_EQUAL(file_handler.file_root(), "");
}

BOOST_AUTO_TEST_CASE(set_file_root_not_a_dir) {
    BOOST_CHECK_EQUAL(file_handler.initialize("test_isawordd"), 
                      FileHandler::FILE_ROOT_NOT_A_DIRECTORY);
    BOOST_CHECK_EQUAL(file_handler.file_root(), "");
}

BOOST_AUTO_TEST_SUITE_END()

/* ============ attach_to_server tests ==============*/
BOOST_FIXTURE_TEST_SUITE(FileHandler_attach_to_server_tests, FileHandlerWithRootFixture)

BOOST_AUTO_TEST_CASE(attach_to_server_ok_with_slash) {
    BOOST_CHECK_EQUAL(file_handler->attach_to_server(server, "/root/"),
                      FileHandler::ATTACHED_OK);

    BOOST_CHECK(file_handler->is_attached());
    BOOST_CHECK_EQUAL(file_handler->url_root(), "/root/");
    
    //Check whether the appropriate UriHandler is present.
    std::vector<shared_ptr<UriHandler> > uri_handlers = server->uri_handlers();
    
    BOOST_REQUIRE_EQUAL(uri_handlers.size(), 1);
    shared_ptr<UriHandler> uri_handler = uri_handlers[0];
    
    boost::regex expected_pattern("/root/.*");
    BOOST_CHECK_EQUAL(uri_handler->pattern().compare(expected_pattern), 0);
    BOOST_CHECK_EQUAL(uri_handler->handler(), (void*) &FileHandler::handler_callback);
    BOOST_CHECK_EQUAL(uri_handler->handler_data(), (void*) file_handler.get());
}

BOOST_AUTO_TEST_CASE(attach_to_server_ok_without_slash) {
    BOOST_CHECK_EQUAL(file_handler->attach_to_server(server, "/root"),
                      FileHandler::ATTACHED_OK);
    
    BOOST_CHECK(file_handler->is_attached());
    BOOST_CHECK_EQUAL(file_handler->url_root(), "/root/");
    
    //Check whether the appropriate UriHandler is present.
    std::vector<shared_ptr<UriHandler> > uri_handlers = server->uri_handlers();
    
    BOOST_REQUIRE_EQUAL(uri_handlers.size(), 1);
    shared_ptr<UriHandler> uri_handler = uri_handlers[0];
    
    boost::regex expected_pattern("/root/.*");
    BOOST_CHECK_EQUAL(uri_handler->pattern().compare(expected_pattern), 0);
    BOOST_CHECK_EQUAL(uri_handler->handler(), (void*) &FileHandler::handler_callback);
    BOOST_CHECK_EQUAL(uri_handler->handler_data(), (void*) file_handler.get());
}

// TODO: check invalid expression for URL root.

BOOST_AUTO_TEST_CASE(attach_without_file_root) {
    shared_ptr<FileHandler> empty_handler(new FileHandler);
    BOOST_CHECK_EQUAL(empty_handler->attach_to_server(server, "/root"),
                      FileHandler::NO_FILE_ROOT_SET);
    
    BOOST_CHECK(!file_handler->is_attached());
    BOOST_CHECK_EQUAL(empty_handler->url_root(), "");
    
    //Check that no UriHandlers were added.
    std::vector<shared_ptr<UriHandler> > uri_handlers = server->uri_handlers();
    BOOST_CHECK_EQUAL(uri_handlers.size(), 0);
}

BOOST_AUTO_TEST_CASE(attach_twice) {
    BOOST_CHECK_EQUAL(file_handler->attach_to_server(server, "/root"),
                      FileHandler::ATTACHED_OK);
    BOOST_CHECK_EQUAL(file_handler->attach_to_server(server, "/leaf"),
                      FileHandler::ALREADY_ATTACHED);
    
    BOOST_CHECK(file_handler->is_attached());
    BOOST_CHECK_EQUAL(file_handler->url_root(), "/root/");
    
    //Check that no UriHandlers were added.
    std::vector<shared_ptr<UriHandler> > uri_handlers = server->uri_handlers();
    BOOST_REQUIRE_EQUAL(uri_handlers.size(), 1);
    shared_ptr<UriHandler> uri_handler = uri_handlers[0];
    
    boost::regex expected_pattern("/root/.*");
    BOOST_CHECK_EQUAL(uri_handler->pattern().compare(expected_pattern), 0);
    BOOST_CHECK_EQUAL(uri_handler->handler(), (void*) &FileHandler::handler_callback);
    BOOST_CHECK_EQUAL(uri_handler->handler_data(), (void*) file_handler.get());
}

BOOST_AUTO_TEST_SUITE_END()

/* ============ read_file tests ==============*/
BOOST_FIXTURE_TEST_SUITE(FileHandler_read_file_tests, FileHandlerWithRootFixture)

BOOST_AUTO_TEST_CASE(read_valid_file) {
    const char* expected_data = 
        "0DlgZXgqFAYdDe8CZhS2FWP6yP0\nknAqBkd4z0SunDsUIdz7MrQ01eiMzUYf\n";
    
    size_t bytes_read = 0;
    bool succeeded = 
        file_handler->read_file(std::string("testing/test_file"), 
                                buffer,
                                buffer_size,
                                bytes_read);
    
    BOOST_CHECK(succeeded);
    BOOST_CHECK_EQUAL(strcmp(buffer, expected_data), 0);
}

BOOST_AUTO_TEST_CASE(read_invalid_file) {
    size_t bytes_read = 0;
    bool succeeded = 
        file_handler->read_file("testing/invalid.file", buffer, buffer_size, bytes_read);
    
    BOOST_CHECK(!succeeded);
    BOOST_CHECK_EQUAL(bytes_read, 0);
    BOOST_CHECK_EQUAL(strcmp(buffer, ""), 0);
}

BOOST_AUTO_TEST_CASE(read_backtrack_file) {
    size_t bytes_read = 0;
    bool succeeded = 
        file_handler->read_file("testing/../testing/test_file", 
                                buffer, 
                                buffer_size, 
                                bytes_read);
    
    BOOST_CHECK(!succeeded);
    BOOST_CHECK_EQUAL(bytes_read, 0);
    BOOST_CHECK_EQUAL(strcmp(buffer, ""), 0);
}
    
BOOST_AUTO_TEST_CASE(read_directory) {
    size_t bytes_read = 0;
    bool succeeded = 
        file_handler->read_file("..", 
                                buffer, 
                                buffer_size, 
                                bytes_read);
    
    BOOST_CHECK(!succeeded);
    BOOST_CHECK_EQUAL(bytes_read, 0);
    BOOST_CHECK_EQUAL(strcmp(buffer, ""), 0);
}

/* ============ is_permitted_file_path tests ==============*/
BOOST_FIXTURE_TEST_SUITE(FileHandler_is_permitted_file_path_tests, FileHandlerWithRootFixture)

// Good path tests
BOOST_AUTO_TEST_CASE(good_path1) {
    BOOST_CHECK(file_handler->is_permitted_file_path("file"));
}

BOOST_AUTO_TEST_CASE(good_path2) {
    BOOST_CHECK(file_handler->is_permitted_file_path("file.txt"));
}

BOOST_AUTO_TEST_CASE(good_path3) {
    BOOST_CHECK(file_handler->is_permitted_file_path("file..txt"));
}

BOOST_AUTO_TEST_CASE(good_path4) {
    BOOST_CHECK(file_handler->is_permitted_file_path("file-path.txt"));
}

BOOST_AUTO_TEST_CASE(good_path5) {
    BOOST_CHECK(file_handler->is_permitted_file_path("file_path.txt"));
}

BOOST_AUTO_TEST_CASE(good_path6) {
    BOOST_CHECK(file_handler->is_permitted_file_path("file_path.tx_t"));
}

BOOST_AUTO_TEST_CASE(good_path7) {
    BOOST_CHECK(file_handler->is_permitted_file_path("file_path.tx-t"));
}

BOOST_AUTO_TEST_CASE(good_path8) {
    BOOST_CHECK(file_handler->is_permitted_file_path("file.twodot.txt"));
}

BOOST_AUTO_TEST_CASE(good_path9) {
    BOOST_CHECK(file_handler->is_permitted_file_path("good/file"));
}

BOOST_AUTO_TEST_CASE(good_path10) {
    BOOST_CHECK(file_handler->is_permitted_file_path("good/file.txt"));
}

BOOST_AUTO_TEST_CASE(good_path11) {
    BOOST_CHECK(file_handler->is_permitted_file_path("good.path.to/file.txt"));
}

BOOST_AUTO_TEST_CASE(good_path12) {
    BOOST_CHECK(file_handler->is_permitted_file_path("good/file.twodot.txt"));
}

BOOST_AUTO_TEST_CASE(good_path13) {
    BOOST_CHECK(file_handler->is_permitted_file_path("good/comple-x/file"));
}

BOOST_AUTO_TEST_CASE(good_path14) {
    BOOST_CHECK(file_handler->is_permitted_file_path("good/dott.ed_path/file"));
}

BOOST_AUTO_TEST_CASE(good_path15) {
    BOOST_CHECK(file_handler->is_permitted_file_path("good/dott.ed_path/file.txt"));
}

BOOST_AUTO_TEST_CASE(good_path16) {
    BOOST_CHECK(file_handler->is_permitted_file_path("good/dott.ed-path/file.twodot.txt"));
}

// Bad path tests
BOOST_AUTO_TEST_CASE(bad_path0) {
    BOOST_CHECK(!file_handler->is_permitted_file_path(""));
}

BOOST_AUTO_TEST_CASE(bad_path1) {
    BOOST_CHECK(!file_handler->is_permitted_file_path(".."));
}

BOOST_AUTO_TEST_CASE(bad_path2) {
    BOOST_CHECK(!file_handler->is_permitted_file_path("../file"));
}

BOOST_AUTO_TEST_CASE(bad_path3) {
    BOOST_CHECK(!file_handler->is_permitted_file_path("../file.txt"));
}

BOOST_AUTO_TEST_CASE(bad_path4) {
    BOOST_CHECK(!file_handler->is_permitted_file_path("/"));
}

BOOST_AUTO_TEST_CASE(bad_path5) {
    BOOST_CHECK(!file_handler->is_permitted_file_path("/bad"));
}

BOOST_AUTO_TEST_CASE(bad_path6) {
    BOOST_CHECK(!file_handler->is_permitted_file_path("/bad/path"));
}

BOOST_AUTO_TEST_CASE(bad_path7) {
    BOOST_CHECK(!file_handler->is_permitted_file_path("/bad/path/file.txt"));
}

BOOST_AUTO_TEST_CASE(bad_path8) {
    BOOST_CHECK(!file_handler->is_permitted_file_path("../bad/file.txt"));
}

BOOST_AUTO_TEST_CASE(bad_path9) {
    BOOST_CHECK(!file_handler->is_permitted_file_path("/bad/../file.txt"));
}

BOOST_AUTO_TEST_CASE(bad_path10) {
    BOOST_CHECK(!file_handler->is_permitted_file_path("/bad/../../file.txt"));
}

BOOST_AUTO_TEST_CASE(bad_path11) {
    BOOST_CHECK(!file_handler->is_permitted_file_path(".hidden"));
}

BOOST_AUTO_TEST_CASE(bad_path12) {
    BOOST_CHECK(!file_handler->is_permitted_file_path("bad/.hidden"));
}

BOOST_AUTO_TEST_CASE(bad_path13) {
    BOOST_CHECK(!file_handler->is_permitted_file_path("bad/.hidden.file"));
}


BOOST_AUTO_TEST_SUITE_END()



BOOST_AUTO_TEST_SUITE_END()


