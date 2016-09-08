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
                         * @param is_dist the flag indicating whether there is a distortion limit or not
                         * @param NUM_WORDS_PER_SENTENCE the maximum allowed number of words per sentence
                         * @param MAX_HISTORY_LENGTH the maximum allowed length of the target translation hystory
                         * @param MAX_M_GRAM_QUERY_LENGTH the maximum length of the m-gram query
                         */
                        template<bool is_dist, size_t NUM_WORDS_PER_SENTENCE, size_t MAX_HISTORY_LENGTH, size_t MAX_M_GRAM_QUERY_LENGTH>
                        struct state_data_templ {
                            //Give a short name for the stack data
                            typedef stack_data_templ<is_dist, NUM_WORDS_PER_SENTENCE, MAX_HISTORY_LENGTH, MAX_M_GRAM_QUERY_LENGTH> stack_data;
                            //Make the typedef for the stack state translation frame
                            typedef circular_queue<word_uid, MAX_M_GRAM_QUERY_LENGTH > state_frame;

                            //Make the typedef for the covered bitset
                            typedef bitset<NUM_WORDS_PER_SENTENCE> covered_info;

                            //Stores the undefined word index
                            static constexpr int32_t UNDEFINED_WORD_IDX = -1;
                            static constexpr int32_t ZERRO_WORD_IDX = UNDEFINED_WORD_IDX + 1;

#if IS_SERVER_TUNING_MODE
#define INIT_STATE_DATA_TUNING_DATA , m_lattice_scores(new prob_weight[m_stack_data.get_num_features()]())
#define PASS_TUNING_FEATURES_MAP m_lattice_scores
#define ADD_TUNING_FEATURE_SCORE(id, value) m_lattice_scores[id] = value;
#else
#define INIT_STATE_DATA_TUNING_DATA
#define PASS_TUNING_FEATURES_MAP NULL
#define ADD_TUNING_FEATURE_SCORE(name, value)
#endif                        

                            /**
                             * The basic constructor that is to be used for the BEGIN STATE <s>
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
                            m_covered(), m_partial_score(0.0), m_total_score(0.0) INIT_STATE_DATA_TUNING_DATA{
                                LOG_DEBUG1 << "New BEGIN state data: " << this << ", translating [" << m_s_begin_word_idx
                                << ", " << m_s_end_word_idx << "], stack_level=" << m_stack_level
                                << ", lm_level=" << m_begin_lm_level << ", target = ___<s>___" << END_LOG;

                                LOG_DEBUG2 << "Trans frame: " << m_trans_frame << END_LOG;
                                LOG_DEBUG2 << "Covered: " << covered_to_string() << END_LOG;
                                LOG_DEBUG2 << "Reordering: " << rm_entry_data << END_LOG;
                            }

                            /**
                             * The basic constructor that is to be used for the END STATE </s>
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
                            //The coverage vector stays the same, nothing new is added, we take over the partial score
                            m_covered(prev_state_data.m_covered), m_partial_score(prev_state_data.m_partial_score),
                            m_total_score(0.0) INIT_STATE_DATA_TUNING_DATA{
                                LOG_DEBUG1 << "New END state data: " << this << " translating [" << m_s_begin_word_idx
                                << ", " << m_s_end_word_idx << "], stack_level=" << m_stack_level
                                << ", lm_level=" << m_begin_lm_level << ", target = ___</s>___" << END_LOG;

                                LOG_DEBUG2 << "Trans frame: " << m_trans_frame << END_LOG;
                                LOG_DEBUG2 << "Covered: " << covered_to_string() << END_LOG;
                                LOG_DEBUG2 << "Reordering: " << rm_entry_data << END_LOG;

                                //Compute the end state final score, the new partial score is then the same as the total score
                                compute_partial_score_final(prev_state_data);

                                //Compute the total score
                                compute_total_score_final();
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
                            m_covered(covered), m_partial_score(prev_state_data.m_partial_score),
                            m_total_score(0.0) INIT_STATE_DATA_TUNING_DATA{
                                LOG_DEBUG1 << "New state data: " << this << ", translating [" << m_s_begin_word_idx
                                << ", " << m_s_end_word_idx << "], stack_level=" << m_stack_level
                                << ", lm_level=" << m_begin_lm_level << ", target = ___"
                                << m_target->get_target_phrase() << "___" << END_LOG;

                                LOG_DEBUG2 << "Trans frame: " << m_trans_frame << END_LOG;
                                LOG_DEBUG2 << "Covered: " << covered_to_string() << END_LOG;
                                LOG_DEBUG2 << "Reordering: " << rm_entry_data << END_LOG;

                                //Update the partial score;
                                compute_partial_score(prev_state_data);

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

                            /**
                             * Extract the target, including the case when we are in the
                             * begin <s> or end state </s> or a phrase with no translation.
                             * @param is_lattice if true then we are retrieving a translation
                             *                   for lattice dump in this case we just set the
                             *                   target to the argument value and do not append,
                             *                   also we do not add a space after. Default is false.
                             * @param storage the storage string to append or set the target string to.
                             */
                            template<bool is_lattice = false >
                            void get_target_phrase(string & storage) const {
                                LOG_DEBUG1 << "The BEGIN storage value is: ___" << storage << "___" << END_LOG;

                                LOG_DEBUG1 << "The target phrase pointer is " << ((m_target != NULL) ? "NOT " : "") << "NULL" << END_LOG;
                                //Check that the target is not NULL if it is then
                                //it is either the begin <s> or end </s> state
                                if (m_target != NULL) {
                                    LOG_DEBUG1 << "The source phrase has " << (m_target->is_unk_trans() ? "NO " : "") << "translation" << END_LOG;
                                    //Append the space plus the current state translation
                                    if (m_target->is_unk_trans()) {
                                        //If this is an unknown translation then just copy the original source text

                                        //Get the begin and end source phrase word indexes
                                        const phrase_length begin_word_idx = m_s_begin_word_idx;
                                        const phrase_length end_word_idx = m_s_end_word_idx;

                                        LOG_DEBUG1 << "begin_word_idx = " << to_string(begin_word_idx) << ", end_word_idx =" << to_string(end_word_idx) << END_LOG;

                                        //Get the begin and end source phrase character indexes
                                        const uint32_t begin_ch_idx = m_stack_data.m_sent_data[begin_word_idx][begin_word_idx].m_begin_ch_idx;
                                        LOG_DEBUG1 << "m_stack_data.m_sent_data[" << to_string(begin_word_idx) << "]["
                                                << to_string(begin_word_idx) << "].m_begin_ch_idx = " << to_string(begin_ch_idx) << END_LOG;
                                        const uint32_t end_ch_idx = m_stack_data.m_sent_data[end_word_idx][end_word_idx].m_end_ch_idx;
                                        LOG_DEBUG1 << "m_stack_data.m_sent_data[" << to_string(end_word_idx) << "]["
                                                << to_string(end_word_idx) << "].m_begin_ch_idx = " << to_string(end_ch_idx) << END_LOG;

                                        //Add the source phrase to the target
                                        if (is_lattice) {
                                            storage = m_stack_data.m_source_sent.substr(begin_ch_idx, end_ch_idx - begin_ch_idx);
                                        } else {
                                            storage += m_stack_data.m_source_sent.substr(begin_ch_idx, end_ch_idx - begin_ch_idx);
                                        }
                                    } else {
                                        //If this is a known translation then add the translation text
                                        if (is_lattice) {
                                            storage = m_target->get_target_phrase();
                                        } else {
                                            storage += m_target->get_target_phrase();
                                        }
                                    }

                                    //Add the space after the new phrase, if needed
                                    if (!is_lattice) {
                                        storage += UTF8_SPACE_STRING;
                                    }
                                } else {
                                    if (is_lattice) {
                                        //Check if this is the first state, with a null target 
                                        if (m_s_begin_word_idx == UNDEFINED_WORD_IDX) {
                                            storage = "<s>";
                                        } else {
                                            storage = "</s>";
                                        }
                                    }
                                }

                                //Temporary error logging
                                if (is_lattice && (trim(storage) == "")) {
                                    LOG_ERROR << "Empty translation string, target: " <<
                                            (m_target != NULL ?
                                            (m_target->is_unk_trans() ? string("UNK[") +
                                            to_string(m_stack_data.m_sent_data[m_s_begin_word_idx][m_s_begin_word_idx].m_begin_ch_idx)
                                            + string(",") +
                                            to_string(m_stack_data.m_sent_data[m_s_end_word_idx][m_s_end_word_idx].m_end_ch_idx) +
                                            string("], text: '") + m_stack_data.m_source_sent + string("'") :
                                            m_target->get_target_phrase())
                                            : "NULL") << END_LOG;
                                }

                                LOG_DEBUG1 << "The END storage value is: ___" << storage << "___" << END_LOG;
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

                            //Stores the logarithmic total score of the current hypothesis
                            //The total score is the sum of the current partial score and
                            //the future cost estimate for the given hypothesis state
                            const prob_weight m_total_score;

#if IS_SERVER_TUNING_MODE
                            //An array of lattice scores for the case of server tuning
                            prob_weight * const m_lattice_scores;
#endif

                        private:

                            /**
                             * Allows to retrieve the language model probability for the given query, to
                             * do that we need to extract the query from the current translation frame
                             */
                            inline prob_weight get_lm_cost() {
                                //The number of new words that came into translation is either the
                                //number of words in the target or one, for the <s> or </s> tags
                                const size_t num_new_words = ((m_target != NULL) ? m_target->get_num_words() : 1);

                                //Do the sanity check
                                ASSERT_SANITY_THROW(((MAX_HISTORY_LENGTH + num_new_words) > MAX_M_GRAM_QUERY_LENGTH),
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
                                return m_stack_data.m_lm_query.execute(
                                        num_query_words, query_word_ids,
                                        m_begin_lm_level, PASS_TUNING_FEATURES_MAP);
                            }

                            /**
                             * Allows to compute the linear distortion penalty.
                             * @param prev_state_data the previous state
                             * @return the linear distortion penalty value
                             */
                            inline prob_weight get_lin_dist_cost(const state_data_templ & prev_state_data) {
                                int32_t distance = abs(m_s_begin_word_idx - prev_state_data.m_s_end_word_idx - 1);
                                if (is_dist && (distance > m_stack_data.m_params.m_dist_limit)) {
                                    distance = m_stack_data.m_params.m_dist_limit;
                                }

                                //Store the lin dist cost feature value without the lambda, so just the distance
                                ADD_TUNING_FEATURE_SCORE(de_parameters::DE_LD_PENALTY_GLOBAL_ID, distance);

                                return m_stack_data.m_params.m_lin_dist_penalty * distance;
                            }

                            /**
                             * Allows to compute the lexicolized reordering penalty
                             * @param prev_state_data the previous state
                             * @return the lexicolized distortion penalty 
                             */
                            inline prob_weight get_lex_rm_cost(const state_data_templ & prev_state_data) {
                                const reordering_orientation orient =
                                        rm_entry::get_reordering_orientation(
                                        prev_state_data.m_s_begin_word_idx,
                                        prev_state_data.m_s_end_word_idx,
                                        m_s_begin_word_idx, m_s_end_word_idx);
                                //Compute the reordering costs
                                return prev_state_data.template get_weight<true>(orient, PASS_TUNING_FEATURES_MAP) +
                                        this->template get_weight<false>(orient, PASS_TUNING_FEATURES_MAP);
                            }

                            /**
                             * Allows to get the weight for the given distortion value
                             * @param is_from the flag allowing to distinguish between the from and to case 
                             * if true then we get the value from the from source phrase case
                             * if false then we get the value for the to source phrase case
                             * @param orient the reordering orientation
                             * @return the weight for the given distortion value
                             */
                            template<bool is_from>
                            inline const prob_weight get_weight(const reordering_orientation orient, prob_weight * scores = NULL) const {
                                return rm_entry_data.template get_weight<is_from>(orient, scores);
                            }

                            /**
                             * Allows to obtain the translation model probability
                             * @return the translation model probability for the chosen phrase translation
                             */
                            inline prob_weight get_tm_cost() {
                                //Get the translation model costs
                                return m_target->get_total_weight(PASS_TUNING_FEATURES_MAP);
                            }

                            /**
                             * Allows to obtain the word penalty costs
                             * @return the word penalty costs
                             */
                            inline prob_weight get_word_cost() {
                                //Store the word penalty feature value without the lambda
                                //It will be the number of words with the negative sign
                                ADD_TUNING_FEATURE_SCORE(de_parameters::DE_WORD_PENALTY_GLOBAL_ID, m_target->get_num_words());

                                return m_stack_data.m_params.m_word_penalty * m_target->get_num_words();
                            }

                            /**
                             * For the end translation hypothesis state is supposed to compute the partial and total score
                             * @param prev_state_data the previous state data
                             */
                            inline void compute_partial_score_final(const state_data_templ & prev_state_data) {
                                //After the construction the partial score is to stay fixed,
                                //thus it is declared as constant and here we do a const_cast
                                prob_weight & partial_score = const_cast<prob_weight &> (m_partial_score);

                                LOG_DEBUG1 << "Initial end-state score is: " << partial_score << END_LOG;

                                //Add the language model probability
                                partial_score += get_lm_cost();

                                LOG_DEBUG1 << "end-state score + LM is: " << partial_score << END_LOG;

                                //Add the distance based reordering penalty
                                partial_score += get_lin_dist_cost(prev_state_data);

                                LOG_DEBUG1 << "end-state score + RM discrete is: " << partial_score << END_LOG;

                                //Add the lexicolized reordering costs
                                partial_score += get_lex_rm_cost(prev_state_data);

                                LOG_DEBUG1 << "end-state score + RM lexicolized is: " << partial_score << END_LOG;
                            }

                            /**
                             * Allows to update the currently stored partial score from the
                             * parent state with the partial score of the current hypothesis
                             * @param prev_state_data the previous strate data
                             */
                            inline void compute_partial_score(const state_data_templ & prev_state_data) {
                                //After the construction the partial score is to stay fixed,
                                //thus it is declared as constant and here we do a const_cast
                                prob_weight & partial_score = const_cast<prob_weight &> (m_partial_score);

                                LOG_DEBUG1 << "Initial partial score is: " << partial_score << END_LOG;

                                //Add the phrase translation probability
                                partial_score += get_tm_cost();

                                LOG_DEBUG1 << "partial score + TM is: " << partial_score << END_LOG;

                                //Add the language model probability
                                partial_score += get_lm_cost();

                                LOG_DEBUG1 << "partial score + LM is: " << partial_score << END_LOG;

                                //Add the word penalty
                                partial_score += get_word_cost();

                                LOG_DEBUG1 << "partial score + word penalty is: " << partial_score << END_LOG;

                                //Add the distance based reordering penalty
                                partial_score += get_lin_dist_cost(prev_state_data);

                                LOG_DEBUG1 << "partial score + RM discrete is: " << partial_score << END_LOG;

                                //Add the lexicolized reordering costs
                                partial_score += get_lex_rm_cost(prev_state_data);

                                LOG_DEBUG1 << "partial score + RM lexicolized is: " << partial_score << END_LOG;
                            }

                            /**
                             * Allows to compute the total score of the current hypothesis
                             * I.e. the partial score plus the future cost estimate. The
                             * future cost estimate is not used in the search lattice used
                             * for tuning.
                             */
                            inline void compute_total_score_final() {
                                //After the construction the total score is to stay fixed,
                                //thus it is declared as constant and here we do a const_cast
                                prob_weight & total_score = const_cast<prob_weight &> (m_total_score);

                                //Set the total score to be equal to the partial score as the translation is finished
                                total_score = m_partial_score;

                                LOG_DEBUG1 << "Final end-state score: " << total_score << END_LOG;
                            }

                            /**
                             * Allows to compute the total score of the current hypothesis
                             * I.e. the partial score plus the future cost estimate. The
                             * future cost estimate is not used in the search lattice used
                             * for tuning.
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

                        template<bool is_dist, size_t NUM_WORDS_PER_SENTENCE, size_t MAX_HISTORY_LENGTH, size_t MAX_M_GRAM_QUERY_LENGTH>
                        constexpr int32_t state_data_templ<is_dist, NUM_WORDS_PER_SENTENCE, MAX_HISTORY_LENGTH, MAX_M_GRAM_QUERY_LENGTH>::UNDEFINED_WORD_IDX;

                        template<bool is_dist, size_t NUM_WORDS_PER_SENTENCE, size_t MAX_HISTORY_LENGTH, size_t MAX_M_GRAM_QUERY_LENGTH>
                        constexpr int32_t state_data_templ<is_dist, NUM_WORDS_PER_SENTENCE, MAX_HISTORY_LENGTH, MAX_M_GRAM_QUERY_LENGTH>::ZERRO_WORD_IDX;
                    }
                }
            }
        }
    }
}

#endif /* STATE_DATA_HPP */

