/* 
 * File:   multi_level.hpp
 * Author: zapreevis
 *
 * Created on February 17, 2016, 4:41 PM
 */

#ifndef STACK_LEVEL_HPP
#define STACK_LEVEL_HPP

#include <string>

#include "common/utils/threads.hpp"
#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"

#include "server/decoder/de_configs.hpp"
#include "server/decoder/de_parameters.hpp"

#include "server/decoder/stack/stack_state.hpp"

using namespace std;

using namespace uva::utils::threads;
using namespace uva::utils::logging;
using namespace uva::utils::exceptions;

using namespace uva::smt::bpbd::server::decoder;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace decoder {
                    namespace stack {

                        //Forward declaration of the stack level class
                        class stack_level;
                        //The typedef of the stack level pointer
                        typedef stack_level * stack_level_ptr;

                        /**
                         * Represents the multi-stack level
                         */
                        class stack_level {
                        public:

                            /**
                             * The basic constructor
                             * @param params the decoder parameters, stores the reference to it
                             * @param is_stop the stop flag
                             */
                            stack_level(const de_parameters & params, acr_bool_flag is_stop)
                            : m_params(params), m_is_stop(is_stop), m_first_state(NULL), m_last_state(NULL), m_size(0) {
                                LOG_DEBUG2 << "stack_level create, with parameters: " << m_params << END_LOG;
                            }

                            /**
                             * The basic destructor, this implementation is iterative.
                             */
                            ~stack_level() {
                                //Delete the states one by one
                                stack_state_ptr curr_state = m_first_state;
                                stack_state_ptr next_state = NULL;
                                while (curr_state != NULL) {
                                    //Store the next state
                                    next_state = curr_state->get_next_in_level();
                                    //Delete the current state
                                    delete curr_state;
                                    //Set the next state as the current one
                                    curr_state = next_state;
                                }
                            }

                            /**
                             * Allows to add a new state into the level
                             * @param new_state the new state to add
                             */
                            void add_state(stack_state_ptr new_state) {
                                LOG_DEBUG1 << "Adding a new state (" << new_state << ") to the "
                                        << "level with " << m_size << " states." << END_LOG;

                                //If there is no states in the level yet, then set this one as the first
                                if (m_first_state == NULL) {
                                    m_first_state = new_state;
                                    m_last_state = new_state;
                                    ++m_size;

                                    LOG_DEBUG1 << "Setting (" << m_first_state << "/" << m_last_state
                                            << ") as the first/last in the level!" << END_LOG;
                                } else {
                                    //Currently just add the new state to the end

                                    LOG_DEBUG1 << "Setting (" << new_state
                                            << ") as the last in the level!" << END_LOG;

                                    //Set the new state after the last one
                                    m_last_state->set_next_in_level(new_state);

                                    //Set the new state to be the last one.
                                    m_last_state = new_state;

                                    //Check if the stack capacity is exceeded
                                    if (m_size < m_params.m_stack_capacity) {
                                        LOG_DEBUG1 << "The stack size does not exceed its capacity "
                                                << m_params.m_stack_capacity << ", incrementing the "
                                                << "stack size" << END_LOG;

                                        //If not then just increment the count
                                        ++m_size;
                                    } else {
                                        LOG_DEBUG1 << "The stack exceeds its capacity: "
                                                << m_params.m_stack_capacity
                                                << " deleting the first state " << END_LOG;

                                        //If exceeded then remove the first state
                                        //This is just for now to keep things rolling.
                                        stack_state_ptr tmp = m_first_state;
                                        m_first_state = m_first_state->get_next_in_level();
                                        delete tmp;
                                    }
                                }

                                LOG_DEBUG1 << "The new number of level states: " << m_size << END_LOG;
                            }

                            /*
                                //Initialize the reference to the pointer to the
                                //state place where we should put the new one
                                stack_state_ptr & place_state = m_first_state;

                                //ToDo: Search through the list for two things:
                                // 1. The equal hypothesis == - for recombination
                                // 2. The order position < for cost ordering.
                                //In case the equal hypothesis is not found and
                                //the recombination is not done, then insert the
                                //hypothesis to the found position based on cost
                                //comparison.
                                THROW_NOT_IMPLEMENTED();

                                //Search for the proper position of the state,
                                //or until we find an empty position, then stop.
                                while ((place_state != NULL) && (*new_state < *place_state)) {
                                    //Move further to the next state
                                    place_state = place_state->get_next_in_level();
                                }

                                //ToDo: How can we combine it with recombination?
                                //We re-combine two hypothesis if:
                                //1. They cover the same words
                                //2. They have the same last translated source word
                                //3. They have the same history: last n-1 target words
                                THROW_NOT_IMPLEMENTED();

                                //Check if the found free
                                if (place_state == NULL) {
                                    //Assign the state to the found position
                                    place_state = new_state;
                                } else {
                                    //If the place state is not NULL then the new
                                    //state is >= that the place state, so insert
                                    //the new state before this one
                                }
                             */

                            /**
                             * Allows to expand the stack elements, to do that this method just
                             * goes through all the stack elements one by one and expands them.
                             * We could have done this recursively but this way we avoid stack
                             * allocations so we might be just faster.
                             */
                            void expand() {
                                //Get the pointer to the first state
                                stack_state_ptr curr_state = m_first_state;

                                //Iterate while we do not need to stop or we reach the end of the stack
                                while (!m_is_stop && (curr_state != NULL)) {
                                    LOG_DEBUG << ">>>>> Start STATE (" << curr_state << ") expansion" << END_LOG;

                                    //Allow the state to expand itself
                                    curr_state->expand();

                                    LOG_DEBUG << "<<<<< End STATE (" << curr_state << ") expansion" << END_LOG;

                                    //Move to the next state
                                    curr_state = curr_state->get_next_in_level();
                                }
                            }

                            /**
                             * Allows to get the best translation target string for this stack.
                             * To do that, it takes the first element/state in the level's ordered 
                             * by costs stack and asks it to unroll itself to give its translation.
                             * @param target_sent [out] the variable to store the translation
                             */
                            void get_best_trans(string & target_sent) const {
                                //Assert that there is something in the stack, if not
                                //then it is possible we could not get any translation due
                                //to an unexpected situation in the algorithms
                                ASSERT_CONDITION_THROW((m_first_state == NULL),
                                        "The translation process failed, not translations!");

                                //Call the get-translation function of the most probable state in the stack
                                m_first_state->get_translation(target_sent);
                            }

                        protected:

                        private:
                            //Stores the reference to the decoder parameters
                            const de_parameters & m_params;

                            //Stores the stopping flag
                            acr_bool_flag m_is_stop;

                            //Stores the pointer to the first level state
                            stack_state_ptr m_first_state;

                            //Stores the pointer to the last level state
                            stack_state_ptr m_last_state;

                            //Stores the stack size, i.e. the number
                            //of elements stored inside the stack
                            size_t m_size;
                        };
                    }
                }
            }
        }
    }
}

#endif /* MULTI_LEVEL_HPP */

