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
                            : m_params(params), m_is_stop(is_stop), m_first_state(NULL),
                            m_last_state(NULL), m_size(0), m_score_bound(0.0) {
                                LOG_DEBUG3 << "stack_level create, with parameters: " << m_params << END_LOG;
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
                                        << "level with " << m_size << " state(s)." << END_LOG;

                                //If there is no states in the level yet, then set this one as the first
                                if (m_first_state == NULL) {
                                    LOG_DEBUG1 << "Setting (" << new_state
                                            << ") as the first/last in the level!" << END_LOG;

                                    //Insert this state as the first one
                                    insert_as_first(new_state);
                                } else {
                                    LOG_DEBUG1 << "Setting (" << new_state
                                            << ") as the last in the level!" << END_LOG;

                                    //Initialize the reference to the pointer to the
                                    //state place where we should put the new one
                                    stack_state_ptr curr_state = m_first_state;

                                    //Search for the proper position of the state,
                                    //or until we find an empty position, then stop.
                                    while ((curr_state != NULL) && (*new_state < *curr_state)) {
                                        //Move further to the next state
                                        curr_state = curr_state->get_next();
                                    }

                                    LOG_DEBUG1 << "The last considered state is: " << curr_state << END_LOG;

                                    //Check if we found a state which is less probable than the new one
                                    if (curr_state != NULL) {
                                        //There is a less probable state
                                        LOG_DEBUG1 << "The new state is to be added before: " << curr_state << END_LOG;

                                        //Insert this state before the 
                                        insert_before(curr_state, new_state);

                                        //Perform pruning techniques
                                        prune_states();
                                    } else {
                                        //All other states are more probable
                                        LOG_DEBUG1 << "The new state is to be added to the end of the level" << END_LOG;

                                        //Check if there is free space left in the level
                                        if (is_space_left() && is_above_threshold(new_state)) {
                                            //If there is free space left and the
                                            //state satisfies the threshold pruning
                                            //then we add it to the list of states
                                            insert_as_last(new_state);
                                        } else {
                                            //There is no place in the existence for this
                                            //poor fellow, destroy it. This is part of
                                            //histogram and threshold pruning method.
                                            delete new_state;
                                        }
                                    }
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
                                        "The translation process failed, no translations!");

                                //Call the get-translation function of the most probable state in the stack
                                m_first_state->get_translation(target_sent);
                            }

                            /**
                             * This method allows to retrieve the number of stack level elements
                             * @return the number of stack level elements.
                             */
                            inline size_t get_size() const {
                                return m_size;
                            }

                        protected:

                            /**
                             * Allows to update the best score, or rather threshold for threshold pruning.
                             */
                            inline void remember_best_score() {
                                ASSERT_SANITY_THROW((m_first_state == NULL), "Bad pointer m_first_state == NULL");
                                //Get the best state score
                                const float best_score = m_first_state->m_state_data.m_total_score;
                                //Compute the score lower bound
                                m_score_bound = best_score * m_params.m_pruning_threshold;

                                LOG_DEBUG1 << "new best state: " << m_first_state
                                        << ", new max score: " << best_score
                                        << ", new bound: " << m_score_bound << END_LOG;
                            }

                            /**
                             * Allows to check if the given new state is within the
                             * threshold limit from the best scoring state. This method
                             * must be called only if there is already at least one
                             * state in the stack level! I.e. m_first_state is not NULL!
                             * @param state the state to be tested for satisfying the pruning threshold,  not NULL
                             * @return true if the state is good to keep, otherwise false
                             */
                            inline bool is_above_threshold(stack_state_ptr state) const {
                                ASSERT_SANITY_THROW((state == NULL), "Bad pointer state == NULL");

                                //Get the new state score
                                const float score = state->m_state_data.m_total_score;

                                LOG_DEBUG1 << "state " << state << " score is: " << score
                                        << ", m_score_bound: " << m_score_bound << END_LOG;

                                return ( score >= m_score_bound);
                            }

                            /**
                             * Allows to check if there is still space left for adding states into the level
                             * If there is no space left then we can still add states but we shall do histogram
                             * pruning afterwards in order to keep the stack size within the capacity limits.
                             * @return true if there is empty space left for adding states
                             */
                            inline bool is_space_left() const {
                                LOG_DEBUG1 << "The current level size is: " << m_size
                                        << ", capacity is: " << m_params.m_stack_capacity << END_LOG;
                                return (m_size < m_params.m_stack_capacity);
                            }

                            /**
                             * This method makes sure there is not too many
                             * elements in the stack, the last ones are removed.
                             * This method decrements the level size counter.
                             */
                            inline void prune_states() {
                                //Check if the stack capacity is exceeded or the last states are not probable
                                //Remove the last state until both conditions are falsified
                                while ((m_size > m_params.m_stack_capacity) || !is_above_threshold(m_last_state)) {
                                    LOG_DEBUG1 << "Pushing out the last state " << m_last_state << END_LOG;

                                    //Destroy the last state in the list
                                    remove_and_destroy(m_last_state);
                                }

                                ASSERT_SANITY_THROW((m_first_state == NULL),
                                        "This should not be happening, we deleted all states!");
                            }

                            /**
                             * Allows to insert the stack state as the first one in the level
                             * This method increments the level size counter. Updates the best
                             * score!
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

                                //Set the new first state
                                m_first_state = state;

                                //Now it is time to increment the count
                                ++m_size;

                                //Remember the best score
                                remember_best_score();
                            }

                            /**
                             * Allows to insert the stack state as the last one in the level
                             * This method increments the level size counter. Updates the best
                             * score!
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

                                    //Remember the best score
                                    remember_best_score();
                                } else {
                                    //If there was something within the level then the old
                                    //last one should point to this one as to its next
                                    m_last_state->m_next = state;
                                }

                                //Set the new last state
                                m_last_state = state;

                                //Now it is time to increment the count
                                ++m_size;
                            }

                            /**
                             * Allows to insert the stack state in between the given two elements
                             * Note that the next and previous states are to be different!
                             * This method increments the level size counter.
                             * @param prev the pointer reference to the prev state, NOT NULL
                             * @param next the pointer reference to the next state, NOT NULL
                             * @param state the state to insert,  NUL NULL
                             */
                            inline void insert_between(stack_state_ptr prev, stack_state_ptr next, stack_state_ptr state) {
                                ASSERT_SANITY_THROW((prev == next),
                                        string("Bad pointers: prev = next!"));

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

                                //Now it is time to increment the count
                                ++m_size;
                            }

                            /**
                             * Allows to insert a new element before the given stack element in the level list
                             * This method increments the level size counter. Updates the best score!
                             * @param curr_state the state before which the new state is to be inserted, not NULL
                             * @param new_state the state to be inserted, NOT NULL
                             */
                            inline void insert_before(stack_state_ptr curr_state, stack_state_ptr new_state) {
                                if (curr_state == m_first_state) {
                                    //If the current state is the first one then insert
                                    //the new state as the first state in the list
                                    insert_as_first(new_state);
                                } else {
                                    //We should insert before a state which is not the first state
                                    //in the list, so it is definite that the current state has a
                                    //previous, so we are to insert between these two states.
                                    insert_between(curr_state->get_prev(), curr_state, new_state);
                                }
                            }

                            /**
                             * Allows to destroy the given state from the level
                             * This method decrements the level size counter.
                             * The given state must be within the level list of states!
                             * @param state the state to be destroyed
                             */
                            inline void remove_and_destroy(stack_state_ptr state) {
                                ASSERT_SANITY_THROW((state == NULL),
                                        "Bad pointer state == NULL");

                                if (m_first_state == m_last_state) {
                                    LOG_DEBUG1 << "Removing the only state " << state << " from the list " << END_LOG;
                                    ASSERT_SANITY_THROW((state != m_first_state),
                                            "The provided state is not in the list!");

                                    //This is the only element in the list
                                    m_first_state = NULL;
                                    m_last_state = NULL;
                                } else {
                                    //There is more elements in the list
                                    if (state == m_last_state) {
                                        LOG_DEBUG1 << "Removing the last state!" << END_LOG;

                                        //We are deleting the last element of the list, and
                                        //there is more than one element in the list
                                        ASSERT_SANITY_THROW((state->m_prev == NULL),
                                                "The previous state can not be NULL!");

                                        //The new last element will be the previous of this one
                                        m_last_state = state->m_prev;
                                        //The new last element shall have no next element 
                                        m_last_state->m_next = NULL;
                                    } else {
                                        if (state == m_first_state) {
                                            LOG_DEBUG1 << "Removing the first state!" << END_LOG;

                                            //We are deleting the first element of the list, and
                                            //there is more than one element in the list
                                            ASSERT_SANITY_THROW((state->m_next == NULL),
                                                    "The next state can not be NULL!");

                                            //The new first element will be the next of this one
                                            m_first_state = state->m_next;
                                            //The new first element shall have no previous element
                                            m_first_state->m_prev = NULL;
                                        } else {
                                            //We are deleting some intermediate element
                                            LOG_DEBUG1 << "Removing an intermediate state!" << END_LOG;

                                            //The previous of this shall now point to the next of this as next
                                            state->m_prev->m_next = state->m_next;

                                            //The next of this shall now point to the previous of this as previous
                                            state->m_next->m_prev = state->m_prev;
                                        }
                                    }
                                }

                                LOG_DEBUG1 << "Destroying the state itself!" << END_LOG;
                                //Delete the state
                                delete state;

                                //Now it is time to decrement the count
                                m_size--;
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

                            //Stores the probability score bound for threshold pruning
                            prob_weight m_score_bound;
                        };
                    }
                }
            }
        }
    }
}

#endif /* MULTI_LEVEL_HPP */

