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

#ifndef ISAWORD_WORD_PICKER_H
#define ISAWORD_WORD_PICKER_H

#include <ctime>
#include <string>
#include <vector>
#include <utility>

#include <boost/random.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_01.hpp>
#include <boost/random/variate_generator.hpp>
#include <boost/regex.hpp>

#include "generator/pseudoword_generator.h"

namespace isaword {

typedef std::pair<std::string, std::string> WordDefinition;

/*---------------------------------------------------------
                    WordDescription class.
----------------------------------------------------------*/
/**
 * A simple structure containing description of a word.
 */
class WordDescription {
public:
    std::string word;
    std::string description;
    bool is_real;
};

typedef boost::shared_ptr<WordDescription> WordDescriptionPtr;
class WordIndexDescription;

/*---------------------------------------------------------
                    WordPicker class.
----------------------------------------------------------*/
/**
 * The class responsible for picking the combinations of real and
 * fake words.
 */
class WordPicker {
public:
    // Word index types.
    static const size_t ALL_VOWELS          = 0;
    static const size_t ALL_CONSONANTS      = 1;
    static const size_t ALL_VOWELS_BUT_ONE  = 2;
    static const size_t Q_WORDS             = 3;
    static const size_t Z_WORDS             = 4;
    static const size_t X_WORDS             = 5;
    static const size_t J_WORDS             = 6;

    WordPicker(const std::vector<boost::shared_ptr<WordIndexDescription> >& index_descriptions)
    : index_descriptions_(index_descriptions),
      pseudoword_generator_(new makewords::PseudowordGenerator("ABCDEFGHIJKLMNOPQRSTUVWXYZ")),
      random_numbers_generator_(time(0)),
      uniform_01_(),
      random_01_(random_numbers_generator_, uniform_01_) {
    }
    
    /**
     * Ininitalize the word picker by providing it a path
     * to a dictionary to work with.
     *
     * @return true if the initialization was successfull, 
     * false otherwise.
     */
    bool initialize(const std::string& dictionary_path);
    
    /**
     * Pick a number of words by length.
     */
    std::vector<WordDescriptionPtr> get_words_by_length(size_t from, 
                                                        size_t to, 
                                                        size_t num_words);
    
    /**
     * Pick a number of words satisfying a certain criteria.
     */
    std::vector<WordDescriptionPtr> get_words_from_index(size_t index, size_t num_words);
    
private:
    /// Main list of words by length.
    std::vector<WordDescriptionPtr> words_by_length_;
    
    /// Index of where the words of specific length start.
    std::vector<size_t> word_length_ends_;
    
    /// Descriptions of the indexes to be tracked.
    std::vector<boost::shared_ptr<WordIndexDescription> > index_descriptions_;
    
    /// Other word indexes.
    std::vector<std::vector<WordDescriptionPtr> > indexes_;
    
    /// Pseudoword generator.
    boost::shared_ptr<makewords::PseudowordGenerator> pseudoword_generator_;
    
    /// Random numbers generator.
    mutable boost::mt19937 random_numbers_generator_;
    
    /// Uniform [0, 1] distribution.
    mutable boost::uniform_01<> uniform_01_;
    
    /// The main generator.
    mutable boost::variate_generator<boost::mt19937&, boost::uniform_01<> > random_01_;
};

/*---------------------------------------------------------
                    WordIndexDescription class.
----------------------------------------------------------*/
class WordIndexDescription {
public:
    /// Create a WordIndexDescription object.
    WordIndexDescription(const std::string& name, 
                         const std::string& description,
                         const std::string& pattern)
    : name_(name),
      description_(description),
      pattern_(pattern) {
    }
    
    /// Check whether the word satisfies the criteria to be indexed
    /// by this index.
    bool should_be_indexed(const std::string& word) {return regex_match(word, pattern_);}
    
    /*================== Getters/setters =====================*/
    /// Get the index name.
    std::string name() const            {return name_;}
    
    /// Get the index description.
    std::string description() const     {return description_;}
    
private:
    /// Name/id of the index (e.g. "q_words").
    std::string name_;
    
    /// A short description of the index ("Q words").
    std::string description_;
    
    /// Regex pattern which all members of index must satisfy.
    boost::regex pattern_;
};

} /* namespace isaword */

#endif
