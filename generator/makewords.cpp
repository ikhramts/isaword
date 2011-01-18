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

//This program generates English language pseudowords given a
//dictionary of real words.  
//Usage: 
// $ ./generate <num_words> <dictionary_file>
// e.g
// $ ./generate 10000 dict.txt
//
//The dictionary is assumed to have one valid word per line.
//The words may consist only of letters A-Z, and may not 
//contain spaces, apostrophes, hyphens, or any other
//characters.

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/regex/pattern_except.hpp>
#include "pseudoword_generator.h"

using boost::shared_ptr;
using makewords::PseudowordGenerator;

const size_t kReadBufferSize = 30;

int main(int argc, char* argv[]) {
    //Validate the inputs.
    //Check for correct number of arguments.
    bool is_valid = true;
    std::stringstream error_message;
    
    if (argc != 3 && argc != 4) {
        error_message << "Error: received " << (argc - 1) 
                      << " arguments, expected 2" << std::endl;
        is_valid = false;
    }
    
    //Attempt to convert the first argument to integer.
    int num_words_to_generate = 0;
    if (is_valid) {
        try {
            num_words_to_generate = boost::lexical_cast<int>(argv[1]);
        
        } catch (boost::bad_lexical_cast &) {
            is_valid = false;
            error_message << "Error: first argument should be an integer; "
                          << "received \"" << argv[1] << "\" instead." << std::endl;
        }
    }
    
    //Attempt to open the file listed in the second argument.
    std::ifstream dictionary_file;
    if (is_valid) {
        dictionary_file.open(argv[2], std::ifstream::in);  
        
        if (dictionary_file.fail()) {
            is_valid = false;
            error_message << "Error: cannot open file " << argv[2] << std::endl;
        }
    }
    
    //Attempt to read and compile the regex criteria.
    bool has_criteria = false;
    boost::regex criteria;
    if (is_valid && 4 <= argc) {
        has_criteria = true;
        
        try {
            criteria = boost::regex(argv[3]);
        
        } catch (boost::bad_expression&) {
            is_valid = false;
            error_message << "Invalid criteria argument: " << argv[3] << std::endl;
        }
    }
    
    //Let the user know if there's an error.
    if (!is_valid) {
        std::cerr << error_message.str();
        std::cerr << "Usage:" << std::endl;
        std::cerr << "    makewords <num_words> <dictionary_file> [<criteria>]" << std::endl;
        return 1;
    }
    
    //Load the files into the random word generator.
    std::string alphabet("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    shared_ptr<PseudowordGenerator> generator(new PseudowordGenerator(alphabet));
    char buffer[kReadBufferSize];
    bool found_bad_word = false;
    int line_number = 0;
    std::string word;
    
    while (!dictionary_file.eof()) {
        line_number++;
        dictionary_file.getline(buffer, kReadBufferSize);
        word = buffer;
        
        //Skip blank lines.
        if (word.length() == 0) {
            continue;
        }
        
        //Attempt to add the word.
        if (!generator->add_dictionary_word(word)) {
            found_bad_word = true;
            break;
        }
    }
    
    if (found_bad_word) {
        error_message << "Error in dictionary file on line " << line_number
                      << ": Word \"" << word << "\" does not is empty or has"
                      << " prohibited characters.";
        std::cerr << error_message.str() << std::endl;
        
        dictionary_file.close();
        return 1;
    }
    
    dictionary_file.close();
    
    //Generate the requested number of words.
    generator->prepare_for_generation();
    
    for (int i = 0; i < num_words_to_generate; ++i) {
        if (has_criteria) {
            std::cout << generator->make_word(criteria) << std::endl;
        } else {
            std::cout << generator->make_word() << std::endl;
        }
    }
    
    return 0;
}




