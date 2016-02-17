/* 
 * File:   multi_level.hpp
 * Author: zapreevis
 *
 * Created on February 17, 2016, 4:41 PM
 */

#ifndef MULTI_LEVEL_HPP
#define	MULTI_LEVEL_HPP

#include <string>

#include "common/utils/threads.hpp"
#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"

#include "server/decoder/de_configs.hpp"
#include "server/decoder/de_parameters.hpp"

#include "server/decoder/stack/multi_state.hpp"

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

                        /**
                         * Represents the multi-stack level
                         */
                        class multi_level {
                        public:

                            /**
                             * Defines the basic multi-level iterator for iterating the level
                             */
                            class iterator {
                            public:

                                /**
                                 * The basic destructor
                                 */
                                ~iterator() {
                                }

                                /**
                                 * The equality operator for the iterator
                                 * @param other the other iterator to compare with
                                 * @return true if the iterators are the same, otherwise false
                                 */
                                inline bool operator==(iterator & other) const {
                                    return (this->m_state_ptr == other.m_state_ptr);
                                }

                                /**
                                 * The inequality operator for the iterator
                                 * @param other the other iterator to compare with
                                 * @return true if the iterators are different, otherwise false
                                 */
                                inline bool operator!=(iterator & other) const {
                                    return (this->m_state_ptr != other.m_state_ptr);
                                }

                                /**
                                 * Implements the assignment operator
                                 * @param other the other iterator to assign from
                                 * @return the reference to itself
                                 */
                                inline iterator & operator=(iterator & other) {
                                    //Copy the pointer
                                    this->m_state_ptr = other.m_state_ptr;

                                    //Return the reference to itself
                                    return *this;
                                }

                                /**
                                 * The de-reference operator of the iterator
                                 * @return the reference to the stored state, or an exception if it is the end iterator!
                                 */
                                inline multi_state & operator*() const {
                                    //Check that we are not dereferencing the end iterator
                                    ASSERT_CONDITION_THROW((m_state_ptr == NULL), string("Iterator is at the end!"));

                                    //Return the reference to the state object
                                    return *m_state_ptr;
                                }

                                /**
                                 * Define the pre-increment operator
                                 * @return the reference to itself
                                 */
                                inline iterator & operator++() {
                                    //Check that we are not incrementing to after the end
                                    ASSERT_CONDITION_THROW((m_state_ptr == NULL), string("Iterating past the end!"));

                                    //Move on to the next element
                                    m_state_ptr = m_state_ptr->get_next();

                                    //Return itself
                                    return *this;
                                }

                            private:
                                //Stores the pointer to the current multi-state
                                multi_state_ptr m_state_ptr;

                                //Declare the friend class to access the private constructors
                                friend class multi_level;

                                /**
                                 * The basic constructor, will be an end iterator
                                 */
                                iterator() : m_state_ptr(NULL) {
                                }

                                /**
                                 * The basic constructor, will be an end iterator
                                 * @param state_ptr the pointer to the begin element
                                 */
                                iterator(multi_state_ptr state_ptr) : m_state_ptr(state_ptr) {
                                }
                            };

                            /**
                             * The basic constructor
                             */
                            multi_level() : m_first_state(NULL) {
                            }

                            /**
                             * The basic destructor
                             */
                            ~multi_level() {
                                //ToDo: Iterate through the level states and delete them
                            }

                            /**
                             * Allows to add a new state into the level
                             * @param new_state the new state to add
                             */
                            void add_state(multi_state_ptr new_state) {
                                //Initialize the reference to the pointer to the
                                //state place where we should put the new one
                                multi_state_ptr & place_state = m_first_state;

                                //ToDo: Search through the list for two things:
                                // 1. The equal hypothesis == - for recombination
                                // 2. The order position < for cost ordering.
                                //In case the equal hypothesis is not found and
                                //the recombination is not done, then insert the
                                //hypothesis to the found position based on cost
                                //comparison.
                                
                                //Search for the proper position of the state,
                                //or until we find an empty position, then stop.
                                while ((place_state != NULL) && (*new_state < *place_state)) {
                                    //Move further to the next state
                                    place_state = place_state->get_next();
                                }

                                //ToDo: How can we combine it with recombination?
                                //We re-combine two hypothesis if:
                                //1. They cover the same words
                                //2. They have the same last translated source word
                                //3. They have the same history: last n-1 target words
                                
                                //Check if the found free
                                if (place_state == NULL) {
                                    //Assign the state to the found position
                                    place_state = new_state;
                                } else {
                                    //If the place state is not NULL then
                                }
                            }

                            /**
                             * Allows to get the begin iterator
                             * @return the begin iterator
                             */
                            iterator begin() const {
                                return iterator(m_first_state);
                            }

                            /**
                             * Allows to get the end iterator
                             * @return the end iterator
                             */
                            iterator end() const {
                                return iterator(NULL);
                            }

                        protected:
                        private:
                            //Stores the pointer to the first level state
                            multi_state_ptr m_first_state;
                        };
                    }
                }
            }
        }
    }
}

#endif	/* MULTI_LEVEL_HPP */

