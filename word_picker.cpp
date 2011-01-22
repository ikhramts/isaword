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

// This is the header for the module responsible for picking
// lists of fake and real words to be guessed.
#include <string>
#include <vector>
#include <utility>
#include <fstream>

#include <boost/random.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_01.hpp>
#include <boost/random/variate_generator.hpp>

#include "generator/pseudoword_generator.h"
#include "word_picker.h"

using boost::shared_ptr;

namespace isaword {

/*---------------------------------------------------------
                    WordPicker class.
----------------------------------------------------------*/
const size_t WordPicker::ALL_VOWELS;
const size_t WordPicker::ALL_CONSONANTS;
const size_t WordPicker::ALL_VOWELS_BUT_ONE;
const size_t WordPicker::Q_WORDS;
const size_t WordPicker::Z_WORDS;
const size_t WordPicker::X_WORDS;
const size_t WordPicker::J_WORDS;

/**
 * Ininitalize the word picker by providing it a path
 * to a dictionary to work with.
 *
 * @return true if the initialization was successfull, 
 * false otherwise.
 */
bool WordPicker::initialize(const std::string& dictionary_path) {
    indexes_.resize(index_descriptions_.size());
    
    // Load the dictionary line by line, adding the words to the
    // pseudorandom word generator, the in-memory dictionary,
    // and all the indexes.
    // 
    // We assume that the dictionary contains words in order of
    // increasing length.
    std::ifstream dictionary_file(dictionary_path.c_str());
    if (dictionary_file.bad()) {
        dictionary_file.close();
        return false;
    }
    
    const size_t buffer_size = 500;
    char buffer[buffer_size];
    std::string line;
    std::string word;
    std::string description;
    words_by_length_.reserve(200000);
    
    size_t current_length = 2;
    word_length_ends_.push_back(0);
    word_length_ends_.push_back(0);    
    
    while(!dictionary_file.eof()) {
        // Read the word data.
        dictionary_file.getline(buffer, buffer_size);
        line = &(buffer[0]);
        const size_t first_space = line.find_first_of(' ');
        word = line.substr(0, first_space);
        description = line.substr(first_space + 1);
        
        // Put the word into the main index.
        WordDescriptionPtr word_description(new WordDescription);
        word_description->word = word;
        word_description->description = description;
        word_description->is_real = true;
        
        words_by_length_.push_back(word_description);
        
        // Check whether this block of words by length
        // is over.
        if (word.length() > current_length) {
            word_length_ends_.push_back(current_length);
            current_length++;
        }
        
        // Add the word to the various indexes.
        for (size_t i = 0; i < index_descriptions_.size(); ++i) {
            if (index_descriptions_[i]->should_be_indexed(word)) {
                indexes_[i].push_back(word_description);
            }
        }
        
        // Add the word to the pseudoword generator.
        pseudoword_generator_->add_dictionary_word(word);
    }
    
    pseudoword_generator_->prepare_for_generation();
}

/**
 * Pick a number of words by length.
 */
std::vector<WordDescriptionPtr> WordPicker::get_words_by_length(size_t from, 
                                                                size_t to, 
                                                                size_t num_words) {
    //TODO: Implement.
    std::vector<WordDescriptionPtr> empty_vector;
    return empty_vector;
}

/**
 * Pick a number of words satisfying a certain criteria.
 */
std::vector<WordDescriptionPtr> WordPicker::get_words_from_index(size_t index, size_t num_words) {
    //TODO: Implement.
    std::vector<WordDescriptionPtr> empty_vector;
    return empty_vector;
}

} /* namespace isaword */
