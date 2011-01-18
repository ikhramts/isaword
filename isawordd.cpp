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

#include <iostream>
#include <boost/shared_ptr.hpp>
#include "http_server.h"
#include "file_handler.h"

using boost::shared_ptr;
using namespace isaword;

int main() {
    //Set up a simple file server.
    shared_ptr<HttpServer> server(new HttpServer());
    server->initialize();
    
    shared_ptr<FileHandler> file_handler(new FileHandler());
    file_handler->initialize(".");
    file_handler->attach_to_server(server, "/t/");
    file_handler->set_cache_control("public, max-age=0");
    
    server->serve("0.0.0.0", 5574);
    return 0;
}

