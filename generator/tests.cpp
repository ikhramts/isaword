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

// Test module for PseudowordGenerator.
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE PseudowordGenerator

#include <sstream>
#include <vector>
#include <string>
#include <boost/test/unit_test.hpp>
#include "pseudoword_generator.h"
#include "utils.h"

using namespace makewords;

/*========= Useful functions/data. ===================*/
const int kAlphabetSize = 26;
const int kExpectedRows = 729;
const int kExpectedColumns = 27;
const size_t kuAlphabetSize = 26;
 
std::string make_alphabet() {
    std::string alphabet("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    return alphabet;
}

const int kSmallAlphabetSize = 4;
const int kExpectedSmallRows = 25;
const int kExpectedSmallColumns = 5;

std::string make_small_alphabet() {
    std::string small_alphabet("ABDE");
    return small_alphabet;
}

/*---------------------------------------------------------
                    PrecedingChars tests.
----------------------------------------------------------*/
/*========= Fixtures. ===================*/
/*========= Constructor tests. ===================*/
BOOST_AUTO_TEST_SUITE(PrecedingChars_constructor_tests)

BOOST_AUTO_TEST_CASE(constructor_normal_alphabet) {
    PrecedingChars preceding_chars(2, make_alphabet());
    
    std::string expected_chars("0^0^");
    std::string chars(preceding_chars.chars());

    //Check that the internal state has been correctly initialized.
    BOOST_CHECK_EQUAL(expected_chars, chars);
    BOOST_CHECK_EQUAL(preceding_chars.num_matrix_rows(), kExpectedRows);
    BOOST_CHECK_EQUAL(preceding_chars.num_matrix_columns(), kExpectedColumns);
    BOOST_CHECK_EQUAL(preceding_chars.num_chars(), 2);
    BOOST_CHECK_EQUAL(preceding_chars.alphabet(), make_alphabet());
    
}

BOOST_AUTO_TEST_CASE(constructor_small_alphabet) {
    PrecedingChars preceding_chars(2, make_small_alphabet());

    std::string expected_chars("0^0^");
    std::string chars(preceding_chars.chars());

    //Check that the internal state has been correctly initialized.
    BOOST_CHECK_EQUAL(expected_chars, chars);
    BOOST_CHECK_EQUAL(preceding_chars.num_matrix_rows(), kExpectedSmallRows);
    BOOST_CHECK_EQUAL(preceding_chars.num_matrix_columns(), kExpectedSmallColumns);
    BOOST_CHECK_EQUAL(preceding_chars.num_chars(), 2);
    BOOST_CHECK_EQUAL(preceding_chars.alphabet(), make_small_alphabet());
}

BOOST_AUTO_TEST_SUITE_END()

/*========= Character manipulation tests. ===================*/
class PrecedingCharsCharacterManipulationFixture {
public:
    PrecedingCharsCharacterManipulationFixture()
    : preceding_chars(2, make_alphabet()) {
        test_chars = "JXE";
        expected_internal_states.push_back("0^J0");
        expected_internal_states.push_back("J0X0");
        expected_internal_states.push_back("X0E0");
        expected_end_of_word_state = "E00$";
    }
    
    ~PrecedingCharsCharacterManipulationFixture() {
    }
    
    PrecedingChars preceding_chars;
    std::string test_chars;
    std::vector<std::string> expected_internal_states;
    std::string expected_end_of_word_state;
};

BOOST_FIXTURE_TEST_SUITE(PrecedingChars_manipulation_tests, PrecedingCharsCharacterManipulationFixture)

BOOST_AUTO_TEST_CASE(set_next_char) {
    BOOST_CHECK_EQUAL(preceding_chars.chars(), "0^0^");
    
    for (int i = 0; i < 3; ++i) {
        preceding_chars.set_next_char(test_chars[i]);
        BOOST_CHECK_EQUAL(preceding_chars.chars(), expected_internal_states[i]);
    }
}

BOOST_AUTO_TEST_CASE(set_next_char_end_of_word) {
    for (int i = 0; i < 3; ++i) {
        preceding_chars.set_next_char(test_chars[i]);
    }
    
    preceding_chars.set_next_char_end_of_word();
    
    BOOST_CHECK_EQUAL(preceding_chars.chars(), expected_end_of_word_state);
}

BOOST_AUTO_TEST_CASE(set_word_start) {
    for (int i = 0; i < 3; ++i) {
        preceding_chars.set_next_char(test_chars[i]);
    }
    
    preceding_chars.set_word_start();
    
    BOOST_CHECK_EQUAL(preceding_chars.chars(), "0^0^");
}

BOOST_AUTO_TEST_SUITE_END()

/*========= Matrix index calculation tests. ===================*/
class PrecedingCharsIndexCalculationFixture {
public:
    PrecedingCharsIndexCalculationFixture()
    : preceding_chars(2, make_small_alphabet()) {
        combinations.push_back("^^");
        combinations.push_back("^A");
        combinations.push_back("^B");
        combinations.push_back("^D");
        combinations.push_back("^E");
        combinations.push_back("AA");
        combinations.push_back("AB");
        combinations.push_back("AD");
        combinations.push_back("AE");
        combinations.push_back("A$");
        combinations.push_back("BA");
        combinations.push_back("BB");
        combinations.push_back("BD");
        combinations.push_back("BE");
        combinations.push_back("B$");
        combinations.push_back("DA");
        combinations.push_back("DB");
        combinations.push_back("DD");
        combinations.push_back("DE");
        combinations.push_back("D$");
        combinations.push_back("EA");
        combinations.push_back("EB");
        combinations.push_back("ED");
        combinations.push_back("EE");
        combinations.push_back("E$");
    }
    
    ~PrecedingCharsIndexCalculationFixture() {
    }
    
    PrecedingChars preceding_chars;
    static const int kNumCombinations = 25;
    std::vector<std::string> combinations;
};

BOOST_FIXTURE_TEST_SUITE(PrecedingChars_index_calculation, PrecedingCharsIndexCalculationFixture)

BOOST_AUTO_TEST_CASE(index_calculations) {
    for (int i = 0; i < kNumCombinations; ++i) {
        //Set up the test.
        preceding_chars.set_word_start();
        
        if (combinations[i][0] != '^') {
            preceding_chars.set_next_char(combinations[i][0]);
        }
        
        if (combinations[i][1] == '$') {
            preceding_chars.set_next_char_end_of_word();
        
        } else if (combinations[i][1] != '^') {
            preceding_chars.set_next_char(combinations[i][1]);
        }
        
        //Compare the produced index with the expected index.
        if (preceding_chars.row_index() != i) {
            std::stringstream message;
            message << "Error at combination " << i << ": expected index "
                    << i << " got " << preceding_chars.row_index();
            BOOST_ERROR(message.str());
            break;
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()

/*---------------------------------------------------------
                PseudowordGenerator tests.
----------------------------------------------------------*/
/*========= Fixtures. ===================*/
class BasicFixture {
public:
    BasicFixture()
    : generator(make_alphabet()) {
        generator.initialize();
    }
    
    ~BasicFixture() {
    }
    
    PseudowordGenerator generator;
};

class SmallAlphabetWithWordsFixture {
public:
    SmallAlphabetWithWordsFixture()
    : generator(make_small_alphabet()) {
        generator.initialize();
        
        valid_words.push_back(std::string("AE"));
        valid_words.push_back(std::string("BADE"));
        valid_words.push_back(std::string("BEAB"));

        invalid_words.push_back(std::string(""));
        invalid_words.push_back(std::string("OPERA"));
        invalid_words.push_back(std::string("TIC-TAC"));
    }
    
    ~SmallAlphabetWithWordsFixture() {
    }
    
    PseudowordGenerator generator;
    std::vector<std::string> valid_words;
    std::vector<std::string> invalid_words;
    
};

/*========= Constructor tests. ===================*/
BOOST_AUTO_TEST_SUITE(PseudowordGenerator_constructor_tests)

BOOST_AUTO_TEST_CASE(constructor_normal_alphabet) {
    PseudowordGenerator generator(make_alphabet());
    
    //General checks.
    std::string empty_string;
    BOOST_CHECK_EQUAL(generator.error_message().compare(empty_string), 0);
    BOOST_CHECK_EQUAL(generator.num_conditioning_characters(), 
        PseudowordGenerator::kDefaultNumCondidiontingCharacters);
        
    //Alphabet checks
    BOOST_CHECK_EQUAL(generator.alphabet().compare(make_alphabet()), 0);
    std::vector<int> column_indexes(generator.column_indexes());
    
    for (short ch = 0; ch < 256; ++ch) {
        if (ch < 'A' || ch > 'Z') {
            if (PseudowordGenerator::kNoColumnIndex != column_indexes[ch]) {
                std::string message;
                message += "Letter '";
                message += ch;
                message += "' was incorrectly labeled as valid.";
                BOOST_ERROR(message);
                break;
            }
        
        } else {
            if(PseudowordGenerator::kNoColumnIndex == column_indexes[ch]) {
                std::string message;
                message += "Letter '";
                message += ch;
                message += "' was incorrectly labeled as invalid.";
                BOOST_ERROR(message);
                break;
            }
        }
    }
    
    //Checks on sampling and transition matrices.
    BOOST_CHECK_EQUAL(generator.num_matrix_rows(), kExpectedRows);
    BOOST_CHECK_EQUAL(generator.num_matrix_columns(), kExpectedColumns);
    
    std::vector<int> sampling_matrix(generator.sampling_matrix());
    std::vector<double> transition_matrix(generator.transition_matrix());
    
    const uint expected_size = static_cast<uint>(kExpectedRows * kExpectedColumns);
    BOOST_CHECK_EQUAL(sampling_matrix.size(), expected_size);
    BOOST_CHECK_EQUAL(transition_matrix.size(), expected_size);
    
    for (uint i = 0; i < sampling_matrix.size(); ++i) {
        BOOST_CHECK_EQUAL(sampling_matrix[i], 0);
        BOOST_CHECK_EQUAL(transition_matrix[i], 0);
    }
}

BOOST_AUTO_TEST_CASE(constructor_small_alphabet) {
    PseudowordGenerator generator(make_small_alphabet());
    
    //Alphabet checks.
    BOOST_CHECK_EQUAL(generator.alphabet().compare(make_small_alphabet()), 0);
    std::vector<int> column_indexes(generator.column_indexes());
    
    for (short ch = 0; ch < 256; ++ch) {
        if (ch < 'A' || ch > 'E' || ch == 'C') {
            if (PseudowordGenerator::kNoColumnIndex != column_indexes[ch]) {
                std::string message;
                message += "Letter '";
                message += ch;
                message += "' was incorrectly labeled as valid.";
                BOOST_ERROR(message);
                break;
            }
            
        } else {
            if(PseudowordGenerator::kNoColumnIndex == column_indexes[ch]) {
                std::string message;
                message += "Letter '";
                message += ch;
                message += "' was incorrectly labeled as invalid.";
                BOOST_ERROR(message);
                break;
            }
        }
    }
    
    //Checks on sampling and transition matrices.
    BOOST_CHECK_EQUAL(generator.num_matrix_rows(), kExpectedSmallRows);
    BOOST_CHECK_EQUAL(generator.num_matrix_columns(), kExpectedSmallColumns);
    
    std::vector<int> sampling_matrix(generator.sampling_matrix());
    std::vector<double> transition_matrix(generator.transition_matrix());
    
    const uint expected_size = static_cast<uint>(kExpectedSmallRows * kExpectedSmallColumns);
    BOOST_CHECK_EQUAL(sampling_matrix.size(), expected_size);
    BOOST_CHECK_EQUAL(transition_matrix.size(), expected_size);
    
    for (uint i = 0; i < sampling_matrix.size(); ++i) {
        BOOST_CHECK_EQUAL(sampling_matrix[i], 0);
        BOOST_CHECK_EQUAL(transition_matrix[i], 0);
    }
}

BOOST_AUTO_TEST_SUITE_END()

/*========= Initialization tests. ===================*/
BOOST_AUTO_TEST_SUITE(PseudowordGenerator_initializer_tests)

BOOST_AUTO_TEST_CASE(initialize_small_dictionary) {
    PseudowordGenerator generator(make_alphabet());
    BOOST_CHECK(generator.initialize(10));
}

BOOST_AUTO_TEST_SUITE_END()

/*========= Check word validation mechanism. ===================*/
BOOST_FIXTURE_TEST_SUITE(PseudowordGenerator_alphabet_validation_tests, BasicFixture)

BOOST_AUTO_TEST_CASE(add_valid_word1) {
    std::string word("HELLO");
    BOOST_CHECK(generator.add_dictionary_word(word));
}

BOOST_AUTO_TEST_CASE(add_valid_word2) {
    std::string word("PLAGUILY");
    BOOST_CHECK(generator.add_dictionary_word(word));
}

BOOST_AUTO_TEST_CASE(add_valid_word3) {
    std::string word("PREDETERMINE");
    BOOST_CHECK(generator.add_dictionary_word(word));
}

BOOST_AUTO_TEST_CASE(add_invalid_empty_word) {
    std::string word("");
    BOOST_CHECK(!generator.add_dictionary_word(word));
}

BOOST_AUTO_TEST_CASE(add_invalid_word1) {
    std::string word("O'SHANTER");
    BOOST_CHECK(!generator.add_dictionary_word(word));
}

BOOST_AUTO_TEST_CASE(add_invalid_word2) {
    std::string word("TET-A-TET");
    BOOST_CHECK(!generator.add_dictionary_word(word));
}

BOOST_AUTO_TEST_SUITE_END()

/*========= Check the dictionary. ===================*/
BOOST_FIXTURE_TEST_SUITE(PseudowordGenerator_dictionary_tests, BasicFixture)

BOOST_AUTO_TEST_CASE(add_valid_word1) {
    std::string word("HELLO");
    BOOST_CHECK(generator.add_dictionary_word(word));
    BOOST_CHECK(generator.is_dictionary_word(word));
}

BOOST_AUTO_TEST_CASE(add_valid_word2) {
    std::string word("PLAGUILY");
    BOOST_CHECK(generator.add_dictionary_word(word));
    BOOST_CHECK(generator.is_dictionary_word(word));
}

BOOST_AUTO_TEST_CASE(add_valid_word3) {
    std::string word("PREDETERMINE");
    BOOST_CHECK(generator.add_dictionary_word(word));
    BOOST_CHECK(generator.is_dictionary_word(word));
}

BOOST_AUTO_TEST_CASE(add_invalid_empty_word) {
    std::string word("");
    BOOST_CHECK(!generator.add_dictionary_word(word));
    BOOST_CHECK(!generator.is_dictionary_word(word));
}

BOOST_AUTO_TEST_CASE(add_invalid_word1) {
    std::string word("O'SHANTER");
    BOOST_CHECK(!generator.add_dictionary_word(word));
    BOOST_CHECK(!generator.is_dictionary_word(word));
}

BOOST_AUTO_TEST_CASE(add_invalid_word2) {
    std::string word("TET-A-TET");
    BOOST_CHECK(!generator.add_dictionary_word(word));
    BOOST_CHECK(!generator.is_dictionary_word(word));
}

BOOST_AUTO_TEST_SUITE_END()

/*========= Check the sampling matrix composition. ===================*/
BOOST_FIXTURE_TEST_SUITE(PseudowordGenerator_sampling_matrix_tests, 
                         SmallAlphabetWithWordsFixture)

BOOST_AUTO_TEST_CASE(add_word1) {
    generator.add_dictionary_word(valid_words[0]);
    std::vector<int> sampling_matrix(generator.sampling_matrix());
    
    for (size_t i = 0; i < sampling_matrix.size(); ++i) {
        int should_be = 0;
        
        if (i == 0 || i == 9 || i == 48) {
            should_be = 1;
        }
        
        if (sampling_matrix[i] != should_be) {
            std::stringstream message;
            message << "Value at " << i << " should be 1, is " << sampling_matrix[i];
            BOOST_ERROR(message.str());
            break;
        }
    }
}

BOOST_AUTO_TEST_CASE(add_word2) {
    generator.add_dictionary_word(valid_words[1]);
    std::vector<int> sampling_matrix(generator.sampling_matrix());
    
    for (size_t i = 0; i < sampling_matrix.size(); ++i) {
        int should_be = 0;
        
        if (i == 1 || i == 10 || i == 39 || i == 52 || i == 98) {
            should_be = 1;
        }
        
        if (sampling_matrix[i] != should_be) {
            std::stringstream message;
            message << "Value at " << i << " should be 1, is " << sampling_matrix[i];
            BOOST_ERROR(message.str());
            break;
        }
    }
}

BOOST_AUTO_TEST_CASE(add_word3) {
    generator.add_dictionary_word(valid_words[2]);
    std::vector<int> sampling_matrix(generator.sampling_matrix());
    
    for (size_t i = 0; i < sampling_matrix.size(); ++i) {
        int should_be = 0;
        
        if (i == 1 || i == 13 || i == 46 || i == 65 || i == 104) {
            should_be = 1;
        }
        
        if (sampling_matrix[i] != should_be) {
            std::stringstream message;
            message << "Value at " << i << " should be 1, is " << sampling_matrix[i];
            BOOST_ERROR(message.str());
            break;
        }
    }
}

BOOST_AUTO_TEST_CASE(add_invalid_word1) {
    generator.add_dictionary_word(invalid_words[0]);
    std::vector<int> sampling_matrix(generator.sampling_matrix());
    
    for (size_t i = 0; i < sampling_matrix.size(); ++i) {
        int should_be = 0;

        if (sampling_matrix[i] != should_be) {
            std::stringstream message;
            message << "Value at " << i << " should be 1, is " << sampling_matrix[i];
            BOOST_ERROR(message.str());
            break;
        }
    }
}

BOOST_AUTO_TEST_CASE(add_invalid_word2) {
    generator.add_dictionary_word(invalid_words[1]);
    std::vector<int> sampling_matrix(generator.sampling_matrix());
    
    for (size_t i = 0; i < sampling_matrix.size(); ++i) {
        int should_be = 0;

        if (sampling_matrix[i] != should_be) {
            std::stringstream message;
            message << "Value at " << i << " should be 1, is " << sampling_matrix[i];
            BOOST_ERROR(message.str());
            break;
        }
    }
}

BOOST_AUTO_TEST_CASE(add_invalid_word3) {
    generator.add_dictionary_word(invalid_words[2]);
    std::vector<int> sampling_matrix(generator.sampling_matrix());
    
    for (size_t i = 0; i < sampling_matrix.size(); ++i) {
        int should_be = 0;

        if (sampling_matrix[i] != should_be) {
            std::stringstream message;
            message << "Value at " << i << " should be 1, is " << sampling_matrix[i];
            BOOST_ERROR(message.str());
            break;
        }
    }
}

BOOST_AUTO_TEST_CASE(add_all_words) {
    generator.add_dictionary_word(valid_words[0]);
    generator.add_dictionary_word(valid_words[1]);
    generator.add_dictionary_word(valid_words[2]);
    std::vector<int> sampling_matrix(generator.sampling_matrix());
    
    for (size_t i = 0; i < sampling_matrix.size(); ++i) {
        int should_be = 0;
        
        if (i == 0 || i == 9 || i == 10 || i == 13 || i == 39 ||
          i == 46 || i == 48 || i == 52 || i == 65 || i == 98|| 
          i == 104) {
            should_be = 1;
            
        } else if (i == 1) {
            should_be = 2;
        }
        
        if (sampling_matrix[i] != should_be) {
            std::stringstream message;
            message << "Value at " << i << " should be " << should_be 
                    << ", is " << sampling_matrix[i];
            BOOST_ERROR(message.str());
            break;
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()

/*========= Check the transition matrix creation. ===================*/
class TransitionMatrixFixture {
public:
    TransitionMatrixFixture()
    : generator(make_small_alphabet()) {
        generator.initialize();
        
        //A sampling matrix for testing.
        std::vector<int> sm;
        sm.push_back(94); sm.push_back(67); sm.push_back(45); sm.push_back(12); sm.push_back(23); 
        sm.push_back(49); sm.push_back(53); sm.push_back(18); sm.push_back(89); sm.push_back(69); 
        sm.push_back(55); sm.push_back(77); sm.push_back(82); sm.push_back(35); sm.push_back(36); 
        sm.push_back(15); sm.push_back(31); sm.push_back(38); sm.push_back(8); sm.push_back(82); 
        sm.push_back(40); sm.push_back(79); sm.push_back(15); sm.push_back(81); sm.push_back(24); 
        sm.push_back(9); sm.push_back(60); sm.push_back(74); sm.push_back(36); sm.push_back(62); 
        sm.push_back(81); sm.push_back(7); sm.push_back(78); sm.push_back(23); sm.push_back(37); 
        sm.push_back(96); sm.push_back(55); sm.push_back(38); sm.push_back(58); sm.push_back(22); 
        sm.push_back(47); sm.push_back(44); sm.push_back(4); sm.push_back(1); sm.push_back(58); 
        sm.push_back(57); sm.push_back(76); sm.push_back(0); sm.push_back(94); sm.push_back(56); 
        sm.push_back(13); sm.push_back(43); sm.push_back(48); sm.push_back(98); sm.push_back(11); 
        sm.push_back(60); sm.push_back(89); sm.push_back(35); sm.push_back(54); sm.push_back(1); 
        sm.push_back(62); sm.push_back(5); sm.push_back(70); sm.push_back(83); sm.push_back(66); 
        sm.push_back(98); sm.push_back(74); sm.push_back(57); sm.push_back(54); sm.push_back(78); 
        sm.push_back(41); sm.push_back(79); sm.push_back(0); sm.push_back(6); sm.push_back(74); 
        sm.push_back(92); sm.push_back(13); sm.push_back(52); sm.push_back(92); sm.push_back(1); 
        sm.push_back(37); sm.push_back(90); sm.push_back(15); sm.push_back(98); sm.push_back(28); 
        sm.push_back(0); sm.push_back(0); sm.push_back(0); sm.push_back(0); sm.push_back(0); 
        sm.push_back(25); sm.push_back(69); sm.push_back(8); sm.push_back(33); sm.push_back(35); 
        sm.push_back(81); sm.push_back(66); sm.push_back(88); sm.push_back(34); sm.push_back(42); 
        sm.push_back(66); sm.push_back(30); sm.push_back(70); sm.push_back(65); sm.push_back(33); 
        sm.push_back(67); sm.push_back(78); sm.push_back(58); sm.push_back(16); sm.push_back(44); 
        sm.push_back(95); sm.push_back(99); sm.push_back(79); sm.push_back(73); sm.push_back(93); 
        sm.push_back(35); sm.push_back(99); sm.push_back(8); sm.push_back(68); sm.push_back(62); 
        sm.push_back(77); sm.push_back(27); sm.push_back(84); sm.push_back(76); sm.push_back(56); 
        sampling_matrix = sm;
        
        //Expected transition matrix.
        std::vector<double> tm;
        tm.push_back(0.390041493775934); tm.push_back(0.66804979253112); tm.push_back(0.854771784232365); tm.push_back(0.904564315352697); tm.push_back(1); 
        tm.push_back(0.176258992805755); tm.push_back(0.366906474820144); tm.push_back(0.431654676258993); tm.push_back(0.751798561151079); tm.push_back(1); 
        tm.push_back(0.192982456140351); tm.push_back(0.463157894736842); tm.push_back(0.750877192982456); tm.push_back(0.873684210526316); tm.push_back(1); 
        tm.push_back(0.0862068965517241); tm.push_back(0.264367816091954); tm.push_back(0.482758620689655); tm.push_back(0.528735632183908); tm.push_back(1); 
        tm.push_back(0.167364016736402); tm.push_back(0.497907949790795); tm.push_back(0.560669456066946); tm.push_back(0.899581589958159); tm.push_back(1); 
        tm.push_back(0.037344398340249); tm.push_back(0.286307053941909); tm.push_back(0.593360995850622); tm.push_back(0.742738589211618); tm.push_back(1); 
        tm.push_back(0.358407079646018); tm.push_back(0.389380530973451); tm.push_back(0.734513274336283); tm.push_back(0.836283185840708); tm.push_back(1); 
        tm.push_back(0.356877323420074); tm.push_back(0.561338289962825); tm.push_back(0.702602230483271); tm.push_back(0.9182156133829); tm.push_back(1); 
        tm.push_back(0.305194805194805); tm.push_back(0.590909090909091); tm.push_back(0.616883116883117); tm.push_back(0.623376623376623); tm.push_back(1); 
        tm.push_back(0.201413427561837); tm.push_back(0.469964664310954); tm.push_back(0.469964664310954); tm.push_back(0.802120141342756); tm.push_back(1); 
        tm.push_back(0.0610328638497653); tm.push_back(0.262910798122066); tm.push_back(0.488262910798122); tm.push_back(0.948356807511737); tm.push_back(1); 
        tm.push_back(0.251046025104602); tm.push_back(0.623430962343096); tm.push_back(0.769874476987448); tm.push_back(0.99581589958159); tm.push_back(1); 
        tm.push_back(0.216783216783217); tm.push_back(0.234265734265734); tm.push_back(0.479020979020979); tm.push_back(0.769230769230769); tm.push_back(1); 
        tm.push_back(0.271468144044321); tm.push_back(0.476454293628809); tm.push_back(0.634349030470914); tm.push_back(0.78393351800554); tm.push_back(1); 
        tm.push_back(0.205); tm.push_back(0.6); tm.push_back(0.6); tm.push_back(0.63); tm.push_back(1); 
        tm.push_back(0.368); tm.push_back(0.42); tm.push_back(0.628); tm.push_back(0.996); tm.push_back(1); 
        tm.push_back(0.138059701492537); tm.push_back(0.473880597014925); tm.push_back(0.529850746268657); tm.push_back(0.895522388059702); tm.push_back(1); 
        tm.push_back(0); tm.push_back(0); tm.push_back(0); tm.push_back(0); tm.push_back(0); 
        tm.push_back(0.147058823529412); tm.push_back(0.552941176470588); tm.push_back(0.6); tm.push_back(0.794117647058823); tm.push_back(1); 
        tm.push_back(0.260450160771704); tm.push_back(0.472668810289389); tm.push_back(0.755627009646302); tm.push_back(0.864951768488746); tm.push_back(1); 
        tm.push_back(0.25); tm.push_back(0.363636363636364); tm.push_back(0.628787878787879); tm.push_back(0.875); tm.push_back(1); 
        tm.push_back(0.254752851711027); tm.push_back(0.551330798479088); tm.push_back(0.771863117870722); tm.push_back(0.832699619771863); tm.push_back(1); 
        tm.push_back(0.216400911161731); tm.push_back(0.441913439635535); tm.push_back(0.621867881548975); tm.push_back(0.788154897494305); tm.push_back(1); 
        tm.push_back(0.128676470588235); tm.push_back(0.492647058823529); tm.push_back(0.522058823529412); tm.push_back(0.772058823529412); tm.push_back(1); 
        tm.push_back(0.240625); tm.push_back(0.325); tm.push_back(0.5875); tm.push_back(0.825); tm.push_back(1); 
        expected_transition_matrix = tm;
        
        tolerance = 0.0000001;
    }
    
    ~TransitionMatrixFixture() {
    }
    
    PseudowordGenerator generator;
    std::vector<int> sampling_matrix;
    std::vector<double> expected_transition_matrix;
    double tolerance;
};

BOOST_FIXTURE_TEST_SUITE(PseudowordGenerator_transition_matrix_tests, 
                         TransitionMatrixFixture)

BOOST_AUTO_TEST_CASE(prepare_for_generation) {
    generator.set_sampling_matrix(sampling_matrix);
    generator.prepare_for_generation();
    std::vector<double> transition_matrix(generator.transition_matrix());
    
    for (size_t i = 0; i < transition_matrix.size(); ++i) {
        const double difference = transition_matrix[i] - expected_transition_matrix[i];
        
        if (difference > tolerance || difference < -tolerance) {
            std::stringstream message;
            message << "Error at index " << i << ": expecting value "
                    << expected_transition_matrix[i] << ", got "
                    << transition_matrix[i] << ".";
            BOOST_ERROR(message.str());
            break;
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()

/*========= Check the word generation process. ===================*/
BOOST_FIXTURE_TEST_SUITE(PseudowordGenerator_make_word_tests, 
                         SmallAlphabetWithWordsFixture)

BOOST_AUTO_TEST_CASE(make_word) {
    //This should produce words of the form /BAE(AE)*D/, except for
    //BAEAED itself, as it's a dictionary word.
    std::string base_word("BEAEAD");
    generator.add_dictionary_word(base_word);
    generator.prepare_for_generation();
    bool has_error = false;
    
    for (int i = 0; i < 5; ++i) {
        std::string pseudoword(generator.make_word());
        
        bool is_correct = true;
        
        //Check the length of the word.
        is_correct &= (pseudoword.length() >= 4);
        is_correct &= (pseudoword.length() != 6);
        is_correct &= ((pseudoword.length() % 2) == 0);
        
        if (!is_correct) {
            BOOST_ERROR("Generated word does not have correct length.");
            break;
        }
        
        //Check the composition of the word.
        char expected_char = 'B';
        
        for (size_t c = 0; c < pseudoword.length(); ++c) {
            if (pseudoword.length() - 1 == c) {
                expected_char = 'D';
            
            } else if (c > 0 && (c % 2) == 1) {
                expected_char = 'E';
                
            } else if (c > 0 && (c % 2) == 0) {
                expected_char = 'A';
            }    
            
            if (expected_char != pseudoword[c]) {
                std::stringstream message;
                message << "Error at round " << i << " in word \"" 
                        << pseudoword << "\" at character " << c
                        << ".  Expected character '" << expected_char 
                        << "', have character '" << pseudoword[c]
                        << "'.";
                BOOST_ERROR(message.str());
                has_error = true;
                break;
            }
        }
        
        if (has_error) {
            break;
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
