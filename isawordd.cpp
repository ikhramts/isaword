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
#include <boost/filesystem.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>
#include <boost/program_options.hpp>
#include "daemonize.h"
#include "file_handler.h"
#include "http_server.h"
#include "views.h"

using boost::shared_ptr;
using boost::shared_array;
using namespace isaword;
namespace po = boost::program_options;

int main(int argc, char* argv[]) {
    //Process options.
    po::options_description options("Usage:\n    isawordd [options]\n\nOptions");
    options.add_options()
        ("help,h", "produce help message")
        ("ip,i", 
         po::value<std::vector<std::string> >(), 
         "IP address to listen on (default: 0.0.0.0)")
        ("log_file,l", 
         po::value<std::vector<std::string> >(), 
         "log file")
        ("no_daemon,d", "do not run as a daemon")
        ("port,p", po::value<int>(), "port to listen on (default: 80)")
        ("res_root,r", 
         po::value<std::vector<std::string> >(), 
         "root directory for server resources (default: current dir)");
    
    po::variables_map args;
    po::store(po::parse_command_line(argc, argv, options), args);
    po::notify(args);
    
    if (args.count("help")) {
        std::cout << options << std::endl;
        return 0;
    }
    
    // Get the IP to serve on.
    std::string listen_ip("0.0.0.0");

    if (args.count("ip")) {
        listen_ip = args["port"].as<std::vector<std::string> >()[0];
    }
    
    // Get the port to serve on.
    u_short listen_port = 80;

    if (args.count("port")) {
        listen_port = static_cast<u_short>(args["port"].as<int>());
    }
    
    // Get the root for the resource files.
    std::string resource_dir;
    
    if (args.count("res_root")) {
        resource_dir = args["res_root"].as<std::vector<std::string> >()[0];
        
        if (resource_dir[resource_dir.length() - 1] != '/') {
            resource_dir += '/';
        }
    
    } else {
        //Find the current directory.
        resource_dir = 
            boost::filesystem::current_path<boost::filesystem::path>().string();
        
        if (resource_dir[resource_dir.length() - 1] != '/') {
            resource_dir += '/';
        }
        
        std::cout << "WARNING: no resource path was specified.  Using"
                  << " current directory (" << resource_dir << ")." << std::endl;
    }
    
    // Get logging file name, if such exists.
    //shared_array<char> log_file_name;
    shared_array<char> log_file_name;
    
    if (args.count("log_file")) {
        std::string relative_log_file_path= 
            args["log_file"].as<std::vector<std::string> >()[0];
        
        // Convert the log file name to absolute path.
        boost::filesystem::path log_path(relative_log_file_path);
        std::string s_log_file_name = log_path.string();
        
        if (s_log_file_name[0] != '/') {
            //Prepend current dir.
            s_log_file_name = 
                boost::filesystem::current_path<boost::filesystem::path>().string() +
                    '/' + 
                    s_log_file_name;
        }
        
        log_file_name = shared_array<char>(new char[s_log_file_name.length() + 1]);
        memcpy(log_file_name.get(), s_log_file_name.c_str(), s_log_file_name.length());
        log_file_name[s_log_file_name.length()] = '\0';
        
        std::cout << "Logging to " << log_file_name.get() << std::endl;
        
    } else {
        std::cout << "No log file specified; sending logs to /dev/null." << std::endl;
    }
        
    std::cout << "Preparing to serve on " << listen_ip 
              << ":" << listen_port << std::endl;
    
    if (!args.count("no_daemon")) {
        pid_t pid = daemonize(log_file_name.get());
        
        if (pid < 0) {
            // Cound not create the daemon.
            std::cerr << "Could not launch daemon process." << std::endl;
            
        } else if (pid > 0) {
            // This is the parent process.  It's done now.
            std::cout << "Launched isawordd daemon." << std::endl;
            return 0;
        }
    
    } else {
        std::cout << "Starting in non-daemon mode." << std::endl;
    }
    
    // Set up the server.
    shared_ptr<HttpServer> server(new HttpServer());
    server->initialize();
    
    // This part of the server is responsible for loading files.
    shared_ptr<FileHandler> file_handler(new FileHandler(0 /* cache period */));
    file_handler->initialize(resource_dir + "resources/");
    file_handler->attach_to_server(server, "/resources/");
    
    //Add some pages to the server.
    shared_ptr<PageHandler> page_handler(new PageHandler(server));
    page_handler->initialize(resource_dir);
    
    server->serve(listen_ip, listen_port);
    return 0;
}

