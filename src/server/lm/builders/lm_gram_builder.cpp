/* 
 * File:   lm_gram_builder.cpp
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
 * Created on April 18, 2015, 12:02 PM
 */
#include "server/lm/builders/lm_gram_builder.hpp"

#include <string>    // std::string
#include <vector>    // std::vector
#include <stdexcept> // std::invalid_argument
#include <algorithm> // std::min

#include "common/utils/logging/logger.hpp"
#include "common/utils/text/string_utils.hpp"
#include "common/utils/exceptions.hpp"

#include "server/lm/dictionaries/basic_word_index.hpp"
#include "server/lm/dictionaries/counting_word_index.hpp"
#include "server/lm/dictionaries/optimizing_word_index.hpp"
#include "server/lm/dictionaries/hashing_word_index.hpp"

using std::invalid_argument;

using namespace uva::utils::logging;
using namespace uva::utils::text;

using namespace uva::smt::bpbd::server::lm::dictionary;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace lm {
                    namespace arpa {

                        template<typename WordIndexType, phrase_length CURR_LEVEL, bool is_mult_weight>
                        const unsigned short int lm_gram_builder<WordIndexType, CURR_LEVEL, is_mult_weight>::MIN_NUM_TOKENS_NGRAM_STR = 2;
                        template<typename WordIndexType, phrase_length CURR_LEVEL, bool is_mult_weight>
                        const unsigned short int lm_gram_builder<WordIndexType, CURR_LEVEL, is_mult_weight>::MAX_NUM_TOKENS_NGRAM_STR = 3;

                        template<typename WordIndexType, phrase_length CURR_LEVEL, bool is_mult_weight>
                        lm_gram_builder<WordIndexType, CURR_LEVEL, is_mult_weight>::lm_gram_builder(const lm_parameters & params, WordIndexType & word_index, typename TAddGramFunct<WordIndexType>::func add_garm_func)
                        : m_params(params), m_word_idx(word_index), m_add_garm_func(add_garm_func), m_token(), m_m_gram(CURR_LEVEL) {
                            LOG_DEBUG2 << "Constructing ARPANGramBuilder(" << CURR_LEVEL << ", trie)" << END_LOG;
                        }

                        template<typename WordIndexType, phrase_length CURR_LEVEL, bool is_mult_weight>
                        lm_gram_builder<WordIndexType, CURR_LEVEL, is_mult_weight>::lm_gram_builder(const lm_gram_builder<WordIndexType, CURR_LEVEL, is_mult_weight>& orig)
                        : m_params(orig.m_params), m_word_idx(orig.m_word_idx), m_add_garm_func(orig.m_add_garm_func), m_token(), m_m_gram(CURR_LEVEL) {
                        }

                        template<typename WordIndexType, phrase_length CURR_LEVEL, bool is_mult_weight>
                        lm_gram_builder<WordIndexType, CURR_LEVEL, is_mult_weight>::~lm_gram_builder() {
                        }

                        template<typename WordIndexType, phrase_length CURR_LEVEL, bool is_mult_weight>
                        bool lm_gram_builder<WordIndexType, CURR_LEVEL, is_mult_weight>::parse_to_gram(text_piece_reader &line) {
                            //Read the first element until the tab, we read until the tab because it should be the probability
                            if (line.get_first_tab(m_token)) {
                                //Try to parse the probability to float
                                if (fast_s_to_f(m_m_gram.m_payload.m_prob, m_token.get_rest_c_str())) {
                                    LOG_DEBUG2 << "Parsed the N-gram log_10 probability: " << m_m_gram.m_payload.m_prob << END_LOG;

                                    //Convert the log_10 probability into the log_e probability weight
                                    m_m_gram.m_payload.m_prob = std::pow(ARPA_PROB_WEIGHT_LOG_BASE, m_m_gram.m_payload.m_prob);
                                    m_m_gram.m_payload.m_prob = std::log(m_m_gram.m_payload.m_prob);
                                    
                                    LOG_DEBUG2 << "Converted the N-gram log_e probability: " << m_m_gram.m_payload.m_prob << END_LOG;
                                    
                                    //In case we need to multiply with the lambda weight do it now;
                                    if (is_mult_weight) {
                                        m_m_gram.m_payload.m_prob *= m_params.get_0_lm_weight();
                                        LOG_DEBUG2 << "Weighted N-gram probability: " << m_m_gram.m_payload.m_back << END_LOG;
                                    }

                                    //Start the new m-gram
                                    m_m_gram.start_new_m_gram();

                                    //Read the first N-1 tokens of the N-gram - space separated
                                    for (int i = 0; i < (CURR_LEVEL - 1); i++) {
                                        if (!line.get_first_space(m_m_gram.get_next_new_token())) {
                                            LOG_WARNING << "An unexpected end of line '" << line.str()
                                                    << "' when reading the " << (i + 1)
                                                    << "'th " << CURR_LEVEL << "-gram token!" << END_LOG;
                                            //The unexpected end of file, broken file format (?)
                                            return false;
                                        }
                                    }

                                    //Read the last token of the N-gram, which is followed by the new line or a tab
                                    if (!line.get_first_tab(m_m_gram.get_next_new_token())) {
                                        LOG_WARNING << "An unexpected end of line '" << line.str()
                                                << "' when reading the " << CURR_LEVEL << "'th "
                                                << CURR_LEVEL << "-gram token!" << END_LOG;
                                        //The unexpected end of file, broken file format (?)
                                        return false;
                                    }

                                    //Now if there is something left it should be the back-off weight, otherwise we are done
                                    if (line.has_more()) {
                                        //Take the remainder of the line and try to parse it!
                                        if (!fast_s_to_f(m_m_gram.m_payload.m_back, line.get_rest_c_str())) {
                                            LOG_WARNING << "Could not parse the remainder of the line '" << line.str()
                                                    << "' as a back-off weight!" << END_LOG;
                                            //The first token was not a float, need to skip to another N-Gram section(?)
                                            return false;
                                        }
                                        LOG_DEBUG2 << "Parsed the N-gram back-off weight: " << m_m_gram.m_payload.m_back << END_LOG;

                                        //In case we need to multiply with the lambda weight do it now;
                                        if (is_mult_weight) {
                                            m_m_gram.m_payload.m_back *= m_params.get_0_lm_weight();
                                            LOG_DEBUG2 << "Weighted N-gram back-off weight: " << m_m_gram.m_payload.m_back << END_LOG;
                                        }
                                    } else {
                                        //There is no back-off so set it to zero
                                        m_m_gram.m_payload.m_back = 0.0;
                                        LOG_DEBUG2 << "The parsed N-gram '" << line.str()
                                                << "' does not have back-off using: " << m_m_gram.m_payload.m_back << END_LOG;
                                    }
                                    return true;
                                } else {
                                    //NOTE: Do it as a debug3 level and not a warning because 
                                    //this will happen each time we need to move on to a new section!
                                    LOG_DEBUG3 << "Could not parse the the string '" << m_token.str()
                                            << "' as a probability!" << END_LOG;
                                    //The first token was not a float, need to skip to another N-Gram section(?)

                                    //Take the line and convert it into a string, then trim it.
                                    string line = m_token.str();
                                    trim(line);
                                    //Skip to the next section only if we are dealing with a non-empty line!
                                    return (line == "");
                                }
                            } else {
                                LOG_DEBUG3 << "An unexpected end of line '" << line.str()
                                        << "', an empty line detected!" << END_LOG;
                                //The unexpected end of line, it is an empty line so we skip it but keep reading this N-gram section
                                return true;
                            }
                        }

                        template<typename WordIndexType, phrase_length CURR_LEVEL, bool is_mult_weight>
                        bool lm_gram_builder<WordIndexType, CURR_LEVEL, is_mult_weight>::parse_line(text_piece_reader & line) {
                            LOG_DEBUG << "Processing the " << CURR_LEVEL << "-Gram (?) line: '" << line << "'" << END_LOG;
                            //We expect a good input, so the result is set to false by default.
                            bool result = false;

                            //First tokenize as a pattern "prob \t gram \t back-off"
                            if (parse_to_gram(line)) {
                                //Prepare the N-gram and for being added to the trie
                                m_m_gram.template prepare_for_adding<WordIndexType>(m_word_idx);

                                LOG_DEBUG << "Adding a " << SSTR(CURR_LEVEL) << "-Gram "
                                        << m_m_gram << " to the Trie" << END_LOG;

                                //Add the obtained N-gram data to the Trie
                                m_add_garm_func(m_m_gram);
                            } else {
                                //If we could not parse the line to gram then it should
                                //be the beginning of the next m-gram section
                                result = true;
                            }

                            LOG_DEBUG << "Finished processing the " << CURR_LEVEL << "-Gram (?) line: '"
                                    << line << "', it is " << (result ? "NOT " : "") << "accepted" << END_LOG;

                            return result;
                        }

                        //Make sure that there will be templates instantiated, at least for the given parameter values

#define INSTANTIATE_ARPA_GRAM_BUILDER_LEVEL_WEIGHT(LEVEL, IS_MULT_WEIGHT) \
                template class lm_gram_builder<basic_word_index, LEVEL, IS_MULT_WEIGHT>; \
                template class lm_gram_builder<counting_word_index, LEVEL, IS_MULT_WEIGHT>; \
                template class lm_gram_builder<hashing_word_index, LEVEL, IS_MULT_WEIGHT>; \
                template class lm_gram_builder<basic_optimizing_word_index, LEVEL, IS_MULT_WEIGHT>; \
                template class lm_gram_builder<counting_optimizing_word_index, LEVEL, IS_MULT_WEIGHT>;

#define INSTANTIATE_ARPA_GRAM_BUILDER_LEVEL(LEVEL) \
                INSTANTIATE_ARPA_GRAM_BUILDER_LEVEL_WEIGHT(LEVEL, true); \
                INSTANTIATE_ARPA_GRAM_BUILDER_LEVEL_WEIGHT(LEVEL, false);

                        INSTANTIATE_ARPA_GRAM_BUILDER_LEVEL(M_GRAM_LEVEL_1);
                        INSTANTIATE_ARPA_GRAM_BUILDER_LEVEL(M_GRAM_LEVEL_2);
                        INSTANTIATE_ARPA_GRAM_BUILDER_LEVEL(M_GRAM_LEVEL_3);
                        INSTANTIATE_ARPA_GRAM_BUILDER_LEVEL(M_GRAM_LEVEL_4);
                        INSTANTIATE_ARPA_GRAM_BUILDER_LEVEL(M_GRAM_LEVEL_5);
                        INSTANTIATE_ARPA_GRAM_BUILDER_LEVEL(M_GRAM_LEVEL_6);
                        INSTANTIATE_ARPA_GRAM_BUILDER_LEVEL(M_GRAM_LEVEL_7);
                    }
                }
            }
        }
    }
}