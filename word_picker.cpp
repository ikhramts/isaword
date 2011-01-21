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

#include <boost/random.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_01.hpp>
#include <boost/random/variate_generator.hpp>

#include "generator/pseudoword_generator.h"
#include "word_picker.h"

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
 */
void WordPicker::initialize(const std::string& dictionary_path) {
    //TODO: implement;
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
