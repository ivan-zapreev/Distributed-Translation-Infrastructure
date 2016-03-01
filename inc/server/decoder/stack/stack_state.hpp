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
#include <bitset>

#include "common/utils/containers/circular_queue.hpp"

#include "server/lm/lm_configurator.hpp"
#include "server/rm/proxy/rm_query_proxy.hpp"

#include "server/decoder/de_configs.hpp"
#include "server/decoder/de_parameters.hpp"

#include "server/decoder/stack/stack_data.hpp"

using namespace std;

using namespace uva::utils::containers;

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
                        template<size_t NUM_WORDS_PER_SENTENCE, size_t MAX_M_GRAM_LENGTH>
                        class stack_state_templ {
                        public:
                            //Make the tyopedef for the stack state history
                            typedef circular_queue<word_uid, MAX_M_GRAM_LENGTH - 1 > stack_state_history;

                            //Stores the undefined word index
                            static constexpr int32_t UNDEFINED_WORD_IDX = -1;
                            static constexpr int32_t ZERRO_WORD_IDX = UNDEFINED_WORD_IDX + 1;

                            /**
                             * The basic constructor for the root stack state
                             * @param data the shared data container
                             */
                            stack_state_templ(const stack_data & data)
                            : m_data(data), m_parent(NULL), m_next_in_level(NULL), m_recomb_from(),
                            m_recomb_to(NULL), m_covered(), m_last_begin_pos(UNDEFINED_WORD_IDX),
                            m_last_end_pos(UNDEFINED_WORD_IDX), m_target(NULL), m_history(),
                            m_partial_score(0.0), m_future_cost(0.0) {
                                LOG_DEBUG2 << "multi_state create: " << m_data.m_params << END_LOG;

                                //Mark the zero word as covered
                                m_covered.set(ZERRO_WORD_IDX);

                                //Add the sentence begin tag uid to the target
                                m_history.push_back(m_data.m_lm_query.get_begin_tag_uid());
                            }

                            /**
                             * The basic constructor for the non-root stack state
                             * @param parent the pointer to the parent element
                             */
                            stack_state_templ(const stack_data & data,
                                    stack_state_ptr parent,
                                    const int32_t last_begin_pos,
                                    const int32_t last_end_pos,
                                    tm_const_target_entry* target)
                            : m_data(data), m_parent(NULL), m_next_in_level(NULL), m_recomb_from(),
                            m_recomb_to(NULL), m_covered(), m_last_begin_pos(last_begin_pos),
                            m_last_end_pos(last_end_pos), m_target(target), m_history(),
                            m_partial_score(0.0), m_future_cost(0.0) {
                                LOG_DEBUG2 << "stack_state create, with parameters: " << m_data.m_params << END_LOG;

                                //Compute the partial score;
                                compute_partial_score();

                                //Compute the future costs;
                                compute_future_cost();
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
                             * to the number of  so far translated words.
                             * @return the stack level
                             */
                            uint32_t get_stack_level() const {
                                //ToDo: Implement
                                THROW_NOT_IMPLEMENTED();
                            }

                            /**
                             * Allows the state to expand itself, it will
                             * add itself to the proper stack.
                             */
                            inline void expand() {
                                //Expand to the left of the last phrase
                                expand_left();
                                //Expand to the right of the last phrase
                                expand_right();
                            }

                            /**
                             * Allows to get the translation ending in this state.
                             * @param target_sent [out] the variable to store the translation
                             */
                            inline void get_translation(string & target_sent) const {
                                //ToDo: Implement
                                THROW_NOT_IMPLEMENTED();
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
                                return m_next_in_level = next;
                            }

                            /**
                             * Allows to compare two states
                             * @param other the other state to compare with
                             * @return true if this state is smaller than the other one
                             */
                            inline bool operator<(const stack_state & other) const {
                                //ToDo: Implement
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
                                return (m_data.m_params.m_distortion_limit < 0) ||
                                        (abs(m_last_end_pos + 1 - next_start_pos) <= m_data.m_params.m_distortion_limit);
                            }

                            /**
                             * Expand to the left of the last phrase, for all the possible of start positions
                             */
                            inline void expand_left() {
                                //Iterate to the left of the last begin positions until the position is valid and the distortion is within the limits
                                for (int32_t start_pos = (m_last_begin_pos - 1);
                                        (start_pos >= 0) && is_dist_ok(start_pos); start_pos--) {
                                    //If the next position is not covered then expand the lengths
                                    if (!m_covered[start_pos]) {
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
                                for (uint32_t start_pos = (m_last_end_pos + 1);
                                        (start_pos < m_data.m_sent_data.get_dim()) && is_dist_ok(start_pos); ++start_pos) {
                                    //If the next position is not covered then expand the lengths
                                    if (!m_covered[start_pos]) {
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
                                for (size_t len = 1; len <= m_data.m_params.m_max_s_phrase_len; ++len) {
                                    //Move to the next end position
                                    ++end_pos;
                                    //Check if we are in a good state
                                    if ((end_pos < m_data.m_sent_data.get_dim()) && (!m_covered[end_pos])) {
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
                                tm_const_source_entry_ptr entry = m_data.m_sent_data[start_pos][end_pos].m_source_entry;

                                //Check if we are in the situation of a single word
                                if (single_word || entry->has_translations()) {
                                    //Get the targets
                                    tm_const_target_entry* targets = entry->get_targets();

                                    //Iterate through all the available target translations
                                    for (size_t idx = 0; idx < entry->num_targets(); ++idx) {
                                        //ToDo: Add translation and translation history !!!!
                                        THROW_NOT_IMPLEMENTED();

                                        //Add a new hypothesis state to the multi-stack
                                        m_data.m_add_state(new stack_state(m_data, this, start_pos, end_pos, &targets[idx]));
                                    }
                                } else {
                                    //Do nothing we have an unknown phrase of length > 1

                                    //ToDo: Just log it!
                                    THROW_NOT_IMPLEMENTED();
                                }
                            }

                            /**
                             * Allows to compute the partial score of the current hypothesis
                             */
                            inline void compute_partial_score() {
                                //ToDo: Implement
                                THROW_NOT_IMPLEMENTED();
                            }

                            /**
                             * Allows to compute the future score of the current hypothesis
                             */
                            inline void compute_future_cost() {
                                //ToDo: Implement
                                THROW_NOT_IMPLEMENTED();
                            }

                        private:
                            //Stores the reference to the parameters
                            const stack_data & m_data;

                            //This variable stores the pointer to the parent state or NULL if it is the root state
                            stack_state_ptr m_parent;

                            //This variable stores the pointer to the next state in the stack level or NULL if it is the last one
                            stack_state_ptr m_next_in_level;

                            //This vector stores the list of states recombined into this state
                            vector<stack_state_ptr> m_recomb_from;

                            //This variable stores to which state this state was recombined or NULL
                            stack_state_ptr m_recomb_to;

                            //Stores the bitset of covered words indexes
                            bitset<NUM_WORDS_PER_SENTENCE> m_covered;

                            //Stores the last translated word index
                            int32_t m_last_begin_pos;

                            //Stores the last translated word index
                            int32_t m_last_end_pos;

                            //Stores the pointer to the target translation of the last phrase
                            tm_const_target_entry* m_target;

                            //Stores the N-1 previously translated words
                            stack_state_history m_history;

                            //Stores the logarithmic partial score of the current hypothesis
                            float m_partial_score;

                            //Stores the logarithmic future cost of the current hypothesis
                            float m_future_cost;
                        };

                        template<size_t NUM_WORDS_PER_SENTENCE, size_t MAX_M_GRAM_LENGTH>
                        constexpr int32_t stack_state_templ<NUM_WORDS_PER_SENTENCE, MAX_M_GRAM_LENGTH>::UNDEFINED_WORD_IDX;

                        template<size_t NUM_WORDS_PER_SENTENCE, size_t MAX_M_GRAM_LENGTH>
                        constexpr int32_t stack_state_templ<NUM_WORDS_PER_SENTENCE, MAX_M_GRAM_LENGTH>::ZERRO_WORD_IDX;
                    }
                }
            }
        }
    }
}

#endif /* TRANS_STACK_STATE_HPP */

