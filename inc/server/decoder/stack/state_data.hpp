/* 
 * File:   state_data.hpp
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
 * Created on March 1, 2016, 9:30 AM
 */

#ifndef STATE_DATA_HPP
#define STATE_DATA_HPP

#include <bitset>

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"

#include "common/utils/containers/circular_queue.hpp"

#include "server/tm/models/tm_target_entry.hpp"

#include "server/decoder/stack/stack_data.hpp"

using namespace std;

using namespace uva::utils::containers;

using namespace uva::utils::exceptions;
using namespace uva::utils::logging;

using namespace uva::smt::bpbd::server::tm::models;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace decoder {
                    namespace stack {

                        /**
                         * This structure is needed to store the common state data
                         * that however changes/mutates from state to state and thus
                         * is to be passed on from each state to its child.
                         */
                        template<size_t NUM_WORDS_PER_SENTENCE, size_t MAX_M_GRAM_QUERY_LENGTH>
                        struct state_data_templ {
                            //Make the typedef for the stack state history
                            typedef circular_queue<word_uid, MAX_M_GRAM_QUERY_LENGTH > state_history;

                            //Stores the undefined word index
                            static constexpr int32_t UNDEFINED_WORD_IDX = -1;
                            static constexpr int32_t ZERRO_WORD_IDX = UNDEFINED_WORD_IDX + 1;

                            /**
                             * The basic constructor that is to be used for the root state
                             * @param stack_data the general shared stack data reference 
                             */
                            state_data_templ(const stack_data & stack_data)
                            : m_stack_data(stack_data),
                            m_last_begin_pos(UNDEFINED_WORD_IDX), m_last_end_pos(UNDEFINED_WORD_IDX),
                            m_stack_level(0),
                            m_target(NULL), m_history(),
                            m_covered(), m_partial_score(0.0), m_future_cost(0.0) {
                                //Add the sentence begin tag uid to the target, since this is for the root state
                                m_history.push_back(m_stack_data.m_lm_query.get_begin_tag_uid());
                            }

                            /**
                             * The basic constructor that is to be used for a non-root state data,
                             * it takes the parent state data and the new data to be stored/merged
                             * with the parent's data.
                             * @param state_data the constant reference to the parent state data
                             * @param begin_pos this state translated source phrase begin position
                             * @param end_pos this state translated source phrase end position
                             * @param target the pointer to the target translation of the source phrase
                             */
                            state_data_templ(const state_data_templ & state_data,
                                    const int32_t & begin_pos, const int32_t & end_pos,
                                    tm_const_target_entry* target)
                            : m_stack_data(state_data.m_stack_data),
                            m_last_begin_pos(begin_pos), m_last_end_pos(end_pos),
                            m_stack_level(state_data.m_stack_level + (m_last_end_pos - m_last_begin_pos + 1)),
                            m_target(target), m_history(state_data.m_history, m_target->get_num_words(), m_target->get_word_ids()),
                            m_covered(state_data.m_covered), m_partial_score(state_data.m_partial_score), m_future_cost(state_data.m_future_cost) {
                                
                                //ToDo: update the covered vector with the bits that are now enabled
                                THROW_NOT_IMPLEMENTED();

                                //Compute the partial score;
                                compute_partial_score();

                                //Compute the future costs;
                                compute_future_cost();
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

                            //Stores the reference to the parameters
                            const stack_data & m_stack_data;

                            //Stores the last translated word index
                            const int32_t m_last_begin_pos;
                            //Stores the last translated word index
                            const int32_t m_last_end_pos;

                            //Stores the stack level, i.e. the number of so far translated source words
                            const phrase_length m_stack_level;

                            //Stores the pointer to the target translation of the last phrase
                            tm_const_target_entry* m_target;

                            //Stores the translation history i.e. the number of translated
                            //word ids up until now. This structure should be large enough
                            //to store the maximum m-gram length - 1 from the LM plus the
                            //maximum target phrase length from TM
                            state_history m_history;

                            //Stores the bitset of covered words indexes
                            bitset<NUM_WORDS_PER_SENTENCE> m_covered;

                            //Stores the logarithmic partial score of the current hypothesis
                            prob_weight m_partial_score;

                            //Stores the logarithmic future cost of the current hypothesis
                            prob_weight m_future_cost;

                        };

                        template<size_t NUM_WORDS_PER_SENTENCE, size_t MAX_M_GRAM_QUERY_LENGTH>
                        constexpr int32_t state_data_templ<NUM_WORDS_PER_SENTENCE, MAX_M_GRAM_QUERY_LENGTH>::UNDEFINED_WORD_IDX;

                        template<size_t NUM_WORDS_PER_SENTENCE, size_t MAX_M_GRAM_QUERY_LENGTH>
                        constexpr int32_t state_data_templ<NUM_WORDS_PER_SENTENCE, MAX_M_GRAM_QUERY_LENGTH>::ZERRO_WORD_IDX;
                    }
                }
            }
        }
    }
}

#endif /* STATE_DATA_HPP */

