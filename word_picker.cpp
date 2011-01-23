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
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <utility>

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
    size_t current_word_index = 0;
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
            word_length_ends_.push_back(current_word_index);
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
        current_word_index++;
    }
    
    word_length_ends_.push_back(current_word_index);
    pseudoword_generator_->prepare_for_generation();
    
    // Initialize the regex patterns for max and min word lengths.
    const size_t num_word_lengths = max_word_length_ + 1 - min_word_length_;
    const size_t num_length_patterns = num_word_lengths * (num_word_lengths + 1) / 2;
    word_length_patterns_.reserve(num_length_patterns);
    
    for (size_t to = min_word_length_; to <= max_word_length_; to++) {
        for (size_t from = min_word_length_; from <= to; from++) {
            std::stringstream word_length_pattern;
            word_length_pattern << "^.{" << from << ',' << to << "}$";
            boost::regex word_length_regex(word_length_pattern.str());
            word_length_patterns_.push_back(word_length_regex);
        }
    }
    
    return true;
}

/**
 * Pick a number of words by length.
 */
std::vector<WordDescriptionPtr> WordPicker::get_words_by_length(size_t from, 
                                                                size_t to, 
                                                                size_t num_words) {
    std::vector<WordDescriptionPtr> words;
    if (from > to || num_words == 0) {
        return words;
    }
    
    words.reserve(num_words);
    
    // Find the range of real words to pick from.
    const size_t first_possible_word = word_length_ends_[from - 1];
    const size_t end_of_possible_words = word_length_ends_[to];
    const size_t num_possible_words = end_of_possible_words - first_possible_word;
    
    //Find the right fake word pattern to use.
    const size_t length_pattern_index = 
        (to - min_word_length_) * (to - min_word_length_ + 1) / 2 + (from - min_word_length_);
    boost::regex& length_pattern = word_length_patterns_[length_pattern_index];
    
    //Compose a list of words.
    for (size_t i = 0; i < num_words; i++) {
        //Decide whether this word will be real or fake.
        //TODO: remove duplicates.
        if (0.5 > random_01_()) {
            // Real word.
            const double d_word_offset = static_cast<double>(num_possible_words) *random_01_();
            const size_t word_index = static_cast<size_t>(d_word_offset) + first_possible_word;
            words.push_back(words_by_length_[word_index]);
            
        } else {
            // Fake word.
            WordDescriptionPtr fake_word(new WordDescription());
            fake_word->word = pseudoword_generator_->make_word(length_pattern);
            fake_word->description = "";
            fake_word->is_real = false;
            words.push_back(fake_word);
        }
    }
    
    return words;
}

/**
 * Pick a number of words satisfying a certain criteria.
 */
std::vector<WordDescriptionPtr> WordPicker::get_words_from_index(size_t index_num, size_t num_words) {
    std::vector<WordDescriptionPtr> words;
    if (index_num >= index_descriptions_.size() || num_words == 0) {
        return words;
    }
    
    words.reserve(num_words);
    
    // Find the index to select the words from.
    shared_ptr<WordIndexDescription> index_description = index_descriptions_[index_num];
    std::vector<WordDescriptionPtr>& index = indexes_[index_num];
    const double index_size = static_cast<double>(index.size());
    
    //Compose a list of words.
    for (size_t i = 0; i < num_words; i++) {
        //Decide whether this word will be real or fake.
        //TODO: remove duplicates.
        if (0.5 > random_01_()) {
            // Real word.
            const size_t word_position = static_cast<size_t>(random_01_() * index_size);
            words.push_back(index[word_position]);
            
        } else {
            // Fake word.
            WordDescriptionPtr fake_word(new WordDescription());
            fake_word->word = pseudoword_generator_->make_word(index_description->pattern());
            fake_word->description = "";
            fake_word->is_real = false;
            words.push_back(fake_word);
        }
    }
    
    return words;
}

} /* namespace isaword */
