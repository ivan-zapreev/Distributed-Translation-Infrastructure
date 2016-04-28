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

#include <string>
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
                         * @param is_dist the flag indicating whether there is a left distortion limit or not
                         * @param is_alt_trans the flag indicating if the alternative translations are to be stored when recombining states.
                         * @param NUM_WORDS_PER_SENTENCE the maximum allowed number of words per sentence
                         * @param MAX_HISTORY_LENGTH the maximum allowed length of the target translation hystory
                         * @param MAX_M_GRAM_QUERY_LENGTH the maximum length of the m-gram query
                         */
                        template<bool is_dist, bool is_alt_trans, size_t NUM_WORDS_PER_SENTENCE, size_t MAX_HISTORY_LENGTH, size_t MAX_M_GRAM_QUERY_LENGTH>
                        struct state_data_templ {
                            //Give a short name for the stack data
                            typedef stack_data_templ<is_dist, is_alt_trans, NUM_WORDS_PER_SENTENCE, MAX_HISTORY_LENGTH, MAX_M_GRAM_QUERY_LENGTH> stack_data;
                            //Make the typedef for the stack state translation frame
                            typedef circular_queue<word_uid, MAX_M_GRAM_QUERY_LENGTH > state_frame;

                            //Make the typedef for the covered bitset
                            typedef bitset<NUM_WORDS_PER_SENTENCE> covered_info;

                            //Stores the undefined word index
                            static constexpr int32_t UNDEFINED_WORD_IDX = -1;
                            static constexpr int32_t ZERRO_WORD_IDX = UNDEFINED_WORD_IDX + 1;

                            /**
                             * The basic constructor that is to be used for the BEGIN STATE
                             * @param stack_data the general shared stack data reference 
                             * @param is_begin_end this flag allows to detect whether this
                             * data is created for the begin or end tag. If true then it is
                             * for the begin tag, if false then it is for the end tag
                             */
                            state_data_templ(const stack_data & stack_data)
                            : m_stack_data(stack_data),
                            m_s_begin_word_idx(UNDEFINED_WORD_IDX), m_s_end_word_idx(UNDEFINED_WORD_IDX),
                            m_stack_level(0), m_target(NULL),
                            rm_entry_data(m_stack_data.m_rm_query.get_begin_tag_reordering()),
                            //Add the sentence begin tag uid to the target, since this is for the begin state
                            m_trans_frame(1, &m_stack_data.m_lm_query.get_begin_tag_uid()),
                            m_begin_lm_level(M_GRAM_LEVEL_1),
                            m_covered(), m_partial_score(0.0), m_total_score(0.0) {
                                LOG_DEBUG1 << "New BEGIN state data: " << this << ", translating [" << m_s_begin_word_idx
                                        << ", " << m_s_end_word_idx << "], stack_level=" << m_stack_level
                                        << ", lm_level=" << m_begin_lm_level << ", target = ___<s>___" << END_LOG;

                                LOG_DEBUG2 << "Trans frame: " << m_trans_frame << END_LOG;
                                LOG_DEBUG2 << "Covered: " << covered_to_string() << END_LOG;
                                LOG_DEBUG2 << "Reordering: " << rm_entry_data << END_LOG;
                            }

                            /**
                             * The basic constructor that is to be used for the END STATE
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
                            rm_entry_data(m_stack_data.m_rm_query.get_end_tag_reordering()),
                            //Add the sentence end tag uid to the target, since this is for the end state
                            m_trans_frame(prev_state_data.m_trans_frame, 1, &m_stack_data.m_lm_query.get_end_tag_uid()),
                            m_begin_lm_level(prev_state_data.m_begin_lm_level),
                            //The coverage vector stays the same, nothin new is added, we take over the partial score
                            m_covered(prev_state_data.m_covered), m_partial_score(prev_state_data.m_partial_score), m_total_score(0.0) {
                                LOG_DEBUG1 << "New END state data: " << this << " translating [" << m_s_begin_word_idx
                                        << ", " << m_s_end_word_idx << "], stack_level=" << m_stack_level
                                        << ", lm_level=" << m_begin_lm_level << ", target = ___</s>___" << END_LOG;

                                LOG_DEBUG2 << "Trans frame: " << m_trans_frame << END_LOG;
                                LOG_DEBUG2 << "Covered: " << covered_to_string() << END_LOG;
                                LOG_DEBUG2 << "Reordering: " << rm_entry_data << END_LOG;

                                //Compute the end state final score, the new partial score is then the same as the total score
                                compute_final_score(prev_state_data);
                            }

                            /**
                             * The basic constructor that is to be used for an INTERMEDIATE STATE data,
                             * it takes the parent state data and the new data to be stored/merged
                             * with the parent's data.
                             * @param state_data the constant reference to the parent state data
                             * @param begin_pos this state translated source phrase begin position
                             * @param end_pos this state translated source phrase end position
                             * @param target the pointer to the target translation of the source phrase
                             */
                            state_data_templ(const state_data_templ & prev_state_data,
                                    const int32_t & begin_pos, const int32_t & end_pos,
                                    const covered_info & covered, tm_const_target_entry* target)
                            : m_stack_data(prev_state_data.m_stack_data),
                            m_s_begin_word_idx(begin_pos), m_s_end_word_idx(end_pos),
                            m_stack_level(prev_state_data.m_stack_level + (m_s_end_word_idx - m_s_begin_word_idx + 1)),
                            m_target(target), rm_entry_data(m_stack_data.m_rm_query.get_reordering(m_target->get_st_uid())),
                            m_trans_frame(prev_state_data.m_trans_frame, m_target->get_num_words(), m_target->get_word_ids()),
                            m_begin_lm_level(prev_state_data.m_begin_lm_level),
                            m_covered(covered), m_partial_score(prev_state_data.m_partial_score), m_total_score(0.0) {
                                LOG_DEBUG1 << "New state data: " << this << ", translating [" << m_s_begin_word_idx
                                        << ", " << m_s_end_word_idx << "], stack_level=" << m_stack_level
                                        << ", lm_level=" << m_begin_lm_level << ", target = ___"
                                        << m_target->get_target_phrase() << "___" << END_LOG;

                                LOG_DEBUG2 << "Trans frame: " << m_trans_frame << END_LOG;
                                LOG_DEBUG2 << "Covered: " << covered_to_string() << END_LOG;
                                LOG_DEBUG2 << "Reordering: " << rm_entry_data << END_LOG;

                                //Update the partial score;
                                update_partial_score(prev_state_data);

                                //Compute the total score;
                                compute_total_score();
                            }

                            /**
                             * Allows to give the string representation of the covered vector
                             * @return the string representation of the covered vector
                             */
                            string covered_to_string() const {
                                string result = "[";
                                for (int32_t idx = m_stack_data.m_sent_data.m_min_idx;
                                        idx <= m_stack_data.m_sent_data.m_max_idx; ++idx) {
                                    if (idx == m_s_end_word_idx) {
                                        ASSERT_SANITY_THROW(!m_covered[idx], string("The last covered word index ") +
                                                to_string(idx) + string(" is not marked as covered!"));

                                        result += string("*");
                                    } else {
                                        result += string(m_covered[idx] ? "1" : "0");
                                    }
                                }
                                return result + "]";
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

                            //Stores the translation frame i.e. the number of translated
                            //word ids up until and including now. This structure should
                            //be large enough to store the maximum m-gram length - 1 from
                            //the LM plus the maximum target phrase length from TM
                            const state_frame m_trans_frame;

                            //Stores the minimum m-gram level to consider when computing the LM probability of the history
                            phrase_length m_begin_lm_level;

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
                             * Allows to retrieve the language model probability for the given query, to
                             * do that we need to extract the query from the current translation frame
                             */
                            prob_weight get_lm_probability() {
                                //The number of new words that came into translation is either the
                                //number of words in the target or one, for the <s> or </s> tags
                                const size_t num_new_words = ((m_target != NULL) ? m_target->get_num_words() : 1);

                                //Do the sanity check
                                ASSERT_SANITY_THROW(((MAX_HISTORY_LENGTH + num_new_words) > MAX_M_GRAM_QUERY_LENGTH ),
                                        string("MAX_HISTORY_LENGTH (") + to_string(MAX_HISTORY_LENGTH) +
                                        string(") + num_new_words (") + to_string(num_new_words) +
                                        string(") > MAX_M_GRAM_QUERY_LENGTH (") + to_string(MAX_M_GRAM_QUERY_LENGTH) + string(")"));
                                
                                //It is only for these new words that we need to compute the lm probabilities
                                //for. So compute the current number of elements in the words' history.
                                const size_t all_hist_words = m_trans_frame.get_size() - num_new_words;
                                //The number of interesting words from the history is bounded by MAX_HISTORY_LENGTH
                                const size_t act_hist_words = min(all_hist_words, MAX_HISTORY_LENGTH);

                                //Compute the query length to consider
                                const size_t num_query_words = act_hist_words + num_new_words;

                                //Compute the number of words we need to skip in the query from the translation frame
                                const size_t num_words_to_skip = all_hist_words - act_hist_words;
                                //Compute the pointer to the beginning of the query words array
                                const word_uid * query_word_ids = m_trans_frame.get_elems() + num_words_to_skip;

                                LOG_DEBUG1 << "Begin lm_level: " << m_begin_lm_level << ", hist_words: "
                                        << act_hist_words << ", new_words: " << num_new_words << ", query words: "
                                        << array_to_string<word_uid>(num_query_words, query_word_ids) << END_LOG;
                                
                                //Execute the query and return the value
                                //logger::get_reporting_level() = debug_levels_enum::INFO2;
                                const prob_weight prob = m_stack_data.m_lm_query.execute(num_query_words, query_word_ids, m_begin_lm_level);
                                //logger::get_reporting_level() = debug_levels_enum::DEBUG2;

                                return prob;
                            }

                            /**
                             * Allows to compute the reordering orientatino for the phrase based lexicolized reordering model
                             * @param prev_state_data the previous state translation
                             * @return the reordering orientation
                             */
                            inline reordering_orientation get_reordering_orientation(const state_data_templ & prev_state_data) {
                                LOG_DEBUG2 << "Previous state end_word_idx: " << prev_state_data.m_s_end_word_idx
                                        << ", new state begin_word_idx: " << m_s_begin_word_idx << END_LOG;

                                if (m_s_begin_word_idx < prev_state_data.m_s_end_word_idx) {
                                    //We went to the left from the previous translation
                                    if ((prev_state_data.m_s_begin_word_idx - m_s_end_word_idx) == 1) {
                                        LOG_DEBUG2 << "SWAP_ORIENT" << END_LOG;
                                        //The current phrase is right next to the previous
                                        return reordering_orientation::SWAP_ORIENT;
                                    } else {
                                        LOG_DEBUG2 << "DISCONT_LEFT_ORIENT" << END_LOG;
                                        //We have a discontinuous jump from the previous
                                        return reordering_orientation::DISCONT_LEFT_ORIENT;
                                    }
                                } else {
                                    //We went to the right from the previous translation
                                    if ((m_s_begin_word_idx - prev_state_data.m_s_end_word_idx) == 1) {
                                        LOG_DEBUG2 << "MONOTONE_ORIENT" << END_LOG;
                                        //The current phrase is right next to the previous
                                        return reordering_orientation::MONOTONE_ORIENT;
                                    } else {
                                        LOG_DEBUG2 << "DISCONT_RIGHT_ORIENT" << END_LOG;
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

                                LOG_DEBUG1 << "Initial partial score is: " << partial_score << END_LOG;

                                //Add the language model probability
                                partial_score += get_lm_probability();

                                LOG_DEBUG1 << "partial score + LM is: " << partial_score << END_LOG;

                                //Add the distance based reordering penalty
                                partial_score += -abs(m_s_begin_word_idx - prev_state_data.m_s_end_word_idx - 1);

                                LOG_DEBUG1 << "partial score + RM discrete is: " << partial_score << END_LOG;

                                //Add the lexicolized reordering costs
                                const reordering_orientation orient = get_reordering_orientation(prev_state_data);
                                partial_score += prev_state_data.rm_entry_data.template get_weight<true>(orient);
                                partial_score += rm_entry_data.template get_weight<false>(orient);

                                LOG_DEBUG1 << "partial score + RM lexicolized is: " << partial_score << END_LOG;

                                //After the construction the total score is to stay fixed,
                                //thus it is declared as constant and here we do a const_cast
                                prob_weight & total_score = const_cast<prob_weight &> (m_total_score);

                                //Set the total score to be equal to the partial score as the translation is finished
                                total_score = partial_score;

                                LOG_DEBUG1 << "Total score: " << total_score << END_LOG;
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

                                LOG_DEBUG1 << "Initial partial score is: " << partial_score << END_LOG;

                                //Add the phrase translation probability
                                partial_score += m_target->get_total_weight();

                                LOG_DEBUG1 << "partial score + TM is: " << partial_score << END_LOG;

                                //Add the language model probability
                                partial_score += get_lm_probability();

                                LOG_DEBUG1 << "partial score + LM is: " << partial_score << END_LOG;

                                //Add the phrase penalty
                                partial_score += m_stack_data.m_params.m_phrase_penalty;

                                LOG_DEBUG1 << "partial score + phrase penalty is: " << partial_score << END_LOG;

                                //Add the word penalty
                                partial_score += m_stack_data.m_params.m_word_penalty * m_target->get_num_words();

                                LOG_DEBUG1 << "partial score + word penalty is: " << partial_score << END_LOG;

                                //Add the distance based reordering penalty
                                partial_score += m_stack_data.m_params.m_lin_dist_penalty * abs(m_s_begin_word_idx - prev_state_data.m_s_end_word_idx - 1);

                                LOG_DEBUG1 << "partial score + RM discrete is: " << partial_score << END_LOG;

                                //Add the lexicolized reordering costs
                                const reordering_orientation orient = get_reordering_orientation(prev_state_data);
                                partial_score += prev_state_data.rm_entry_data.template get_weight<true>(orient);
                                partial_score += rm_entry_data.template get_weight<false>(orient);

                                LOG_DEBUG1 << "partial score + RM lexicolized is: " << partial_score << END_LOG;
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

                                LOG_DEBUG1 << "Initial total score: " << total_score << END_LOG;

                                //Iterate through all the non-translated phrase spans and add the future costs thereof
                                phrase_length begin_idx = m_stack_data.m_sent_data.m_min_idx;
                                while (begin_idx <= m_stack_data.m_sent_data.m_max_idx) {
                                    LOG_DEBUG3 << "m_covered[" << begin_idx << "] = "
                                            << (m_covered[begin_idx] ? "true" : "false") << END_LOG;

                                    if (!m_covered[begin_idx]) {
                                        //Start from the begin word, as may be this is the only uncovered word left.
                                        phrase_length end_idx = begin_idx;

                                        LOG_DEBUG3 << "first end_idx: " << end_idx << END_LOG;

                                        //Iterate until we are beyond the last word or the word is covered
                                        while ((end_idx <= m_stack_data.m_sent_data.m_max_idx) && !m_covered[end_idx]) {
                                            ++end_idx;
                                            LOG_DEBUG3 << "Considering end_idx: " << end_idx << END_LOG;
                                        }

                                        LOG_DEBUG3 << "First bad end_idx: " << end_idx << " the previous was good!" << END_LOG;

                                        //The previous end word was the last good one, add the span's costs
                                        total_score += m_stack_data.m_sent_data[begin_idx][end_idx - 1].future_cost;

                                        LOG_DEBUG3 << "total score + future_cost[" << begin_idx << ", "
                                                << (end_idx - 1) << "]: " << total_score << END_LOG;

                                        //Start searching further from the first word after the bad one we just found
                                        begin_idx = end_idx + 1;
                                    } else {
                                        //Move on to the next begin index
                                        ++begin_idx;
                                    }

                                    LOG_DEBUG3 << "next begin_idx: " << begin_idx << END_LOG;
                                }

                                LOG_DEBUG1 << "Total score: " << total_score << END_LOG;
                            }
                        };

                        template<bool is_dist, bool is_alt_trans, size_t NUM_WORDS_PER_SENTENCE, size_t MAX_HISTORY_LENGTH, size_t MAX_M_GRAM_QUERY_LENGTH>
                        constexpr int32_t state_data_templ<is_dist, is_alt_trans, NUM_WORDS_PER_SENTENCE, MAX_HISTORY_LENGTH, MAX_M_GRAM_QUERY_LENGTH>::UNDEFINED_WORD_IDX;

                        template<bool is_dist, bool is_alt_trans, size_t NUM_WORDS_PER_SENTENCE, size_t MAX_HISTORY_LENGTH, size_t MAX_M_GRAM_QUERY_LENGTH>
                        constexpr int32_t state_data_templ<is_dist, is_alt_trans, NUM_WORDS_PER_SENTENCE, MAX_HISTORY_LENGTH, MAX_M_GRAM_QUERY_LENGTH>::ZERRO_WORD_IDX;
                    }
                }
            }
        }
    }
}

#endif /* STATE_DATA_HPP */

