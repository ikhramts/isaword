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

#ifndef MAKEWORDS_PSEUDOWORD_GENERATOR_H
#define MAKEWORDS_PSEUDOWORD_GENERATOR_H
 
// Definition of the Markov Chain pseudoword generator.
// 
#include <bitset>
#include <string>
#include <vector>
#include <boost/functional/hash.hpp>
#include <boost/random.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_01.hpp>
#include <boost/random/variate_generator.hpp>
#include <boost/regex.hpp>
#include <google/sparse_hash_set>
#include <google/dense_hash_map>
#include "utils.h"

namespace makewords {

/**
 * A functor used to compare the strings in the hash set.
 */
struct eqstr {
    bool operator()(const std::string& first, const std::string& second) const {
        return (first == second);
    }
};

/**
 * This class represents a fixed-length sequence of characters representing
 * several consecutive letters of a word, including possibly special characters
 * indicating beginning or ending of a word.
 */
class PrecedingChars {
public:
    typedef google::dense_hash_map<std::string, int, boost::hash<std::string>, eqstr> RowIndexMap;
    
    /// A character representing the beginning of a word.
    static const char kWordStartChar = '^';
    
    /// Special character indicating that the last letter in the word is next.
    static const char kLastCharChar = '$';
    
    /**
     * Initialize with the number of characters to use.
     */
    PrecedingChars(size_t num_chars, const std::string& alphabet);
    
    /// Set the character sequence to represent the begining of the word.
    void set_word_start();
    
    /// Add the next character to the sequence, removing the oldest
    /// character from the back.
    void set_next_char(char ch);
    
    /// Add a special character to the front of the sequence, removing another character
    /// from the back of the sequence.
    void set_next_char_end_of_word();
    
    /// Get the transition matrix row index corresponding to the character sequence.
    int row_index() const;
    
    /**
     * Get the stored sequence of characters.
     * The sequence will actually be num_chars * 2 characters long, with even
     * characters (0, 2, ...) representing alphabet characters, and odd characters
     * (1, 3, ...) representing special characters indicating end of a word.
     * If an odd character is present at position (2n + 1) then the character at
     * position 2n should be ignored as it is not releant.
     */
    std::string chars() const            {return chars_;}
    
    /// Get the number of columns in the transition matrix.
    int num_matrix_columns() const      {return num_matrix_columns_;}
    
    /// Get the number of rows in the transition matrix
    int num_matrix_rows() const         {return num_matrix_rows_;}
    
    /// Get the number of characters to track
    size_t num_chars() const            {return num_chars_;}
    
    ///Get the alphabet
    std::string alphabet() const        {return alphabet_;}
    
private:
    int num_matrix_columns_;
    int num_matrix_rows_;
    size_t num_chars_;
    std::string alphabet_;
    std::string chars_;
    
    RowIndexMap row_index_map_;
};

/**
 * This class is responsible for generating the pseudowords.
 */
class PseudowordGenerator {
private:
    typedef google::sparse_hash_set<std::string, boost::hash<std::string>, eqstr> Dictionary;

public:
    static const int kDefaultNumCondidiontingCharacters = 2;
    static const size_t kExpectedDictionarySize = 200000;
    static const size_t kAlphabetSpaceSize = 256;
    static const int kNoColumnIndex = -1;
    
    /*========= Main logic =======*/
    /** 
     * Create the generator.  The generator will attempt to create the
     * matrix of probabilities of getting a letter in a word given the 
     * preceding chunk_length letters.  Only letters from the alphabet
     * supplied in the first argument will be permitted; adding
     * dictionary words with letters outside the permitted alphabet will
     * return an error.
     *
     * The alphabet may not contain characters '$' and '^'; these will be 
     */ 
    PseudowordGenerator(const std::string& alphabet);
    
    /**
     * Initialize the generator.  This step may fail as it may require
     * allocating a lot of space for the expected dictionary.  Will return
     * true on success, false on failure.
     */
    bool initialize(size_t expected_dictionary_size = kExpectedDictionarySize);
    
    /**
     * Add a dictionary word to the generator to train it.  Returns true
     * on success, false on failure.  Use error_message() to get the
     * error message associated with the latest word.
     */
    bool add_dictionary_word(const std::string& word);
    
    /**
     * Get the error message.
     */
    std::string error_message() const       {return error_message_;}
    
    /**
     * Prepare the generator for pseudoword generation.  Invoke this when
     * you're done adding dictionary words, and want to start generating
     * pseudowords.  Invoking this updates the transition_matrix_.
     * Will return true if succeeded, false if no dictionary words have been
     * provided.
     */
    bool prepare_for_generation();
    
    /**
     * Generate a pseudoword.  The pseudoword will be checked against
     * existing dictionary words to ensure that it is not a dictionary
     * word.
     */
    std::string make_word() const;
    
    /**
     * Generate a pseudoword satisfying specific criteria.  The pseudoword 
     * will be checked against existing dictionary words to ensure that it 
     * is not a dictionary word.
     */
    std::string make_word(const boost::regex& criteria) const;
    
    /*========= Getters/setters =======*/
    
    ///Get the alphabet.
    std::string alphabet() const        {return alphabet_;}
    
    ///Get the number of conditioning characters.
    short num_conditioning_characters() const   {return num_conditioning_characters_;}
    
    ///Get the number of matrix rows.
    int num_matrix_rows() const     {return num_matrix_rows_;}
    
    ///Get the numner of matrix columns.
    int num_matrix_columns() const  {return num_matrix_columns_;}
    
    ///Set the sampling matrix.
    ///No input checking is done.
    bool set_sampling_matrix(const std::vector<int>& matrix);
    
    ///Get the sampling matrix.
    std::vector<int> sampling_matrix() const    {return sampling_matrix_;}
    
    ///Set the transition matrix.  Succeeds only if the matrix has
    ///num_matrix_rows() rows and alphabet_size() columns,
    ///all its entries are positive, and each row adds up to 1.
    ///Returns true if successfull; false if fails.
    //bool set_transition_matrix(const std::vector<double>& matrix);
    
    ///Get the cumulative transition matrix.
    std::vector<double> transition_matrix() const   {return transition_matrix_;}
    
    ///Get the column indexes of the letters.
    ///Letters with no column index should have index of kNoColumnIndex.
    std::vector<int> column_indexes() const         {return column_indexes_;}
    
    /*========= Misc stuff =======*/
    
    ///Check whether a word is in the dictionary.
    bool is_dictionary_word(const std::string& word) const;
    
private:
    /// The error message.
    std::string error_message_;
    
    /// List of valid word letters.
    std::string alphabet_;
    
    /// Number of preceding characters on which the following character will
    /// depend.
    int num_conditioning_characters_;
    
    /// Number of columns in the transition matrix (number of possible next characters).
    int num_matrix_columns_;
    
    /// Number of rows in the transition matrix (the size of the state space).
    int num_matrix_rows_;
    
    /// Matrix where the transitions will be counted.
    std::vector<int> sampling_matrix_;
    
    /// Cumulative transition matrix for the markov chains for the language.
    /// Can be updated by invoking prepare_for_generation().
    std::vector<double> transition_matrix_;
    
    /// Storage place for all valid dictionary words.
    Dictionary dictionary_; 
    
    /// An internal helper to keep track of preceding characters.
    mutable PrecedingChars preceding_chars_;
    
    /// Column index of each letter.
    std::vector<int> column_indexes_;
    
    /// Random numbers generator.
    mutable boost::mt19937 random_numbers_generator_;
    
    /// Uniform [0, 1] distribution.
    mutable boost::uniform_01<> uniform_01_;
    
    /// The main generator.
    mutable boost::variate_generator<boost::mt19937&, boost::uniform_01<> > random_01_;
};

}; /* namespace makewords */

#endif
