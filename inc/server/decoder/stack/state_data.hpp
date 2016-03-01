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
#include "server/rm/models/rm_entry.hpp"

#include "server/decoder/stack/stack_data.hpp"

using namespace std;

using namespace uva::utils::containers;

using namespace uva::utils::exceptions;
using namespace uva::utils::logging;

using namespace uva::smt::bpbd::server::tm::models;
using namespace uva::smt::bpbd::server::rm::models;

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

                            //Make the typedef for the covered bitset
                            typedef bitset<NUM_WORDS_PER_SENTENCE> covered_info;

                            //Stores the undefined word index
                            static constexpr int32_t UNDEFINED_WORD_IDX = -1;
                            static constexpr int32_t ZERRO_WORD_IDX = UNDEFINED_WORD_IDX + 1;

                            /**
                             * The basic constructor that is to be used for the root state
                             * @param stack_data the general shared stack data reference 
                             * @param is_begin_end this flag allows to detect whether this
                             * data is created for the begin or end tag. If true then it is
                             * for the begin tag, if false then it is for the end tag
                             */
                            state_data_templ(const stack_data & stack_data)
                            : m_stack_data(stack_data),
                            m_s_begin_word_idx(UNDEFINED_WORD_IDX), m_s_end_word_idx(UNDEFINED_WORD_IDX),
                            m_stack_level(0), m_target(NULL),
                            rm_entry_data(m_stack_data.m_rm_query.get_begin_tag_reorderin()),
                            //Add the sentence begin tag uid to the target, since this is for the begin state
                            m_history(1, &m_stack_data.m_lm_query.get_begin_tag_uid()),
                            m_covered(), m_partial_score(0.0), m_total_score(0.0) {
                            }

                            /**
                             * The basic constructor that is to be used for the root state
                             * @param stack_data the general shared stack data reference 
                             * @param is_begin_end this flag allows to detect whether this
                             * data is created for the begin or end tag. If true then it is
                             * for the begin tag, if false then it is for the end tag
                             */
                            state_data_templ(const state_data_templ & prev_state_data)
                            : m_stack_data(prev_state_data.m_stack_data),
                            //Set the start and end word index to be the index after the last word in the sentence
                            m_s_begin_word_idx(m_stack_data.m_sent_data.get_dim()), m_s_end_word_idx(m_s_begin_word_idx),
                            //This is the next state level, i.e. the last one but there is of course no target for </s>
                            m_stack_level(prev_state_data.m_stack_level + 1), m_target(NULL),
                            //The reordering entry should contain the end tag reordering
                            rm_entry_data(m_stack_data.m_rm_query.get_end_tag_reorderin()),
                            //Add the sentence end tag uid to the target, since this is for the end state
                            m_history(prev_state_data.m_history, 1, &m_stack_data.m_lm_query.get_end_tag_uid()),
                            //The coverage vector stays the same, nothin new is added, we take over the partial score
                            m_covered(prev_state_data.m_covered), m_partial_score(prev_state_data.m_partial_score), m_total_score(0.0) {
                                //Compute the end state final score, the new partial score is then the same as the total score
                                compute_final_score(prev_state_data);
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
                            state_data_templ(const state_data_templ & prev_state_data,
                                    const int32_t & begin_pos, const int32_t & end_pos,
                                    tm_const_target_entry* target)
                            : m_stack_data(prev_state_data.m_stack_data),
                            m_s_begin_word_idx(begin_pos), m_s_end_word_idx(end_pos),
                            m_stack_level(prev_state_data.m_stack_level + (m_s_end_word_idx - m_s_begin_word_idx + 1)),
                            m_target(target), rm_entry_data(m_stack_data.m_rm_query.get_reordering(m_target->get_st_uid())),
                            m_history(prev_state_data.m_history, m_target->get_num_words(), m_target->get_word_ids()),
                            m_covered(prev_state_data.m_covered), m_partial_score(prev_state_data.m_partial_score), m_total_score(0.0) {
                                //Update the covered vector with the bits that are now enabled
                                //After the construction the covered bits vector is to stay fixed,
                                //therefore it is declared as constant and here we do a const_cast
                                covered_info & covered_ref = const_cast<covered_info &> (m_covered);
                                for (phrase_length idx = m_s_begin_word_idx; idx <= end_pos; ++idx) {
                                    covered_ref.set(idx);
                                }

                                //Update the partial score;
                                update_partial_score(prev_state_data);

                                //Compute the total score;
                                compute_total_score();
                            }

                            //Stores the reference to the parameters
                            const stack_data & m_stack_data;

                            //Stores the begin word index of the translated phrase in the source sentence
                            const int32_t m_s_begin_word_idx;
                            //Stores the end word index of the translated phrase in the source sentence
                            const int32_t m_s_end_word_idx;

                            //Stores the stack level, i.e. the number of so far translated source words
                            const phrase_length m_stack_level;

                            //Stores the pointer to the target translation of the last phrase
                            tm_const_target_entry * const m_target;

                            //Stores the reference to the reordering model entry data corresponding to this state
                            const rm_entry & rm_entry_data;

                            //Stores the translation history i.e. the number of translated
                            //word ids up until now. This structure should be large enough
                            //to store the maximum m-gram length - 1 from the LM plus the
                            //maximum target phrase length from TM
                            const state_history m_history;

                            //Stores the bitset of covered words indexes
                            const covered_info m_covered;

                            //Stores the logarithmic partial score of the current hypothesis
                            const prob_weight m_partial_score;

                            //Stores the logarithmic totasl score of the current hypothesis
                            //The total score is the sum of the current partial score and
                            //the future cost estimate for the given hypothesis state
                            const prob_weight m_total_score;

                        private:

                            /**
                             * Allows to compute the reordering orientatino for the phrase based lexicolized reordering model
                             * @param prev_state_data the previous state translation
                             * @return the reordering orientation
                             */
                            inline reordering_orientation get_reordering_orientation(const state_data_templ & prev_state_data) {
                                if (m_s_begin_word_idx < prev_state_data.m_s_begin_word_idx) {
                                    //We went to the left from the previous translation
                                    if ((prev_state_data.m_s_begin_word_idx - m_s_end_word_idx) == 1) {
                                        //The current phrase is right next to the previous
                                        return reordering_orientation::SWAP_ORIENT;
                                    } else {
                                        //We have a discontinuous jump from the previous
                                        return reordering_orientation::DISCONT_LEFT_ORIENT;
                                    }
                                } else {
                                    //We went to the right from the previous translation
                                    if ((m_s_begin_word_idx - prev_state_data.m_s_end_word_idx) == 1) {
                                        //The current phrase is right next to the previous
                                        return reordering_orientation::MONOTONE_ORIENT;
                                    } else {
                                        //We have a discontinuous jump from the previous
                                        return reordering_orientation::DISCONT_RIGHT_ORIENT;
                                    }
                                }
                            }

                            /**
                             * For the end translation hypothesis state is supposed to compute the partial and total score
                             * @param prev_state_data the previous state data
                             */
                            inline void compute_final_score(const state_data_templ & prev_state_data) {
                                //After the construction the partial score is to stay fixed,
                                //thus it is declared as constant and here we do a const_cast
                                prob_weight & partial_score = const_cast<prob_weight &> (m_partial_score);

                                //ToDo: Add the language model probability
                                //partial_score +=;
                                THROW_NOT_IMPLEMENTED();

                                //Add the distance based reordering penalty
                                partial_score += -abs(m_s_begin_word_idx - prev_state_data.m_s_end_word_idx - 1);

                                //Add the lexicolized reordering costs
                                const reordering_orientation orient = get_reordering_orientation(prev_state_data);
                                partial_score += prev_state_data.rm_entry_data.template get_weight<true>(orient);
                                partial_score += rm_entry_data.template get_weight<false>(orient);

                                //After the construction the total score is to stay fixed,
                                //thus it is declared as constant and here we do a const_cast
                                prob_weight & total_score = const_cast<prob_weight &> (m_total_score);

                                //Set the total score to be equal to the partial score as the translation is finished
                                total_score = partial_score;
                            }

                            /**
                             * Allows to update the currently stored partial score from the
                             * parent state with the partial score of the current hypothesis
                             * @param prev_state_data the previous strate data
                             */
                            inline void update_partial_score(const state_data_templ & prev_state_data) {
                                //After the construction the partial score is to stay fixed,
                                //thus it is declared as constant and here we do a const_cast
                                prob_weight & partial_score = const_cast<prob_weight &> (m_partial_score);

                                //Add the phrase translation probability
                                partial_score += m_target->get_total_weight();

                                //ToDo: Add the language model probability
                                //partial_score +=;
                                THROW_NOT_IMPLEMENTED();

                                //Add the phrase penalty
                                partial_score += m_stack_data.m_params.m_phrase_penalty;

                                //Add the word penalty
                                partial_score += m_stack_data.m_params.m_word_penalty * m_target->get_num_words();

                                //Add the distance based reordering penalty
                                partial_score += -abs(m_s_begin_word_idx - prev_state_data.m_s_end_word_idx - 1);

                                //Add the lexicolized reordering costs
                                const reordering_orientation orient = get_reordering_orientation(prev_state_data);
                                partial_score += prev_state_data.rm_entry_data.template get_weight<true>(orient);
                                partial_score += rm_entry_data.template get_weight<false>(orient);
                            }

                            /**
                             * Allows to compute the total score of the current hypothesis
                             * I.e. the partial score plus the future cost estimate
                             */
                            inline void compute_total_score() {
                                //After the construction the total score is to stay fixed,
                                //thus it is declared as constant and here we do a const_cast
                                prob_weight & total_score = const_cast<prob_weight &> (m_total_score);

                                //Set the total score to the current partial score and then add the future costs
                                total_score = m_partial_score;

                                //ToDo: Add the future costs
                                THROW_NOT_IMPLEMENTED();
                            }
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

