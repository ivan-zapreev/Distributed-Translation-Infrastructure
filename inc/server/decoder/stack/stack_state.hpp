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
#include <algorithm>

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
                            m_prev(NULL), m_next(NULL), m_recomb_from() {
                                LOG_DEBUG3 << "stack_state create, with parameters: " << m_state_data.m_stack_data.m_params << END_LOG;
                            }

                            /**
                             * The basic constructor for the end stack state
                             * @param parent the parent state pointer, NOT NULL!
                             * @param prev_history the previous translation history
                             */
                            stack_state_templ(stack_state_ptr parent) :
                            m_parent(parent), m_state_data(parent->m_state_data),
                            m_prev(NULL), m_next(NULL), m_recomb_from() {
                                LOG_DEBUG3 << "stack_state create, with parameters: " << m_state_data.m_stack_data.m_params << END_LOG;
                            }

                            /**
                             * The basic constructor for the non-begin/end stack state
                             * @param parent the parent state pointer, NOT NULL!
                             * @param begin_pos this state translated source phrase begin position
                             * @param end_pos this state translated source phrase end position
                             * @param covered the pre-cooked covered vector, for efficiency reasons.
                             * @param target the new translation target
                             */
                            stack_state_templ(stack_state_ptr parent,
                                    const int32_t begin_pos, const int32_t end_pos,
                                    const typename state_data::covered_info & covered,
                                    tm_const_target_entry* target)
                            : m_parent(parent), m_state_data(parent->m_state_data, begin_pos, end_pos, covered, target),
                            m_prev(NULL), m_next(NULL), m_recomb_from() {
                                LOG_DEBUG1 << "stack_state create for source[" << begin_pos << "," << end_pos
                                        << "], target ___" << target->get_target_phrase() << "___" << END_LOG;
                            }

                            /**
                             * The basic destructor, should free all the allocated
                             * resources and delete the next state in the row
                             */
                            ~stack_state_templ() {
                                //Delete the states that are recombined into this
                                //state as they are not in any stack level
                                for (vector<stack_state_ptr>::const_iterator iter = m_recomb_from.begin(); iter != m_recomb_from.end(); ++iter) {
                                    delete *iter;
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
                                if (m_state_data.m_covered.count() == m_state_data.m_stack_data.m_sent_data.get_dim()) {
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
                                }

                                //Check that the target is not NULL if it is then
                                //it is either the begin <s> or end </s> state
                                if (m_state_data.m_target != NULL) {
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

                                    //Add the space after the new phrase
                                    target_sent += UTF8_SPACE_STRING;
                                }
                            }

                            /**
                             * Allows to get the previous stack-state within the level
                             * @return the pointer reference to the previous stack-state in the level
                             */
                            inline stack_state_ptr get_prev() const {
                                return m_prev;
                            }

                            /**
                             * Allows to get the next stack-state within the level
                             * @return the pointer reference to the next stack-state in the level
                             */
                            inline stack_state_ptr get_next() const {
                                return m_next;
                            }

                            /**
                             * Allows to compare two states, the comparison is based on the state total score.
                             * The state with the bigger total score is considered to be bigger, i.e. more probable.
                             * The state with the smalle total score is considered to be smaller, i.e. less probable.
                             * @param other the other state to compare with
                             * @return true if this state is smaller than the other one
                             */
                            inline bool operator<(const stack_state & other) const {
                                return (m_state_data.m_total_score < other.m_state_data.m_total_score);
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
                             * Expand to the left of the last phrase, for all the possible of start positions
                             */
                            inline void expand_left() {
                                LOG_DEBUG1 << ">>>>> expand left from [" << m_state_data.m_s_begin_word_idx
                                        << "," << m_state_data.m_s_end_word_idx << "]" << END_LOG;

                                //Compute the minimum position to consider, based on distortion
                                int32_t min_pos;
                                if (m_state_data.m_stack_data.m_params.m_is_dist) {
                                    //Compute the normal minimum position for distortion
                                    min_pos = (m_state_data.m_s_end_word_idx - m_state_data.m_stack_data.m_params.m_distortion);
                                    //If the minimum position is then within the last translated
                                    //phrase take into account the extra left distortion
                                    if (min_pos >= m_state_data.m_s_begin_word_idx) {
                                        min_pos = (m_state_data.m_s_begin_word_idx - m_state_data.m_stack_data.m_params.m_ext_dist_left);
                                    }
                                    //Bound the position by the minimum word index
                                    min_pos = max(min_pos, m_state_data.m_stack_data.m_sent_data.m_min_idx);
                                } else {
                                    //There is no distortion limit
                                    min_pos = m_state_data.m_stack_data.m_sent_data.m_min_idx;
                                }

                                //We shall start expanding from the first word before
                                //the beginning of the last translated phrase
                                int32_t curr_pos = (m_state_data.m_s_begin_word_idx - 1);

                                LOG_DEBUG << "start pos = " << curr_pos << ", min pos = " << min_pos << END_LOG;

                                //Iterate to the left of the last begin positions until the
                                //position is valid and the distortion is within the limits
                                while (curr_pos >= min_pos) {
                                    LOG_DEBUG << "Checking the coverage vector @ position " << curr_pos << END_LOG;

                                    //If the next position is not covered then expand the lengths
                                    if (!m_state_data.m_covered[curr_pos]) {
                                        //Expand the lengths
                                        expand_length(curr_pos);
                                    }

                                    //Decrement the start position
                                    curr_pos--;
                                }

                                LOG_DEBUG1 << "<<<<< expand left" << END_LOG;
                            }

                            /**
                             * Expand to the right of the last phrase, for all the possible of start positions
                             */
                            inline void expand_right() {
                                LOG_DEBUG1 << ">>>>> expand right from [" << m_state_data.m_s_begin_word_idx
                                        << "," << m_state_data.m_s_end_word_idx << "]" << END_LOG;

                                //Compute the maximum position to consider, based on distortion
                                int32_t max_pos;
                                if (m_state_data.m_stack_data.m_params.m_is_dist) {
                                    //Compute the normal maximum position for distortion
                                    max_pos = m_state_data.m_s_end_word_idx + m_state_data.m_stack_data.m_params.m_distortion;
                                    //Bound the position by the maximum word index
                                    max_pos = min(max_pos, m_state_data.m_stack_data.m_sent_data.m_max_idx);
                                } else {
                                    //There is no distortion limit
                                    max_pos = m_state_data.m_stack_data.m_sent_data.m_max_idx;
                                }

                                //We shall start expanding from the first word
                                //after the end of the last translated phrase
                                int32_t curr_pos = (m_state_data.m_s_end_word_idx + 1);

                                LOG_DEBUG << "start pos = " << curr_pos << ", max pos = " << max_pos << END_LOG;

                                //Iterate to the right of the last positions until the
                                //position is valid and the distortion is within the limits
                                while (curr_pos <= max_pos) {
                                    LOG_DEBUG << "Checking the coverage vector @ position " << curr_pos << END_LOG;

                                    //If the next position is not covered then expand the lengths
                                    if (!m_state_data.m_covered[curr_pos]) {
                                        //Expand the lengths
                                        expand_length(curr_pos);
                                    }

                                    //Increment the start position
                                    ++curr_pos;
                                }

                                LOG_DEBUG1 << "<<<<< expand right" << END_LOG;
                            }

                            /**
                             * Allows to expand for all the possible phrase lengths
                             */
                            inline void expand_length(const size_t start_pos) {
                                LOG_DEBUG1 << ">>>>> [start_pos] = [" << start_pos << "]" << END_LOG;

                                //Always take the one word translation even if
                                //It is an unknown entry.
                                size_t end_pos = start_pos;
                                //Just expand the single word, this is always needed and possible
                                expand_trans<true>(start_pos, end_pos);

                                //Iterate through lengths > 1 until the maximum possible source phrase length is 
                                //reached or we hit the end of the sentence or the end of the uncovered region
                                for (size_t len = 2; len <= m_state_data.m_stack_data.m_params.m_max_s_phrase_len; ++len) {
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

                                LOG_DEBUG1 << "<<<<< [start_pos] = [" << start_pos << "]" << END_LOG;
                            }

                            /**
                             * Allows to expand for all the possible translations
                             */
                            template<bool single_word>
                            inline void expand_trans(const size_t start_pos, const size_t end_pos) {
                                LOG_DEBUG1 << ">>>>> [start_pos, end_pos] = [" << start_pos << ", " << end_pos << "]" << END_LOG;

                                //Obtain the source entry for the currently considered source phrase
                                tm_const_source_entry_ptr entry = m_state_data.m_stack_data.m_sent_data[start_pos][end_pos].m_source_entry;

                                ASSERT_SANITY_THROW((entry == NULL),
                                        string("The source entry [") + to_string(start_pos) +
                                        string(", ") + to_string(end_pos) + string("] is NULL!"));

                                //Check if we are in the situation of a single word
                                if (single_word || entry->has_translations()) {
                                    //Get the targets
                                    tm_const_target_entry* targets = entry->get_targets();

                                    //Initialize the new covered vector, take the old one plus enable the new states
                                    typename state_data::covered_info covered(m_state_data.m_covered);
                                    for (phrase_length idx = start_pos; idx <= end_pos; ++idx) {
                                        covered.set(idx);
                                    }

                                    //Iterate through all the available target translations
                                    for (size_t idx = 0; idx < entry->num_targets(); ++idx) {
                                        //Add a new hypothesis state to the multi-stack
                                        m_state_data.m_stack_data.m_add_state(new stack_state(this, start_pos, end_pos, covered, &targets[idx]));
                                    }
                                } else {
                                    //Do nothing we have an unknown phrase of length > 1
                                    LOG_DEBUG << "The source phrase [" << start_pos << ", " << end_pos << "] has no translations, ignoring!" << END_LOG;
                                }

                                LOG_DEBUG1 << "<<<<< [start_pos, end_pos] = [" << start_pos << ", " << end_pos << "]" << END_LOG;
                            }

                        private:
                            //This variable stores the pointer to the parent state or NULL if it is the root state
                            stack_state_ptr m_parent;

                            //Stores the state data that is to be passed on the the children
                            state_data m_state_data;

                            //This variable stores the pointer to the previous state in the stack level or NULL if it is the first one
                            stack_state_ptr m_prev;

                            //This variable stores the pointer to the next state in the stack level or NULL if it is the last one
                            stack_state_ptr m_next;

                            //This vector stores the list of states recombined into this state
                            vector<stack_state_ptr> m_recomb_from;

                            //Make the stack level the friend of this class
                            friend class stack_level;
                        };
                    }
                }
            }
        }
    }
}

#endif /* TRANS_STACK_STATE_HPP */

