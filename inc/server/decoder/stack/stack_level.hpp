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
                                    next_state = curr_state->get_next();
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
                                    LOG_DEBUG1 << "Setting (" << new_state
                                            << ") as the first/last in the level!" << END_LOG;

                                    //Insert this state as the first one
                                    insert_as_first(new_state);
                                } else {
                                    LOG_DEBUG1 << "Setting (" << new_state
                                            << ") as the last in the level!" << END_LOG;

                                    //Currently just add the new state to the end
                                    insert_as_first(new_state);
                                }

                                //Now it is time to increment the count
                                ++m_size;

                                //Check if the stack capacity is exceeded
                                if (m_size > m_params.m_stack_capacity) {
                                    LOG_DEBUG1 << "The stack size: " << m_size
                                            << " exceeds its capacity: "
                                            << m_params.m_stack_capacity
                                            << " pushing out the last state " << END_LOG;

                                    //Destroy the last state in the list
                                    destroy(m_last_state);
                                }

                                LOG_DEBUG1 << "The new number of level states: " << m_size << END_LOG;
                            }

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
                                    curr_state = curr_state->get_next();
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

                            /**
                             * Allows to insert the stack state as the first one in the level
                             * @param state the state to insert
                             */
                            inline void insert_as_first(stack_state_ptr state) {
                                //This state will be the first in the level, so the previous is NULL
                                state->m_prev = NULL;
                                //The next state will be the current first state
                                state->m_next = m_first_state;

                                //Check if there was something inside the level
                                if (m_first_state == NULL) {
                                    //If there was no first state then this state is also the last one
                                    m_last_state = state;
                                } else {
                                    //If there was something within the level then the old
                                    //first one should point to this one as to its previous
                                    m_first_state->m_prev = state;
                                }
                            }

                            /**
                             * Allows to insert the stack state as the last one in the level
                             * @param state the state to insert
                             */
                            inline void insert_as_last(stack_state_ptr state) {
                                //This state is the last one in the level so its next should be NULL
                                state->m_next = NULL;
                                //The previous state will be the current last state
                                state->m_prev = m_last_state;

                                //Check if there was something inside the level
                                if (m_last_state == NULL) {
                                    //If there was no last state then this state is also the first one
                                    m_first_state = state;
                                } else {
                                    //If there was something within the level then the old
                                    //last one should point to this one as to its next
                                    m_last_state->m_next = state;
                                }
                            }

                            /**
                             * Allows to insert the stack state in between the given two elements
                             * @param prev the pointer reference to the prev state, NOT NULL
                             * @param next the pointer reference to the next state, NOT NULL
                             * @param state the state to insert,  NUL NULL
                             */
                            inline void insert_between(stack_state_ptr prev, stack_state_ptr next, stack_state_ptr state) {
                                //Store the previous and next states for this one
                                state->m_prev = prev;
                                state->m_next = next;

                                ASSERT_SANITY_THROW((prev == NULL),
                                        string("Bad pointer: prev = NULL!"));
                                ASSERT_SANITY_THROW((next == NULL),
                                        string("Bad pointer: next = NULL!"));
                                ASSERT_SANITY_THROW((state == NULL),
                                        string("Bad pointer: state = NULL!"));

                                //Update the previous and next states of the others
                                prev->m_next = state;
                                next->m_prev = state;
                            }

                            /**
                             * Allows to destroy the given state from the level
                             * @param state the state to insert
                             */
                            inline void destroy(stack_state_ptr state) {
                                if (m_first_state == m_last_state) {
                                    //This is the only element in the list
                                    m_first_state = NULL;
                                    m_last_state = NULL;
                                } else {
                                    //There is more elements in the list
                                    if (state == m_last_state) {
                                        //We are deleting the last element of the list

                                        //The new last element will be the previous of this one
                                        m_last_state = state->m_prev;
                                        //The new last element shall have no next element 
                                        m_last_state->m_next = NULL;
                                    } else {
                                        if (state == m_first_state) {
                                            //We are deleting the first element of the list

                                            //The new first element will be the next of this one
                                            m_first_state = state->m_next;
                                            //The new first element shall have no previous element
                                            m_first_state->m_prev = NULL;
                                        } else {
                                            //We are deleting some intermediate element

                                            //The previous of this shall now point to the next of this as next
                                            state->m_prev->m_next = state->m_next;

                                            //The next of this shall now point to the previous of this as previous
                                            state->m_next->m_prev = state->m_prev;
                                        }
                                    }
                                }

                                //Commit suicide ;D
                                delete this;
                            }

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

