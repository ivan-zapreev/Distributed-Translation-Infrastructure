/* 
 * File:   trans_stack.hpp
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

#ifndef MULTI_STACK_HPP
#define MULTI_STACK_HPP

#include <string>
#include <functional>

#include "common/utils/threads.hpp"
#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"

#include "server/lm/lm_configurator.hpp"
#include "server/rm/proxy/rm_query_proxy.hpp"

#include "server/decoder/de_configs.hpp"
#include "server/decoder/de_parameters.hpp"

#include "server/decoder/stack/stack_level.hpp"
#include "server/decoder/stack/stack_data.hpp"

using namespace std;
using namespace std::placeholders;

using namespace uva::utils::threads;
using namespace uva::utils::logging;
using namespace uva::utils::exceptions;

using namespace uva::smt::bpbd::server::lm;
using namespace uva::smt::bpbd::server::lm::proxy;
using namespace uva::smt::bpbd::server::rm::proxy;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace decoder {
                    namespace stack {

                        //Stores the number of extrs stack levels that we will need in a multi-
                        //stack, the first one is for <s> and the second one is for </s>
                        static constexpr uint32_t NUM_EXTRA_STACK_LEVELS = 2;

                        //Stores the minimum stack level index
                        static constexpr uint32_t MIN_STACK_LEVEL = 0;

                        /**
                         * This is the translation stack class that is responsible for the sentence translation
                         * @param is_dist the flag indicating whether there is a left distortion limit or not
                         * @param is_alt_trans the flag indicating if the alternative translations are to be stored when recombining states.
                         * @param NUM_WORDS_PER_SENTENCE the maximum allowed number of words per sentence
                         * @param MAX_HISTORY_LENGTH the maximum allowed length of the target translation hystory
                         * @param MAX_M_GRAM_QUERY_LENGTH the maximum length of the m-gram query
                         */
                        template<bool is_dist, bool is_alt_trans, size_t NUM_WORDS_PER_SENTENCE, size_t MAX_HISTORY_LENGTH, size_t MAX_M_GRAM_QUERY_LENGTH>
                        class multi_stack_templ {
                        public:
                            //Give a short name for the stack data
                            typedef stack_data_templ<is_dist, is_alt_trans, NUM_WORDS_PER_SENTENCE, MAX_HISTORY_LENGTH, MAX_M_GRAM_QUERY_LENGTH> stack_data;
                            //Give a short name for the stack level
                            typedef stack_level_templ<is_dist, is_alt_trans, NUM_WORDS_PER_SENTENCE, MAX_HISTORY_LENGTH, MAX_M_GRAM_QUERY_LENGTH> stack_level;
                            //The typedef of the stack level pointer
                            typedef stack_level * stack_level_ptr;
                            //Typedef the state pointer
                            typedef typename stack_data::stack_state_ptr stack_state_ptr;
                            //Typedef the state
                            typedef typename stack_data::stack_state stack_state;

                            /**
                             * The basic constructor
                             * @param params the decoder parameters, stores the reference to it
                             * @param is_stop the stop flag
                             * @param source_sent the reference to the source sentence
                             * @param sent_data the retrieved sentence data
                             * @param rm_query the reordering model query
                             * @param lm_query the language model query object
                             */
                            multi_stack_templ(const de_parameters & params,
                                    acr_bool_flag is_stop,
                                    const string & source_sent,
                                    const sentence_data_map & sent_data,
                                    const rm_query_proxy & rm_query,
                                    lm_fast_query_proxy & lm_query)
                            : m_data(params, is_stop, source_sent, sent_data, rm_query, lm_query, bind(&multi_stack_templ::add_stack_state, this, _1)),
                            m_num_levels(m_data.m_sent_data.get_dim() + NUM_EXTRA_STACK_LEVELS) {
                                LOG_DEBUG1 << "Created a multi stack with parameters: " << m_data.m_params << END_LOG;

                                LOG_DEBUG2 << "Creating a stack levels array of " << m_num_levels << " elements." << END_LOG;

                                //Instantiate an array of stack level pointers
                                m_levels = new stack_level_ptr[m_num_levels]();

                                LOG_DEBUG2 << "The minimum stack level is " << MIN_STACK_LEVEL << END_LOG;

                                //Initialize the stack levels
                                for (uint32_t level = MIN_STACK_LEVEL; level < m_num_levels; ++level) {
                                    m_levels[level] = new stack_level(m_data.m_params, m_data.m_is_stop);
                                }

                                //Add the root state to the stack, the root state must have 
                                //information about the sentence data, rm and lm query and 
                                //have a method for adding a state expansion to the stack.
                                LOG_DEBUG2 << "Creating the begin stack state" << END_LOG;
                                stack_state_ptr begin_state = new stack_state(m_data);
                                LOG_DEBUG2 << "Adding the begin stack state to level " << MIN_STACK_LEVEL << END_LOG;
                                add_stack_state(begin_state);

                                LOG_DEBUG2 << "The multi-stack creation is done" << END_LOG;
                            }

                            /**
                             * The basic destructor
                             */
                            ~multi_stack_templ() {
                                LOG_DEBUG1 << "Destructing stack" << this << ", # levels: " << m_num_levels << END_LOG;

                                //Dispose the stacks
                                if (m_levels != NULL) {

                                    //Iterate through the levels and delete them
                                    for (uint32_t level = MIN_STACK_LEVEL; level < m_num_levels; ++level) {
                                        delete m_levels[level];
                                    }

                                    //Delete the levels array itself
                                    delete[] m_levels;
                                    m_levels = NULL;
                                }
                            }

                            /**
                             * Allows to extend the hypothesis, when extending the stack we immediately re-combine
                             */
                            void expand() {
                                //Define the max stack level constant
                                const size_t MAX_STACK_LEVEL = (m_num_levels - 1);
                                //Stores the current stack level index
                                uint32_t curr_level = MIN_STACK_LEVEL;

                                //Iterate the stack levels and expand them one by one 
                                //until the last one or until we are requested to stop
                                //Note: the last stack level is for the end state </s>
                                //it should not be expanded!
                                while (!m_data.m_is_stop && (curr_level < MAX_STACK_LEVEL)) {
                                    LOG_DEBUG << ">>>>> Start LEVEL (" << curr_level << "/ " << MAX_STACK_LEVEL
                                            << ") expansion, #states=" << m_levels[curr_level]->get_size() << END_LOG;

                                    //Here we expand the stack level and then
                                    //increment the current level index variable
                                    m_levels[curr_level]->expand();

                                    LOG_DEBUG << "<<<<< End LEVEL (" << curr_level << "/ " << MAX_STACK_LEVEL
                                            << ") expansion, #states=" << m_levels[curr_level]->get_size() << END_LOG;

                                    //Move to the next level
                                    ++curr_level;
                                }
                            }

                            /**
                             * Allows to get the best translation from the
                             * stack after the decoding has finished.
                             * @param target_sent [out] the variable to store the translation
                             */
                            void get_best_trans(string & target_sent) const {
                                //Define the max stack level constant
                                const size_t MAX_STACK_LEVEL = (m_num_levels - 1);

                                LOG_DEBUG << "Requesting the best translation from level " << MAX_STACK_LEVEL << END_LOG;

                                //Request the last level for the best translation
                                m_levels[MAX_STACK_LEVEL]->get_best_trans(target_sent);

                                LOG_DEBUG << "The best translation from level " << MAX_STACK_LEVEL
                                        << " is ___" << target_sent << "___" << END_LOG;
                            }

                        protected:

                            /**
                             * Allows to add a new stack state into the proper stack level
                             * @param new_state the new stack state, not NULL
                             */
                            void add_stack_state(stack_state_ptr new_state) {
                                LOG_DEBUG2 << "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" << END_LOG;

                                //Perform a NULL pointer sanity check
                                ASSERT_SANITY_THROW((new_state == NULL), "A NULL pointer stack state!");

                                //Get the new state stack level
                                const uint32_t level = new_state->get_stack_level();

                                LOG_DEBUG << "Adding a new state (" << new_state << ") to stack level " << level << END_LOG;

                                //Perform a sanity check that the state is of a proper level
                                ASSERT_SANITY_THROW((level >= m_num_levels),
                                        string("The new stack state stack level is too big: ") +
                                        to_string(level) + string(" the maximum allowed is: ") +
                                        to_string(m_num_levels - 1));

                                //Add the state to the corresponding stack
                                m_levels[level]->add_state(new_state);

                                LOG_DEBUG << "The state (" << new_state << ") is added to level " << level << END_LOG;

                                LOG_DEBUG2 << "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" << END_LOG;
                            }

                        private:
                            //Stores the shared data for the stack and its elements
                            const stack_data m_data;

                            //Stores the number of multi-stack levels
                            const uint32_t m_num_levels;

                            //This is a pointer to the array of stacks, one stack per number of covered words.
                            stack_level_ptr * m_levels;
                        };
                    }
                }
            }
        }
    }
}

#endif /* TRANS_STACK_HPP */

