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

#include "pseudoword_generator.h"
#include <bitset>
#include <math.h>
#include <string>
#include <vector>
#include <boost/functional/hash.hpp>
#include <google/sparse_hash_set>
#include <time.h>
#include "utils.h"

using google::sparse_hash_set;

namespace makewords {
/*---------------------------------------------------------
                PseudowordGenerator class.
----------------------------------------------------------*/

const int PseudowordGenerator::kDefaultNumCondidiontingCharacters;

PseudowordGenerator::PseudowordGenerator(const std::string& alphabet)
:alphabet_(alphabet), 
num_conditioning_characters_(kDefaultNumCondidiontingCharacters),
preceding_chars_(kDefaultNumCondidiontingCharacters, alphabet),
random_numbers_generator_(time(0)),
uniform_01_(),
random_01_(random_numbers_generator_, uniform_01_) {
    const int alphabet_size = static_cast<int>(alphabet.size());
    num_matrix_rows_ = (alphabet_size + 1) * (alphabet_size + 1);
    num_matrix_columns_ = (alphabet_size + 1);
    const size_t matrix_size = static_cast<size_t>(num_matrix_rows_ * num_matrix_columns_);
    sampling_matrix_ = std::vector<int>(matrix_size, 0);
    transition_matrix_ = std::vector<double>(matrix_size, 0);
    
    //Initialize the column indexes of letters.
    column_indexes_.resize(kAlphabetSpaceSize, kNoColumnIndex);
    
    for (size_t i = 0; i < alphabet.size(); ++i) {
        column_indexes_[alphabet[i]] = i;
    }
}

bool PseudowordGenerator::initialize(size_t expected_dictionary_size) {
    //Initialize the hash set.
    dictionary_ = sparse_hash_set<std::string, boost::hash<std::string>, eqstr>(expected_dictionary_size);
    return true;
}


bool PseudowordGenerator::add_dictionary_word(const std::string& word) {
    //Check whether the word is valid.
    if (word.size() == 0) {
        return false;
    }
    
    for (size_t i = 0; i < word.size(); ++i) {
        if (kNoColumnIndex == column_indexes_[word[i]]) {
            return false;
        }
    }
    
    //Add the word to the dictionary.
    dictionary_.insert(word);
    
    //Add the word to the matrix.
    preceding_chars_.set_word_start();
    
    for (size_t i = 0; i <= word.size(); ++i) {
        const int row_index = preceding_chars_.row_index();
        
        //Get the column of the sampling matrix element to increment.
        char current_char = 'x';
        int column_index = -1;
        bool is_end_of_word_char = false;
        
        if (i < word.size() - 1) {
            current_char = word[i];
            column_index = column_indexes_[current_char];
        
        } else if ((word.size() - 1) == i) {
            // End of word character.
            column_index = num_matrix_columns_ - 1;
            is_end_of_word_char = true;
        
        } else {
            // Last character in the word.
            current_char = word[i - 1];
            column_index = column_indexes_[current_char];
        }
        
        //Record the transition from the preceding caracter combo to the
        //next character in the sampling matrix.
        const int matrix_index = row_index * num_matrix_columns_ + column_index;
        sampling_matrix_[matrix_index]++;
        
        if (is_end_of_word_char) {
            preceding_chars_.set_next_char_end_of_word();
        
        } else {
            preceding_chars_.set_next_char(current_char);
        }
    }
    
    return true;
}

bool PseudowordGenerator::prepare_for_generation() {
    //TODO: add error checking for cases when no words were added.
    for (int row = 0; row < num_matrix_rows_; ++row) {
        const int row_offset = row * num_matrix_columns_;
        
        //Calculate the total transitions sampled in this row.
        double total_transitions = 0;
        for (int column = 0; column < num_matrix_columns_; ++column) {
            total_transitions += static_cast<double>(sampling_matrix_[row_offset + column]);
        }
        
        //Populate the corresponding row in the transition matrix.
        if (fabs(total_transitions) >= 0.5) {
            double cumulative_probability = 0;
            
            for (int column = 0; column < num_matrix_columns_; ++column) {
                const int index = row_offset + column;
                const double num_transitions = static_cast<double>(sampling_matrix_[index]);
                double transition_probability = num_transitions / total_transitions;
                cumulative_probability += transition_probability;
                transition_matrix_[index] = cumulative_probability;
            }
            
        } else {
            for (int column = 0; column < num_matrix_columns_; ++column) {
                //The preceding combination for this row never occured.
                transition_matrix_[row_offset + column] = 0;
            }
        }
    }

    return true;
}

std::string PseudowordGenerator::make_word() const {
    //Use the transition matrix to generate the word.
    //If the produced word is actually a dictionary word, try again.
    std::string word;
    
    do {
        word = "";
        bool is_at_last_character = false;
        bool has_word_ended = false;
        preceding_chars_.set_word_start();
        
        do {
            const int row_offset = preceding_chars_.row_index() * num_matrix_columns_;
            const double p = random_01_();
            
            //Find which letter this corresponds to.
            int column = 0;
            while (p > transition_matrix_[row_offset + column]) {
                column++;
            }
            
            if (column != (num_matrix_columns_ - 1)) {
                const char ch = alphabet_[column];
                word += ch;
                
                if (is_at_last_character) {
                    has_word_ended = true;
                
                } else {
                    preceding_chars_.set_next_char(ch);
                }
            
            } else {
                is_at_last_character = true;
                preceding_chars_.set_next_char_end_of_word();
            }
            
        } while (!has_word_ended);
    } while (this->is_dictionary_word(word));
    
    return word;
}

std::string PseudowordGenerator::make_word(const boost::regex& criteria) const {
    //Use the transition matrix to generate the word.
    //If the produced word is actually a dictionary word, try again.
    std::string word;
    do {
        word = this->make_word();
    } while (!boost::regex_match(word, criteria));
    
    return word;
}



bool PseudowordGenerator::set_sampling_matrix(const std::vector<int>& matrix) {
    sampling_matrix_ = matrix;
    return true;
}

//bool PseudowordGenerator::set_transition_matrix(const std::vector<double>& matrix) {
    //TODO: implement
//    return false;
//}

bool PseudowordGenerator::is_dictionary_word(const std::string& word) const {
    Dictionary::const_iterator it = dictionary_.find(word);
    return (it != dictionary_.end());
}

/*---------------------------------------------------------
                    PrecedingChars class.
----------------------------------------------------------*/
// This class assumes that all error checking occures elsewhere.
/**
 * Initialize with the number of characters to use.
 */
PrecedingChars::PrecedingChars(size_t num_chars, const std::string& alphabet)
: num_chars_(num_chars),
alphabet_(alphabet),
chars_() {
    num_matrix_columns_ = static_cast<int>(alphabet_.size() + 1);
    num_matrix_rows_ = num_matrix_columns_ * num_matrix_columns_;
    
    //Initialize the starting characters.
    for (size_t i = 0; i < num_chars_; ++i) {
        chars_ += "0^";
    }
    
    //Initialize the map of character sequences to indexes.
    row_index_map_.set_empty_key(std::string(""));
    row_index_map_.resize(static_cast<size_t>(num_matrix_rows_));
    int row = 0;
    std::string char_combo("0^0^");
    int first_letter_alphabet_index = -1;
    int second_letter_alphabet_index = -1;
    
    const int alphabet_size = static_cast<int>(alphabet.size());

    while (row < num_matrix_rows_) {
        //Recalculate alphabetic index of the first and second letters to be hashed.
        if (row != 0) {
            second_letter_alphabet_index++;
            
            if (-1 == first_letter_alphabet_index && alphabet_size == second_letter_alphabet_index) {
                second_letter_alphabet_index = 0;
            } else if (second_letter_alphabet_index > alphabet_size) {
                second_letter_alphabet_index = 0;
            }
        }
        
        if ((row % num_matrix_columns_ == 0) && row != 0) {
            first_letter_alphabet_index++;
        }
        
        //Update the character combination to be hashed.
        if (first_letter_alphabet_index != -1) {
            char_combo[0] = alphabet_[first_letter_alphabet_index];
            char_combo[1] = '0';
        }
        
        if (second_letter_alphabet_index != -1) {
            if (second_letter_alphabet_index < alphabet_size) {
                char_combo[2] = alphabet_[second_letter_alphabet_index];
                char_combo[3] = '0';
            } else {
                char_combo[2] = '0';
                char_combo[3] = '$';
            }
        }
        
        row_index_map_[char_combo] = row;
        row++;
    }
}

/// Set the character sequence to represent the begining of the word.
void PrecedingChars::set_word_start() {
    for (size_t i = 0; i < num_chars_; ++i) {
        chars_[i * 2] = '0';
        chars_[i * 2 + 1] = '^';
    }
}

/// Add the next character to the sequence, removing the oldest
/// character from the back.
void PrecedingChars::set_next_char(char ch) {
    //Move all characters back by one letter and add the new character at the end.
    size_t chars_length = chars_.size();
    
    for (size_t i = 2; i < chars_length; ++i) {
        chars_[i - 2] = chars_[i];
    }
    
    chars_[chars_length - 2] = ch;
    chars_[chars_length - 1] = '0';
}

/// Add a special character to the front of the sequence, removing another character
/// from the back of the sequence.
void PrecedingChars::set_next_char_end_of_word() {
    //Move all characters back by one letter and add the new character at the end.
    size_t chars_length = chars_.size();
    
    for (size_t i = 2; i < chars_length; ++i) {
        chars_[i - 2] = chars_[i];
    }

    chars_[chars_length - 2] = '0';
    chars_[chars_length - 1] = '$';
}

/// Get the transition matrix row index corresponding to the character sequence.
int PrecedingChars::row_index() const {
    //Look up the index for the current preceding character combo.
    //If there is no such combo, return -1.
    RowIndexMap::const_iterator it = row_index_map_.find(chars_);
    
    if (row_index_map_.end() == it) {
        return -1;
    }
    
    return (it->second);
}

    


} /* namespace makewords */