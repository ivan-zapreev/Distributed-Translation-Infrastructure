/* 
 * File:   level_iter.hpp
 * Author: zapreevis
 *
 * Created on February 29, 2016, 10:00 AM
 */

#ifndef LEVEL_ITER_HPP
#define	LEVEL_ITER_HPP

#include "server/decoder/stack/stack_state.hpp"

using namespace uva::smt::bpbd::server::decoder;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace decoder {
                    namespace stack {

                        //The forward declaration of the multi-state level
                        class stack_level;
                        
                        /**
                         * Defines the basic multi-level iterator for iterating the level
                         */
                        class level_iter {
                        public:

                            /**
                             * The basic destructor
                             */
                            ~level_iter() {
                            }

                            /**
                             * The equality operator for the iterator
                             * @param other the other iterator to compare with
                             * @return true if the iterators are the same, otherwise false
                             */
                            inline bool operator==(level_iter & other) const {
                                return (this->m_state_ptr == other.m_state_ptr);
                            }

                            /**
                             * The inequality operator for the iterator
                             * @param other the other iterator to compare with
                             * @return true if the iterators are different, otherwise false
                             */
                            inline bool operator!=(level_iter & other) const {
                                return (this->m_state_ptr != other.m_state_ptr);
                            }

                            /**
                             * Implements the assignment operator
                             * @param other the other iterator to assign from
                             * @return the reference to itself
                             */
                            inline level_iter & operator=(level_iter & other) {
                                //Copy the pointer
                                this->m_state_ptr = other.m_state_ptr;

                                //Return the reference to itself
                                return *this;
                            }

                            /**
                             * The de-reference operator of the iterator
                             * @return the reference to the stored state, or an exception if it is the end iterator!
                             */
                            inline stack_state & operator*() const {
                                //Check that we are not dereferencing the end iterator
                                ASSERT_CONDITION_THROW((m_state_ptr == NULL), string("Iterator is at the end!"));

                                //Return the reference to the state object
                                return *m_state_ptr;
                            }

                            /**
                             * Define the pre-increment operator
                             * @return the reference to itself
                             */
                            inline level_iter & operator++() {
                                //Check that we are not incrementing to after the end
                                ASSERT_CONDITION_THROW((m_state_ptr == NULL), string("Iterating past the end!"));

                                //Move on to the next element
                                m_state_ptr = m_state_ptr->get_next();

                                //Return itself
                                return *this;
                            }

                        private:
                            //Stores the pointer to the current multi-state
                            stack_state_ptr m_state_ptr;

                            //Declare the friend class to access the private constructors
                            friend class stack_level;

                            /**
                             * The basic constructor, will be an end iterator
                             */
                            level_iter() : m_state_ptr(NULL) {
                            }

                            /**
                             * The basic constructor, will be an end iterator
                             * @param state_ptr the pointer to the begin element
                             */
                            level_iter(stack_state_ptr state_ptr) : m_state_ptr(state_ptr) {
                            }
                        };
                    }
                }
            }
        }
    }
}

#endif	/* LEVEL_ITER_HPP */

