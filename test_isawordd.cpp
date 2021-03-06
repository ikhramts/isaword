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

#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <unistd.h>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <boost/regex/pattern_except.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>
#include <boost/test/unit_test.hpp>
#include "http_utils.h"
#include "http_server.h"
#include "file_handler.h"
#include "file_cache.h"
#include "word_picker.h"

using namespace isaword;
using boost::shared_ptr;
using boost::shared_array;
using namespace boost::filesystem;
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

BOOST_AUTO_TEST_CASE(time_to_string_test) {
    BOOST_CHECK_EQUAL(
        strcmp(time_to_string((time_t)1295906542).get(), "Mon, 24 Jan 2011 22:02:22 GMT"), 0);
    BOOST_CHECK_EQUAL(
        strcmp(time_to_string((time_t)1295906673).get(), "Mon, 24 Jan 2011 22:04:33 GMT"), 0);
    BOOST_CHECK_EQUAL(
        strcmp(time_to_string((time_t)1240596879).get(), "Fri, 24 Apr 2009 18:14:39 GMT"), 0);
}

BOOST_AUTO_TEST_CASE(string_to_time_test) {
    BOOST_CHECK_EQUAL(string_to_time("Mon, 24 Jan 2011 22:04:33 GMT"), 1295906673);
    BOOST_CHECK_EQUAL(string_to_time("Monday, 24 January 2011 23:3:30 GMT"), 1295910210);
    BOOST_CHECK_EQUAL(string_to_time("Fri, 24 Apr 2009 18:14:39 GMT"), 1240596879);
    BOOST_CHECK_EQUAL(string_to_time("Invalid"), 0);
    BOOST_CHECK_EQUAL(string_to_time((const char*) NULL), 0);
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

BOOST_AUTO_TEST_SUITE_END()

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

/*---------------------------------------------------------
                 Fixture for cache tests.
----------------------------------------------------------*/
class FileCacheFixture {
public:

    FileCacheFixture()
    : file_cache("") {
        // Initialize the main variables.
        starting_data = "cNlrkY2U4ZSKg5O83yQy";
        new_data = "Ceg0zBB8qY";
        file_name = "cache_test";
        
        data = shared_array<char>(new char[4]);
        data_size = 4;
        
        //Create the file to test.
        remove(file_name);
        file.open(file_name.c_str());
        file << starting_data;
        file.close();
        
        // Construct the path for the directory one above the
        // current one.
        boost::filesystem::path parent_path(current_path<path>().parent_path());
        one_up_path = parent_path.string();
        
        one_up_file_name = "test/" + file_name;
    }
    
    ~FileCacheFixture() {
        remove(file_name);
    }
    
    std::string starting_data;
    std::string new_data;
    static const size_t starting_data_size = 20;
    static const size_t new_data_size = 10;
    std::string file_name;
    std::ofstream file;
    
    shared_array<char> data;
    size_t data_size;
    shared_array<char> empty_ptr;
    
    FileCache file_cache;
    
    std::string one_up_path;
    std::string one_up_file_name;
};

const size_t FileCacheFixture::starting_data_size;
const size_t FileCacheFixture::new_data_size;

/*---------------------------------------------------------
                    CachedFile tests.
----------------------------------------------------------*/
BOOST_FIXTURE_TEST_SUITE(CachedFile_tests, FileCacheFixture)

BOOST_AUTO_TEST_CASE(constructor) {
    CachedFile cached_file("blah");
    
    BOOST_CHECK_EQUAL(cached_file.file_path(), "blah");
    BOOST_CHECK_EQUAL(cached_file.expiration_period(), CachedFile::kDefaultExpirationPeriod);
}

BOOST_AUTO_TEST_CASE(constructor_with_cache_period) {
    CachedFile cached_file("blah", 0);
    
    BOOST_CHECK_EQUAL(cached_file.file_path(), "blah");
    BOOST_CHECK_EQUAL(cached_file.expiration_period(), 0);
}

BOOST_AUTO_TEST_CASE(get_nonexistent_file) {
    CachedFile cached_file("no_such_file");
    
    BOOST_CHECK(!cached_file.get(data, data_size));
    BOOST_CHECK_EQUAL(data, empty_ptr);
    BOOST_CHECK_EQUAL(data_size, 0);
}

BOOST_AUTO_TEST_CASE(get_file) {
    CachedFile cached_file(file_name);

    BOOST_CHECK(cached_file.get(data, data_size));
    BOOST_REQUIRE_NE(data, empty_ptr);
    BOOST_REQUIRE_EQUAL(data_size, starting_data_size);
    BOOST_CHECK_EQUAL(memcmp(starting_data.c_str(), data.get(), starting_data_size), 0);
    
    BOOST_CHECK_EQUAL(data[data_size], '\0');
}

BOOST_AUTO_TEST_CASE(get_file_from_cache) {
    CachedFile cached_file(file_name);

    //Load the file to cache
    BOOST_CHECK(cached_file.get(data, data_size));
    
    //Rewrite the file.
    remove(file_name);
    file.open(file_name.c_str());
    file << new_data;
    file.close();
    
    //Load the data again.  Sould still be old data.
    BOOST_CHECK(cached_file.get(data, data_size));
    BOOST_REQUIRE_NE(data, empty_ptr);
    BOOST_REQUIRE_EQUAL(data_size, starting_data_size);
    BOOST_CHECK_EQUAL(memcmp(starting_data.c_str(), data.get(), starting_data_size), 0);
}

BOOST_AUTO_TEST_CASE(get_expired_file) {
    //File is modified after loading, and the cache has expired.
    CachedFile cached_file(file_name, 0);

    //Load the file to cache
    BOOST_CHECK(cached_file.get(data, data_size));
    sleep(2);
    
    //Rewrite the file.
    remove(file_name);
    file.open(file_name.c_str());
    file << new_data;
    file.close();
    
    //Load the data again.  Sould still be old data.
    BOOST_CHECK(cached_file.get(data, data_size));
    BOOST_REQUIRE_NE(data, empty_ptr);
    BOOST_REQUIRE_EQUAL(data_size, new_data_size);
    BOOST_CHECK_EQUAL(memcmp(new_data.c_str(), data.get(), new_data_size), 0);
}

BOOST_AUTO_TEST_CASE(get_deleted_file) {
    //Cache has expired and the file was deleted.
    CachedFile cached_file(file_name, 0);

    //Load the file to cache
    BOOST_CHECK(cached_file.get(data, data_size));
    
    //Rewrite the file.
    sleep(2);
    remove(file_name);
    
    //Load the data again.  Sould still be old data.
    BOOST_CHECK(!cached_file.get(data, data_size));
    BOOST_CHECK_EQUAL(data, empty_ptr);
    BOOST_CHECK_EQUAL(data_size, 0);
}

//TODO: test refresh()

BOOST_AUTO_TEST_SUITE_END()

/*---------------------------------------------------------
                    FileCache tests.
----------------------------------------------------------*/
BOOST_FIXTURE_TEST_SUITE(FileCache_tests, FileCacheFixture)

BOOST_AUTO_TEST_CASE(get_nonexistent_file) {
    BOOST_CHECK(!file_cache.get("no_such_file", data, &data_size));
    BOOST_CHECK_EQUAL(data, empty_ptr);
    BOOST_CHECK_EQUAL(data_size, 0);
}

BOOST_AUTO_TEST_CASE(get_nonexistent_file_no_data_size) {
    BOOST_CHECK(!file_cache.get("no_such_file", data));
    BOOST_CHECK_EQUAL(data, empty_ptr);
}

BOOST_AUTO_TEST_CASE(get_file) {
    BOOST_CHECK(file_cache.get(file_name, data, &data_size));
    BOOST_REQUIRE_NE(data, empty_ptr);
    BOOST_REQUIRE_EQUAL(data_size, starting_data_size);
    BOOST_CHECK_EQUAL(memcmp(starting_data.c_str(), data.get(), starting_data_size), 0);
    
    BOOST_CHECK_EQUAL(data[data_size], '\0');
}

BOOST_AUTO_TEST_CASE(get_file_no_data_size) {
    BOOST_CHECK(file_cache.get(file_name, data));
    BOOST_REQUIRE_NE(data, empty_ptr);
    BOOST_CHECK_EQUAL(memcmp(starting_data.c_str(), data.get(), starting_data_size), 0);
    
    BOOST_CHECK_EQUAL(data[starting_data_size], '\0');
}

BOOST_AUTO_TEST_CASE(set_file_root) {
    file_cache.set_file_root("test");
    BOOST_CHECK_EQUAL(file_cache.file_root(), "test/");
    
    file_cache.set_file_root("toast/");
    BOOST_CHECK_EQUAL(file_cache.file_root(), "toast/");
}

BOOST_AUTO_TEST_CASE(set_file_root_get_file) {
    file_cache.set_file_root(one_up_path);
    BOOST_CHECK(file_cache.get(one_up_file_name, data));
    BOOST_REQUIRE_NE(data, empty_ptr);
    BOOST_CHECK_EQUAL(memcmp(starting_data.c_str(), data.get(), starting_data_size), 0);
    
    BOOST_CHECK_EQUAL(data[starting_data_size], '\0');
}

BOOST_AUTO_TEST_CASE(get_file_from_cache) {
    //Load the file to cache
    file_cache.get(file_name, data, &data_size);
    
    //Rewrite the file.
    remove(file_name);
    file.open(file_name.c_str());
    file << new_data;
    file.close();
    
    //Load the data again.  Sould still be old data.
    BOOST_CHECK(file_cache.get(file_name, data, &data_size));
    BOOST_REQUIRE_NE(data, empty_ptr);
    BOOST_REQUIRE_EQUAL(data_size, starting_data_size);
    BOOST_CHECK_EQUAL(memcmp(starting_data.c_str(), data.get(), starting_data_size), 0);
}

BOOST_AUTO_TEST_CASE(get_expired_file) {
    //File is modified after loading, and the cache has expired.
    //Load the file to cache
    file_cache.set_expiration_period(0);
    file_cache.get(file_name, data, &data_size);
    sleep(2);
    
    //Rewrite the file.
    remove(file_name);
    file.open(file_name.c_str());
    file << new_data;
    file.close();
    
    //Load the data again.  Sould still be old data.
    BOOST_CHECK(file_cache.get(file_name, data, &data_size));
    BOOST_REQUIRE_NE(data, empty_ptr);
    BOOST_REQUIRE_EQUAL(data_size, new_data_size);
    BOOST_CHECK_EQUAL(memcmp(new_data.c_str(), data.get(), new_data_size), 0);
}

BOOST_AUTO_TEST_CASE(get_deleted_file) {
    //Cache has expired and the file was deleted.
    //Load the file to cache
    file_cache.set_expiration_period(0);
    file_cache.get(file_name, data, &data_size);
    
    //Rewrite the file.
    sleep(2);
    remove(file_name);
    
    //Load the data again.  Sould still be old data.
    BOOST_CHECK(!file_cache.get(file_name, data, &data_size));
    BOOST_CHECK_EQUAL(data, empty_ptr);
    BOOST_CHECK_EQUAL(data_size, 0);
}

BOOST_AUTO_TEST_SUITE_END()

/*---------------------------------------------------------
                    WordIndexDescription class.
----------------------------------------------------------*/
class WordIndexDescriptionFixture {
public:
    WordIndexDescriptionFixture() {
        index_description = 
            shared_ptr<WordIndexDescription>(new WordIndexDescription("index", 
                                                                      "Some Index",
                                                                      "^.*Z.*$"));
    }
    
    shared_ptr<WordIndexDescription> index_description;

};

BOOST_FIXTURE_TEST_SUITE(WordIndexDescription_tests, WordIndexDescriptionFixture)

BOOST_AUTO_TEST_CASE(initialization_test) {
    BOOST_CHECK_EQUAL(index_description->name(), "index");
    BOOST_CHECK_EQUAL(index_description->description(), "Some Index");
}

BOOST_AUTO_TEST_CASE(word_sorting_test) {
    BOOST_CHECK(index_description->should_be_indexed("BAAZAAR"));
    BOOST_CHECK(!index_description->should_be_indexed("HELLO"));
}

BOOST_AUTO_TEST_SUITE_END()

/*---------------------------------------------------------
                    WordPicker tests.
----------------------------------------------------------*/
class WordPickerFixture {
public:
    WordPickerFixture() {
        // Create the index descriptions.
        shared_ptr<WordIndexDescription> a_words(new WordIndexDescription("a", "a", ".*A.*"));
        shared_ptr<WordIndexDescription> ends_with_s(new WordIndexDescription("-a", "-s", ".*S$"));
        std::vector<shared_ptr<WordIndexDescription> > index_descriptions;
        index_descriptions.push_back(a_words);
        index_descriptions.push_back(ends_with_s);
        
        //Create the word picker.
        word_picker = shared_ptr<WordPicker>(new WordPicker(index_descriptions));
        word_picker->initialize("testing/simple_dictionary.txt");
    }
    
    shared_ptr<WordPicker> word_picker;
    WordDescriptionPtr empty_ptr;
    
};

BOOST_FIXTURE_TEST_SUITE(WordPicker_tests, WordPickerFixture)

BOOST_AUTO_TEST_CASE(initialization_test) {
    WordPicker::IndexList& indexes = word_picker->indexes();
    std::vector<WordDescriptionPtr>& words_by_length = word_picker->words_by_length();
    
    //Check the contents of the words_by_length.
    BOOST_REQUIRE_EQUAL(words_by_length.size(), 9);
    
    for(size_t i = 0; i < words_by_length.size(); i++) {
        BOOST_REQUIRE_NE(words_by_length[i], empty_ptr);
    }
    
    BOOST_CHECK_EQUAL(words_by_length[0]->word, "BE");
    BOOST_CHECK_EQUAL(words_by_length[1]->word, "BI");
    BOOST_CHECK_EQUAL(words_by_length[2]->word, "AAH");
    BOOST_CHECK_EQUAL(words_by_length[3]->word, "AAL");
    BOOST_CHECK_EQUAL(words_by_length[4]->word, "AAS");
    BOOST_CHECK_EQUAL(words_by_length[5]->word, "FEMS");
    BOOST_CHECK_EQUAL(words_by_length[6]->word, "FEND");
    BOOST_CHECK_EQUAL(words_by_length[7]->word, "HUIC");
    BOOST_CHECK_EQUAL(words_by_length[8]->word, "PAMS");
    
    BOOST_CHECK_EQUAL(words_by_length[0]->description, "to have actuality");
    BOOST_CHECK_EQUAL(words_by_length[1]->description, "a bisexual");
    BOOST_CHECK_EQUAL(words_by_length[2]->description, "to exclaim in amazement, joy, or surprise");
    BOOST_CHECK_EQUAL(words_by_length[3]->description, "an East Indian shrub");
    BOOST_CHECK_EQUAL(words_by_length[4]->description, "(see aa)");
    BOOST_CHECK_EQUAL(words_by_length[5]->description, "(see fem)");
    BOOST_CHECK_EQUAL(words_by_length[6]->description, "to ward off");
    BOOST_CHECK_EQUAL(words_by_length[7]->description, "used to encourage hunting hounds");
    BOOST_CHECK_EQUAL(words_by_length[8]->description, "(see pam)");
    
    // Check endings of size groups.
    std::vector<size_t> word_length_ends = word_picker->word_length_ends();
    
    BOOST_REQUIRE_EQUAL(word_length_ends.size(), 5);
    BOOST_CHECK_EQUAL(word_length_ends[0], 0);
    BOOST_CHECK_EQUAL(word_length_ends[1], 0);
    BOOST_CHECK_EQUAL(word_length_ends[2], 2);
    BOOST_CHECK_EQUAL(word_length_ends[3], 5);
    BOOST_CHECK_EQUAL(word_length_ends[4], 9);
    
    // Check the contents of the indexes.
    BOOST_REQUIRE_EQUAL(indexes.size(), 2);
    BOOST_REQUIRE_EQUAL(indexes[0].size(), 4);
    BOOST_REQUIRE_EQUAL(indexes[1].size(), 3);
    
    for (size_t i = 0; i < indexes.size(); i++) {
        for (size_t j = 0; j < indexes[i].size(); j++) {
            BOOST_REQUIRE_NE(indexes[i][j], empty_ptr);
        }
    }
    
    BOOST_CHECK_EQUAL(indexes[0][0]->word, "AAH");
    BOOST_CHECK_EQUAL(indexes[0][1]->word, "AAL");
    BOOST_CHECK_EQUAL(indexes[0][2]->word, "AAS");
    BOOST_CHECK_EQUAL(indexes[0][3]->word, "PAMS");
    BOOST_CHECK_EQUAL(indexes[1][0]->word, "AAS");
    BOOST_CHECK_EQUAL(indexes[1][1]->word, "FEMS");
    BOOST_CHECK_EQUAL(indexes[1][2]->word, "PAMS");
}


BOOST_AUTO_TEST_SUITE_END()

