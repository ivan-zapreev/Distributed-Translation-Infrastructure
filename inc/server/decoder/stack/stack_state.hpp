/* 
 * File:   trans_stack_state.hpp
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
 * Created on February 16, 2016, 4:20 PM
 */

#ifndef STACK_STATE_HPP
#define STACK_STATE_HPP

#include <vector>

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"
#include "common/utils/string_utils.hpp"

#include "server/lm/lm_configurator.hpp"
#include "server/rm/proxy/rm_query_proxy.hpp"

#include "server/decoder/de_configs.hpp"
#include "server/decoder/de_parameters.hpp"

#include "server/decoder/stack/state_data.hpp"

using namespace std;

using namespace uva::utils::exceptions;
using namespace uva::utils::logging;
using namespace uva::utils::text;

using namespace uva::smt::bpbd::server::lm::proxy;
using namespace uva::smt::bpbd::server::rm::proxy;

using namespace uva::smt::bpbd::server::decoder;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace decoder {
                    namespace stack {

                        /**
                         * This is the translation stack state class that is responsible for the sentence translation
                         */
                        template<size_t NUM_WORDS_PER_SENTENCE, size_t MAX_HISTORY_LENGTH, size_t MAX_M_GRAM_QUERY_LENGTH>
                        class stack_state_templ {
                        public:
                            //Typedef the state data template for a shorter name
                            typedef state_data_templ<NUM_WORDS_PER_SENTENCE, MAX_HISTORY_LENGTH, MAX_M_GRAM_QUERY_LENGTH> state_data;

                            /**
                             * The basic constructor for the begin stack state
                             * @param data the shared data container
                             */
                            stack_state_templ(const stack_data & data)
                            : m_parent(NULL), m_state_data(data),
                            m_next_in_level(NULL), m_recomb_from(), m_recomb_to(NULL) {
                                LOG_DEBUG2 << "multi_state create: " << m_state_data.m_stack_data.m_params << END_LOG;
                            }

                            /**
                             * The basic constructor for the end stack state
                             * @param parent the parent state pointer, NOT NULL!
                             * @param prev_history the previous translation history
                             */
                            stack_state_templ(stack_state_ptr parent) :
                            m_parent(parent), m_state_data(parent->m_state_data),
                            m_next_in_level(NULL), m_recomb_from(), m_recomb_to(NULL) {
                            }

                            /**
                             * The basic constructor for the non-begin/end stack state
                             * @param parent the parent state pointer, NOT NULL!
                             * @param begin_pos this state translated source phrase begin position
                             * @param end_pos this state translated source phrase end position
                             * @param target the new translation target
                             */
                            stack_state_templ(stack_state_ptr parent,
                                    const int32_t begin_pos, const int32_t end_pos,
                                    tm_const_target_entry* target)
                            : m_parent(parent), m_state_data(parent->m_state_data, begin_pos, end_pos, target),
                            m_next_in_level(NULL), m_recomb_from(), m_recomb_to(NULL) {
                                LOG_DEBUG2 << "stack_state create, with parameters: " << m_state_data.m_stack_data.m_params << END_LOG;
                            }

                            /**
                             * The basic destructor, should free all the allocated
                             * resources and delete the next state in the row
                             */
                            ~stack_state_templ() {
                                //ToDo: Check that we do indeed delete all the allocated data

                                //Delete the states that are recombined into this state
                                for (vector<stack_state_ptr>::const_iterator iter = m_recomb_from.begin(); iter != m_recomb_from.end(); ++iter) {
                                    delete *iter;
                                }

                                //Delete the next state if it exists, this
                                //is for the stack level state deletion
                                if (m_next_in_level != NULL) {
                                    delete m_next_in_level;
                                    m_next_in_level = NULL;
                                }
                            }

                            /**
                             * Allows to get the stack level, the latter is equal
                             * to the number of so far translated words.
                             * @return the stack level
                             */
                            uint32_t get_stack_level() const {
                                //Get the stack level as it was computed during the state creation.
                                return m_state_data.m_stack_level;
                            }

                            /**
                             * Allows the state to expand itself, it will
                             * add itself to the proper stack.
                             */
                            inline void expand() {
                                //Check if this is the last state, i.e. we translated everything
                                if (m_state_data.m_covered.all()) {
                                    //All of the words have been translated, add the end state
                                    m_state_data.m_stack_data.m_add_state(new stack_state(this));
                                } else {
                                    //If there are still things to translate then

                                    //Expand to the left of the last phrase
                                    expand_left();

                                    //Expand to the right of the last phrase
                                    expand_right();
                                }
                            }

                            /**
                             * Allows to get the translation ending in this state.
                             * @param target_sent [out] the variable to store the translation
                             */
                            inline void get_translation(string & target_sent) const {
                                //Go through parents until the root state and append translations when going back!
                                if (m_parent != NULL) {
                                    //Go recursively to the parent
                                    m_parent->get_translation(target_sent);

                                    //Add the space after the parent's text
                                    target_sent += UTF8_SPACE_STRING;
                                }

                                //Append the space plus the current state translation
                                if (m_state_data.m_target->is_unk_trans()) {
                                    //If this is an unknown translation then just copy the original source text

                                    //Get the begin and end source phrase word indexes
                                    const phrase_length begin_word_idx = m_state_data.m_s_begin_word_idx;
                                    const phrase_length end_word_idx = m_state_data.m_s_end_word_idx;

                                    //Get the begin and end source phrase character indexes
                                    const uint32_t begin_ch_idx = m_state_data.m_stack_data.m_sent_data[begin_word_idx][begin_word_idx].m_begin_ch_idx;
                                    const uint32_t end_ch_idx = m_state_data.m_stack_data.m_sent_data[end_word_idx][end_word_idx].m_end_ch_idx;

                                    //Add the source phrase to the target
                                    target_sent += m_state_data.m_stack_data.m_source_sent.substr(begin_ch_idx, end_ch_idx - begin_ch_idx);
                                } else {
                                    //If this is a known translation then add the translation text
                                    target_sent += m_state_data.m_target->get_target_phrase();
                                }
                            }

                            /**
                             * Allows to get the next multi-state
                             * @return the poniter to the next multi-state in the list
                             */
                            inline stack_state_ptr get_next_in_level() const {
                                return m_next_in_level;
                            }

                            /**
                             * Allows to set the next multi-state
                             * @param next the poniter to the next multi-state in the list
                             */
                            inline void set_next_in_level(stack_state_ptr next) {
                                m_next_in_level = next;
                            }

                            /**
                             * Allows to compare two states
                             * @param other the other state to compare with
                             * @return true if this state is smaller than the other one
                             */
                            inline bool operator<(const stack_state & other) const {
                                //ToDo: Implement the comparison operator, 
                                THROW_NOT_IMPLEMENTED();
                            }

                            /**
                             * Allows to compare two states
                             * @param other the other state to compare with
                             * @return true if this state is equal to the other one
                             */
                            inline bool operator==(const stack_state & other) const {
                                //ToDo: Implement
                                THROW_NOT_IMPLEMENTED();
                            }

                        protected:

                            /**
                             * Allows to check that we are within the distortion limits
                             * @param next_start_pos the next start position to test
                             * @return true if we are within the distortion limits
                             */
                            inline bool is_dist_ok(const int32_t next_start_pos) {
                                return (m_state_data.m_stack_data.m_params.m_distortion_limit < 0) ||
                                        (abs(m_state_data.m_s_end_word_idx + 1 - next_start_pos) <= m_state_data.m_stack_data.m_params.m_distortion_limit);
                            }

                            /**
                             * Expand to the left of the last phrase, for all the possible of start positions
                             */
                            inline void expand_left() {
                                //Iterate to the left of the last begin positions until the position is valid and the distortion is within the limits
                                for (int32_t start_pos = (m_state_data.m_s_begin_word_idx - 1);
                                        (start_pos >= 0) && is_dist_ok(start_pos); start_pos--) {
                                    //If the next position is not covered then expand the lengths
                                    if (!m_state_data.m_covered[start_pos]) {
                                        //Expand the lengths
                                        expand_length(start_pos);
                                    }
                                }
                            }

                            /**
                             * Expand to the right of the last phrase, for all the possible of start positions
                             */
                            inline void expand_right() {
                                //Iterate to the right of the last positions until the position is valid and the distortion is within the limits
                                for (uint32_t start_pos = (m_state_data.m_s_end_word_idx + 1);
                                        (start_pos < m_state_data.m_stack_data.m_sent_data.get_dim()) && is_dist_ok(start_pos); ++start_pos) {
                                    //If the next position is not covered then expand the lengths
                                    if (!m_state_data.m_covered[start_pos]) {
                                        //Expand the lengths
                                        expand_length(start_pos);
                                    }
                                }
                            }

                            /**
                             * Allows to expand for all the possible phrase lengths
                             */
                            inline void expand_length(const size_t start_pos) {
                                //Always take the one word translation even if
                                //It is an unknown entry.
                                size_t end_pos = start_pos;
                                //Just expand the single word, this is always needed and possible
                                expand_trans<true>(start_pos, end_pos);

                                //Iterate through lengths > 1 until the maximum possible source phrase length is 
                                //reached or we hit the end of the sentence or the end of the uncovered region
                                for (size_t len = 1; len <= m_state_data.m_stack_data.m_params.m_max_s_phrase_len; ++len) {
                                    //Move to the next end position
                                    ++end_pos;
                                    //Check if we are in a good state
                                    if ((end_pos < m_state_data.m_stack_data.m_sent_data.get_dim()) && (!m_state_data.m_covered[end_pos])) {
                                        //Expand the source phrase which is not a single word
                                        expand_trans<false>(start_pos, end_pos);
                                    } else {
                                        //We've hit the end possible length here
                                        break;
                                    }
                                }
                            }

                            /**
                             * Allows to expand for all the possible translations
                             */
                            template<bool single_word>
                            inline void expand_trans(const size_t start_pos, const size_t end_pos) {
                                //Obtain the source entry for the currently considered source phrase
                                tm_const_source_entry_ptr entry = m_state_data.m_stack_data.m_sent_data[start_pos][end_pos].m_source_entry;

                                //Check if we are in the situation of a single word
                                if (single_word || entry->has_translations()) {
                                    //Get the targets
                                    tm_const_target_entry* targets = entry->get_targets();

                                    //Iterate through all the available target translations
                                    for (size_t idx = 0; idx < entry->num_targets(); ++idx) {
                                        //Add a new hypothesis state to the multi-stack
                                        m_state_data.m_stack_data.m_add_state(new stack_state(this, start_pos, end_pos, &targets[idx]));
                                    }
                                } else {
                                    //Do nothing we have an unknown phrase of length > 1
                                    LOG_DEBUG << "The source phrase " << start_pos << ", " << end_pos << "] has no translations, ignoring!" << END_LOG;
                                }
                            }

                        private:
                            //This variable stores the pointer to the parent state or NULL if it is the root state
                            stack_state_ptr m_parent;

                            //Stores the state data that is to be passed on the the children
                            state_data m_state_data;

                            //This variable stores the pointer to the next state in the stack level or NULL if it is the last one
                            stack_state_ptr m_next_in_level;

                            //This vector stores the list of states recombined into this state
                            vector<stack_state_ptr> m_recomb_from;

                            //This variable stores to which state this state was recombined or NULL
                            stack_state_ptr m_recomb_to;

                        };
                    }
                }
            }
        }
    }
}

#endif /* TRANS_STACK_STATE_HPP */

