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
#include "common/utils/text/string_utils.hpp"

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

                        //Forward declaration of the stack level to be used as a state friend
                        template<bool is_dist, size_t NUM_WORDS_PER_SENTENCE, size_t MAX_HISTORY_LENGTH, size_t MAX_M_GRAM_QUERY_LENGTH>
                        class stack_level_templ;

                        /**
                         * This is the translation stack state class that is responsible for the sentence translation
                         * @param is_dist the flag indicating whether there is a left distortion limit or not
                         * @param NUM_WORDS_PER_SENTENCE the maximum allowed number of words per sentence
                         * @param MAX_HISTORY_LENGTH the maximum allowed length of the target translation hystory
                         * @param MAX_M_GRAM_QUERY_LENGTH the maximum length of the m-gram query
                         */
                        template<bool is_dist, size_t NUM_WORDS_PER_SENTENCE, size_t MAX_HISTORY_LENGTH, size_t MAX_M_GRAM_QUERY_LENGTH>
                        class stack_state_templ {
                        public:
                            //Typedef the state data template for a shorter name
                            typedef state_data_templ<is_dist, NUM_WORDS_PER_SENTENCE, MAX_HISTORY_LENGTH, MAX_M_GRAM_QUERY_LENGTH> state_data;
                            //Typedef the stack data
                            typedef typename state_data::stack_data stack_data;
                            //Typedef the state pointer
                            typedef typename stack_data::stack_state_ptr stack_state_ptr;
                            //Typedef the state pointer
                            typedef typename stack_data::const_stack_state_ptr const_stack_state_ptr;
                            //Typedef the state
                            typedef typename stack_data::stack_state stack_state;

                            //Stores the undefined and initial ids for the state 
                            static constexpr int32_t UNDEFINED_STATE_ID = -1;
                            static constexpr int32_t INITIAL_STATE_ID = 0;

#if IS_SERVER_TUNING_MODE
#define INIT_STACK_STATE_TUNING_DATA , m_state_id(UNDEFINED_STATE_ID), m_is_not_dumped(true)
#else
#define INIT_STACK_STATE_TUNING_DATA
#endif                        

                            /**
                             * The basic constructor for the BEGIN stack state
                             * @param data the shared data container
                             */
                            stack_state_templ(const stack_data & data)
                            : m_parent(NULL), m_state_data(data),
                            m_prev(NULL), m_next(NULL), m_recomb_from(NULL) INIT_STACK_STATE_TUNING_DATA{
                                LOG_DEBUG1 << "New BEGIN state: " << this << ", parent: " << m_parent << END_LOG;
                            }

                            /**
                             * The basic constructor for the END stack state
                             * @param parent the parent state pointer, NOT NULL!
                             * @param prev_history the previous translation history
                             */
                            stack_state_templ(stack_state_ptr parent) :
                            m_parent(parent), m_state_data(parent->m_state_data),
                            m_prev(NULL), m_next(NULL), m_recomb_from(NULL) INIT_STACK_STATE_TUNING_DATA{
                                LOG_DEBUG1 << "New END state: " << this << ", parent: " << m_parent << END_LOG;
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
                            m_prev(NULL), m_next(NULL), m_recomb_from(NULL) INIT_STACK_STATE_TUNING_DATA{
                                LOG_DEBUG1 << "New state: " << this << ", parent: " << m_parent
                                << ", source[" << begin_pos << "," << end_pos << "], target ___"
                                << target->get_target_phrase() << "___" << END_LOG;
                            }

                            /**
                             * The basic destructor, should free all the allocated
                             * resources. Deletes the states that are recombined
                             * into this state as they are not in any stack level
                             */
                            ~stack_state_templ() {
                                //We need two pointers one to point to the current state to
                                //be deleted and another one for the next state to move to.
                                stack_state_ptr curr_state = m_recomb_from;
                                stack_state_ptr next_state = NULL;

                                LOG_DEBUG1 << "Destructing state " << this << ", recombined-"
                                        << "from states ptr: " << curr_state << END_LOG;

                                //While there is states to delete
                                while (curr_state != NULL) {
                                    //Save the next state
                                    next_state = curr_state->m_next;

                                    LOG_DEBUG1 << "Deleting recombined from state: " << curr_state << END_LOG;

                                    //Delete the revious state
                                    delete curr_state;

                                    //Move on to the next state;
                                    curr_state = next_state;
                                }
                            }

#if IS_SERVER_TUNING_MODE

                            /**
                             * Allows to set the state id for the case of decoder
                             * tuning, i.e. search lattice generation.
                             * @param state_id the state id as issued by the stack
                             */
                            inline void set_state_id(const int32_t & state_id) {
                                //Check the sanity, that the id is not set yet
                                ASSERT_SANITY_THROW((m_state_id != UNDEFINED_STATE_ID),
                                        string("Re-initializing state id, old value: ") +
                                        to_string(m_state_id) + string(" new value: ") +
                                        to_string(state_id));

                                //Set the new state id
                                m_state_id = state_id;
                            }

                            /**
                             * Allows to dump the end level states as if they were the
                             * from states for the super-end state of the lattice
                             * @param this_dump the stream to dump the from state into
                             */
                            inline void dump_to_from_se_state_data(ostream & this_dump) const {
                                LOG_DEBUG1 << "Dumping the SE FROM state " << this << " ("
                                        << m_state_id << ") to the search lattice" << END_LOG;

                                //Dump the data into the lattice
                                this_dump << to_string(m_state_id) << "||||||0";

                                LOG_DEBUG1 << "Dumping the SE FROM state " << this << " ("
                                        << m_state_id << ") to the lattice is done" << END_LOG;
                            }

                            /**
                             * Allows to dump the stack state data to the lattice
                             * @param this_dump the lattice output stream to dump the data into
                             * @param covers_dump the dump stream to dump the to-from covers
                             * @param to_state the to state
                             * @param main_to_state the main to state, i.e. in case the to_state
                             * was recombined into another state, we shall use that as a main
                             * state-carrying the state id for the lattice
                             */
                            inline void dump_to_from_state_data(ostream & this_dump, ostream & covers_dump,
                                    const stack_state & to_state, const stack_state & main_to_state) const {
                                LOG_DEBUG1 << "Dumping the FROM state " << this << " ("
                                        << m_state_id << ") to the search lattice" << END_LOG;

                                //Extract the target translation string.
                                string target = "";
                                to_state.m_state_data.template get_target_phrase<true>(target);

                                //Dump the data into the lattice
                                this_dump << to_string(m_state_id) << "|||" << target << "|||"
                                        << to_string(to_state.m_state_data.m_partial_score - m_state_data.m_partial_score);

                                //Dump the cover vector for the to state
                                covers_dump << to_string(main_to_state.m_state_id) << "-" << to_string(m_state_id)
                                        << ":" << to_string(to_state.m_state_data.m_s_begin_word_idx)
                                        << ":" << to_string(to_state.m_state_data.m_s_end_word_idx) << " ";

                                LOG_DEBUG1 << "Dumping the FROM state " << this << " ("
                                        << m_state_id << ") to the lattice is done" << END_LOG;
                            }

                            /**
                             * Allows to dump the stack state data to the lattice as a TO state
                             * @param this_dump the stream where the current state is to be dumped right away
                             * @param scores_dump the stream for dumping the used model features per state
                             * @param covers_dump the stream for dumping the cover vectors
                             */
                            inline void dump_to_state_data(ostream & this_dump, ostream & scores_dump, ostream & covers_dump) const {
                                LOG_DEBUG1 << "Dumping the TO state " << this << " ("
                                        << m_state_id << ") to the search lattice" << END_LOG;
                                
                                //Assert sanity that the only state with no parent is the rood one with the zero id.
                                ASSERT_SANITY_THROW((m_parent != NULL)&&(m_state_id == INITIAL_STATE_ID),
                                        string("The parent is present but the root state id is ") + to_string(INITIAL_STATE_ID));
                                ASSERT_SANITY_THROW((m_parent == NULL)&&(m_state_id != INITIAL_STATE_ID),
                                        string("The parent is NOT present but the root state id is NOT ") + to_string(INITIAL_STATE_ID));

                                //If the state does not have a parent then it is the
                                //root of translation tree, so no need to dump it
                                if ((m_parent != NULL) && m_is_not_dumped) {
                                    //Dump the state info
                                    this_dump << to_string(m_state_id) << "\t";

                                    //Dump the scores
                                    dump_state_scores(scores_dump);

                                    //Declare the stream to store the parent's data
                                    stringstream parents_dump;

                                    //Dump the state's parent as its from state 
                                    m_parent->dump_to_from_state_data(this_dump, covers_dump, *this, *this);
                                    //Dump as a to state into the parent dump
                                    m_parent->dump_to_state_data(parents_dump, scores_dump, covers_dump);

                                    //Dump the parents of the recombined from states, if any
                                    stack_state_ptr rec_from = m_recomb_from;
                                    while (rec_from != NULL) {
                                        //Add an extra space before the new element in the lattice dump
                                        this_dump << " ";
                                        //Dump as a from state
                                        rec_from->m_parent->dump_to_from_state_data(this_dump, covers_dump, *rec_from, *this);
                                        //Dump as a to state into the parent dump
                                        rec_from->m_parent->dump_to_state_data(parents_dump, scores_dump, covers_dump);
                                        //Move to the next recombined from state
                                        rec_from = rec_from->m_next;
                                    }

                                    //Finish this entry and add the parents
                                    this_dump << std::endl << parents_dump.str();

                                    //Mark the state as dumped 
                                    const_cast<bool &> (m_is_not_dumped) = false;
                                }

                                LOG_DEBUG1 << "Dumping the TO state " << this << " ("
                                        << m_state_id << ") to the lattice is done" << END_LOG;
                             }
#endif

                            /**
                             * Allows to get the stack level, the latter is equal
                             * to the number of so far translated words.
                             * @return the stack level
                             */
                            inline uint32_t get_stack_level() const {
                                //Get the stack level as it was computed during the state creation.
                                return m_state_data.m_stack_level;
                            }

                            /**
                             * Allows the state to expand itself, it will
                             * add itself to the proper stack.
                             */
                            inline void expand() {
                                //Create shorthands for the data to compare and log
                                const size_t curr_count = m_state_data.m_covered.count();
                                const size_t & num_words = m_state_data.m_stack_data.m_sent_data.get_dim();

                                LOG_DEBUG << "Num words = " << num_words << ", covered: "
                                        << m_state_data.covered_to_string() << ", curr_count = "
                                        << curr_count << END_LOG;

                                //Check if this is the last state, i.e. we translated everything
                                if (curr_count == num_words) {
                                    //All of the words have been translated, add the end state
                                    stack_state_ptr end_state = new stack_state(this);
                                    m_state_data.m_stack_data.m_add_state(end_state);
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

                                //Allows to add the target phrase to the given string
                                m_state_data.get_target_phrase(target_sent);
                            }

                            /**
                             * Allows to compare two states, the comparison is based on the state total score.
                             * The state with the bigger total score is considered to be bigger, i.e. more probable.
                             * The state with the smalle total score is considered to be smaller, i.e. less probable.
                             * @param other the other state to compare with
                             * @return true if this state is smaller than the other one
                             */
                            inline bool operator<(const stack_state & other) const {
                                //Get the shorthand for the other state data
                                const state_data & other_data = other.m_state_data;

                                LOG_DEBUG3 << "Checking states: " << this << " <? " << &other << END_LOG;
                                LOG_DEBUG3 << "Total scores: " << m_state_data.m_total_score << " <? "
                                        << other_data.m_total_score << END_LOG;

                                //Compute the comparison result
                                const bool is_less = (m_state_data.m_total_score < other_data.m_total_score);

                                //Log the comparison result
                                LOG_DEBUG3 << "Result, state: " << this << (is_less ? " < " : " >= ") << &other << END_LOG;

                                //Return the comparison result
                                return is_less;
                            }

                            /**
                             * Allows to compare two states, the states are equal if the solve
                             * the same sub-problem i.e. are eligible for recombination.
                             * The states are equal if and only if:
                             *    1. They have the same last translated word
                             *    2. They have the same history of target words
                             *    3. They cover the same source words
                             * @param other the other state to compare with
                             * @return true if this state is equal to the other one, otherwise false.
                             */
                            inline bool operator==(const stack_state & other) const {
                                //Get the shorthand for the other state data
                                const state_data & other_data = other.m_state_data;

                                LOG_DEBUG3 << "Checking state " << this << " ==? " << &other << END_LOG;
                                LOG_DEBUG3 << m_state_data.m_trans_frame.tail_to_string(MAX_HISTORY_LENGTH) << " ==? "
                                        << other_data.m_trans_frame.tail_to_string(MAX_HISTORY_LENGTH) << END_LOG;
                                LOG_DEBUG3 << m_state_data.covered_to_string() << " ==? "
                                        << other_data.covered_to_string() << END_LOG;

                                //Compute the comparison result
                                const bool is_equal = (m_state_data.m_s_end_word_idx == other_data.m_s_end_word_idx) &&
                                        m_state_data.m_trans_frame.is_equal_last(other_data.m_trans_frame, MAX_HISTORY_LENGTH) &&
                                        (m_state_data.m_covered == other_data.m_covered) &&
                                        m_state_data.rm_entry_data.is_equal_from_weights(other_data.rm_entry_data);

                                //Log the comparison result
                                LOG_DEBUG3 << "Result, state: " << this << (is_equal ? " == " : " != ") << &other << END_LOG;

                                //Return the comparison result
                                return is_equal;
                            }

                            /**
                             * Allows to compare two states for not being equal,
                             * this is an inverse of the == operator.
                             * @param other the other state to compare with
                             * @return true if this state is not equal to the other one, otherwise false.
                             */
                            inline bool operator!=(const stack_state & other) const {
                                return !(*this == other);
                            }

                            /**
                             * Allows to check if the given new state is within the
                             * threshold limit.
                             * @param score_bound the bound to compare with
                             * @return true if the state's totl cost is >= score_bound, otherwise false
                             */
                            inline bool is_above_threshold(const prob_weight & score_bound) const {
                                LOG_DEBUG1 << "checking state " << this << " score: " << m_state_data.m_total_score
                                        << ", threshold: " << score_bound << END_LOG;
                                return ( m_state_data.m_total_score >= score_bound);
                            }

                            /**
                             * Allows to add a state recombined into this one,
                             * i.e. the one equivalent to this one but having
                             * the lower value of the total cost. In case this state
                             * already has too many states recombined into this one
                             * and the new state probability is lower than that of
                             * the others, then we just delete it. Also, if there
                             * were states recombined into the other one, then they
                             * have lower costs, so proper merging of them is to
                             * be done as well. Eventually the states recombined into
                             * this one must have their m_recomb_from arrays empty.
                             * @param new_state the state to recombine into this one.
                             */
                            inline void recombine_from(stack_state_ptr other_state) {
                                LOG_DEBUG2 << "====================================================================" << END_LOG;
                                LOG_DEBUG1 << "Recombining " << other_state << " into " << this << END_LOG;

                                //Combine the new state with its recombined from states
                                //Put the state and its recombine from list together
                                other_state->m_prev = NULL;
                                other_state->m_next = other_state->m_recomb_from;
                                LOG_DEBUG << "State " << other_state << " recombined from "
                                        << "ptr = " << other_state->m_recomb_from << END_LOG;
                                //If the other's state recombine from list is not empty then link-back the list
                                if (other_state->m_next != NULL) {
                                    other_state->m_next->m_prev = other_state;
                                    //Clean the recombined from data
                                    other_state->m_recomb_from = NULL;
                                }

                                //Check if this is the first state we recombine into the current one
                                if (m_recomb_from == NULL) {
                                    LOG_DEBUG << "State " << other_state << " is the first"
                                            << " state recombined into " << this << END_LOG;
                                    //If this state has no recombined-from states
                                    //then just set the other list into this state
                                    m_recomb_from = other_state;
                                } else {
                                    LOG_DEBUG << "State " << this << " already has a "
                                            << " recombined from state " << m_recomb_from << END_LOG;
                                    //If this state already has some states recombined into this one then
                                    //we need to append the two lists together, the order is not important
                                    //We assume that the other state we are recombining into this one has
                                    //fewer recombined-from states, so we put it as a first one

                                    //First skip worward untill the last one
                                    stack_state_ptr cursor = other_state;
                                    while (cursor->m_next != NULL) {
                                        cursor = cursor->m_next;
                                    }

                                    //Set the current recombined from as the last one
                                    cursor->m_next = m_recomb_from;
                                    m_recomb_from->m_prev = cursor;

                                    //Set the other state as the first in the list
                                    m_recomb_from = other_state;
                                }

                                LOG_DEBUG1 << "Recombining " << other_state << " into " << this << " is done!" << END_LOG;
                                LOG_DEBUG2 << "====================================================================" << END_LOG;
                            }

                        protected:

                            /**
                             * Allows to cut the tail of states starting from this one.
                             * The states present in the cut tail are to be deleted.
                             * @param tail the tails of staits to delete
                             */
                            inline void cut_the_tail(stack_state_ptr tail) {
                                //If the tail is not empty
                                if (tail != NULL) {
                                    //If the tail has a preceeding state
                                    if (tail->m_prev != NULL) {
                                        //Cut from the preceeding state
                                        tail->m_prev->m_next = NULL;
                                    }

                                    //Iterate through the states and delete them
                                    stack_state_ptr next = NULL;
                                    while (tail != NULL) {
                                        //Store the next element
                                        next = tail->m_next;
                                        //Delete the tail first element
                                        delete tail;
                                        //Move to the next tail element
                                        tail = next;
                                    }
                                }
                            }

                            /**
                             * Allows to expand the lengths if not the word given
                             * by the current position is not covered.
                             * @param curr_pos the reference to the current position
                             * @param num_exp the reference to the number of positions
                             * we could expand from will be incremented by this method
                             * by one if an expansion is possible.
                             */
                            inline void expand_length_if_not_covered(const size_t curr_pos, size_t & num_exp) {
                                LOG_DEBUG << "Checking the coverage vector @ position " << curr_pos << END_LOG;

                                //If the next position is not covered then expand the lengths
                                if (!m_state_data.m_covered[curr_pos]) {
                                    //Expand the lengths
                                    expand_length(curr_pos);

                                    //Count the expansion
                                    ++num_exp;
                                }
                            }

                            /**
                             * Expand to the left of the last phrase, for all the possible of start positions
                             */
                            inline void expand_left() {
                                LOG_DEBUG1 << ">>>>> expand left from [" << m_state_data.m_s_begin_word_idx
                                        << "," << m_state_data.m_s_end_word_idx << "]" << END_LOG;

                                //Store the shorthand to the minimum possible word index
                                const int32_t & MIN_WORD_IDX = m_state_data.m_stack_data.m_sent_data.m_min_idx;

                                //Compute the minimum position to consider, based on distortion
                                int32_t min_pos;
                                if (is_dist) {
                                    //Compute the normal minimum position for distortion
                                    min_pos = (m_state_data.m_s_end_word_idx - m_state_data.m_stack_data.m_params.m_dist_limit);
                                    //Bound the position by the minimum word index
                                    min_pos = max(min_pos, MIN_WORD_IDX);
                                } else {
                                    //There is no distortion limit
                                    min_pos = MIN_WORD_IDX;
                                }

                                //We shall start expanding from the first word before
                                //the beginning of the last translated phrase
                                int32_t curr_pos = (m_state_data.m_s_begin_word_idx - 1);

                                LOG_DEBUG << "start pos = " << curr_pos << ", min pos = " << min_pos << END_LOG;

                                //Store the number of left expansions made
                                size_t num_exp = 0;

                                //Iterate to the left of the last positions until the
                                //position is valid and the distortion is within the limits
                                //If there was nothing reached with the regular distortion.
                                //limit then we iterate until we can cover at least something
                                while ((curr_pos >= min_pos) || ((num_exp == 0)&&(curr_pos >= MIN_WORD_IDX))) {
                                    //Expand the state to the words of the last phrase if not covered
                                    expand_length_if_not_covered(curr_pos, num_exp);
                                    //Decrement the start position
                                    curr_pos--;
                                    //Log the current number of expansions
                                    LOG_DEBUG << "Number of left expansion position: " << num_exp << END_LOG;
                                }

                                LOG_DEBUG1 << "<<<<< expand left" << END_LOG;
                            }

                            /**
                             * Expand to the right of the last phrase, for all the possible of start positions
                             */
                            inline void expand_right() {
                                LOG_DEBUG1 << ">>>>> expand right from [" << m_state_data.m_s_begin_word_idx
                                        << "," << m_state_data.m_s_end_word_idx << "]" << END_LOG;

                                //Store the shorthand to the minimum possible word index
                                const int32_t & MAX_WORD_IDX = m_state_data.m_stack_data.m_sent_data.m_max_idx;

                                //Compute the maximum position to consider, based on distortion
                                int32_t max_pos;
                                if (is_dist) {
                                    //Compute the normal maximum position for distortion
                                    max_pos = m_state_data.m_s_end_word_idx + m_state_data.m_stack_data.m_params.m_dist_limit;
                                    //Bound the position by the maximum word index
                                    max_pos = min(max_pos, MAX_WORD_IDX);
                                } else {
                                    //There is no distortion limit
                                    max_pos = MAX_WORD_IDX;
                                }

                                //We shall start expanding from the first word
                                //after the end of the last translated phrase
                                int32_t curr_pos = (m_state_data.m_s_end_word_idx + 1);

                                LOG_DEBUG << "start pos = " << curr_pos << ", max pos = " << max_pos << END_LOG;

                                //Store the number of right expansions made
                                size_t num_exp = 0;

                                //Iterate to the right of the last positions until the
                                //position is valid and the distortion is within the limits
                                //If there was nothing reached with the regular distortion.
                                //limit then we iterate until we can cover at least something
                                while ((curr_pos <= max_pos) || ((num_exp == 0)&&(curr_pos <= MAX_WORD_IDX))) {
                                    //Expand the state to the words of the last phrase if not covered
                                    expand_length_if_not_covered(curr_pos, num_exp);
                                    //Increment the start position
                                    ++curr_pos;
                                    //Log the current number of expansions
                                    LOG_DEBUG << "Number of right expansion position: " << num_exp << END_LOG;
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

                                //Check if we are in the situation of a single word or in the
                                //situation when we actually have translations for a phrase
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

                            //This variable stores the pointer to the previous state:
                            //1. In the stack level or NULL if it is the first one
                            //2. In the list of recombined from states
                            stack_state_ptr m_prev;

                            //This variable stores the pointer to the next state:
                            //1. In the stack level or NULL if it is the last one
                            //2. In the list of recombined from states
                            stack_state_ptr m_next;

                            //This double-linked list stores the list of states recombined into this state
                            stack_state_ptr m_recomb_from;

#if IS_SERVER_TUNING_MODE
                            //Stores the state id unique within the multi-stack
                            //In case the software is compiled for the tuning mode.
                            int32_t m_state_id;

                            //Stores the flag indicating whether the state
                            //has been dumped to the lattice or not.
                            const bool m_is_not_dumped;

                            /**
                             * Allows to dump the feature scores to the scores output stream
                             * @param scores_dump the output stream for the featrue scores
                             */
                            inline void dump_state_scores(ostream & scores_dump) const {
                                scores_dump << to_string(m_state_id);
                                for (size_t idx = 0; idx < m_state_data.m_stack_data.get_num_features(); ++idx) {
                                    if (m_state_data.m_lattice_scores[idx] != 0.0) {
                                        scores_dump << " " << to_string(idx) << "=" << m_state_data.m_lattice_scores[idx];
                                    }
                                }
                                scores_dump << std::endl;
                            }
#endif

                            //Make the stack level the friend of this class
                            friend class stack_level_templ<is_dist, NUM_WORDS_PER_SENTENCE, MAX_HISTORY_LENGTH, MAX_M_GRAM_QUERY_LENGTH>;
                        };
                    }
                }
            }
        }
    }
}

#endif /* TRANS_STACK_STATE_HPP */

