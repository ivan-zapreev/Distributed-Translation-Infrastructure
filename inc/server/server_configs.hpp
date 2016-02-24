/* 
 * File:   server_configs.hpp
 * Author: Dr. Ivan S. Zapreev
 *
 * Visit my Linked-in profile:
 *      <https://nl.linkedin.com/in/zapreevis>
 * Visit my GitHub:
 *      <https://github.com/ivan-zapreev>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.#
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Created on February 23, 2016, 4:52 PM
 */

#ifndef SERVER_CONFIGS_HPP
#define SERVER_CONFIGS_HPP

#include <string>

using namespace std;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {

                //This typedef if used for the translation model level
                typedef uint16_t phrase_length;

                //The type used for storing log probabilities, back-off, and feature values
                typedef float prob_weight;

                //Declare the phrase unique identifier type
                typedef uint64_t phrase_uid;

                //Declare the phrase unique identifier type
                typedef uint64_t word_uid;

                //The base of the logarithm for stored probabilities/back-off weights
                static constexpr prob_weight LOG_PROB_WEIGHT_BASE = 10.0;
                //Stores the zero log probability for the partial score or future cost
                static constexpr prob_weight UNKNOWN_LOG_PROB_WEIGHT = -1000.0;
                //The zero like value for log probability weight
                static constexpr prob_weight ZERO_LOG_PROB_WEIGHT = -100.0f;

                namespace decoder {
                    //Stores the start of the sentence symbol
                    static const string BEGIN_SENTENCE_TAG_STR = "<s>";
                    //Stores the end of the sentence symbol
                    static const string END_SENTENCE_TAG_STR = "</s>";
                }

                namespace tm {
                    //Define the feature weights delimiter string for the config file
                    static const string TM_FEATURE_WEIGHTS_DELIMITER_STR = "|";
                    //The various numbers of TM features
                    static constexpr size_t FOUR_TM_FEATURES = 4u;
                    //Stores the maximum number of the translation model features
                    static constexpr size_t MAX_NUM_TM_FEATURES = FOUR_TM_FEATURES;
                    
                    //The considered maximum length of the target phrase
                    static constexpr uint8_t TM_MAX_TARGET_PHRASE_LEN = 7u;

                    //Stores the unknown source phrase string, should be configurable
                    static const string TM_UNKNOWN_SOURCE_STR = "UNK";
                    //Stores the unknown target phrase string, should be configurable
                    static const string TM_UNKNOWN_TARGET_STR = "<unk>";

                    //Stores the unknown source word log probability penalty
                    static constexpr prob_weight UNK_SOURCE_WORD_LOG_PROB = -10.0f;
                }

                namespace lm {
                    //Define the feature weights delimiter string for the config file
                    static const string LM_FEATURE_WEIGHTS_DELIMITER_STR = "|";
                    //Stores the maximum number of the language model features
                    static constexpr size_t LM_MAX_NUM_FEATURES = 1u;

                    //The considered maximum length of the N-gram 
                    static constexpr uint8_t LM_M_GRAM_LEVEL_MAX = 5u;
                    //The considered maximum length of the N-gram query
                    static constexpr uint8_t LM_MAX_QUERY_LEN = tm::TM_MAX_TARGET_PHRASE_LEN + LM_M_GRAM_LEVEL_MAX - 1;

                    //Stores the unknown word string, should be configurable
                    static const string UNKNOWN_WORD_STR = "<unk>";
                    
                    //The default value of the unknown word probability weight
                    const prob_weight DEFAULT_UNK_WORD_LOG_PROB_WEIGHT = -10.0f;
                }

                namespace rm {
                    //Define the feature weights delimiter string for the config file
                    static const string RM_FEATURE_WEIGHTS_DELIMITER_STR = "|";
                    //Stores the various supported numbers of RM features
                    static constexpr size_t SIX_RM_FEATURES = 6u;
                    static constexpr size_t EIGHT_RM_FEATURES = 8u;
                    //Stores the maximum number of the reordering model features
                    static constexpr size_t MAX_NUM_RM_FEATURES = EIGHT_RM_FEATURES;

                    //Stores the unknown source phrase string, should be configurable
                    static const string RM_UNK_SOURCE_PHRASE = "UNK";
                    //Stores the unknown target phrase string, should be configurable
                    static const string RM_UNK_TARGET_PHRASE = "UNK";
                }
            }
        }
    }
}

//INclude server constants as they depend on server configs
#include "server/server_consts.hpp"

#endif /* SERVER_CONFIGS_HPP */

