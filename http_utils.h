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

// Various useful HTTP-related utilities.

#ifndef ISAWORD_HTTP_UTILS_H
#define ISAWORD_HTTP_UTILS_H

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

/// Extract the request URI without the query arguments.
std::string uri_path(const char* uri);


/// Extract the request URI without the query arguments from
/// an evhttp_request struct.
std::string request_uri_path(struct evhttp_request* request);




} /* namespace isaword */
#endif