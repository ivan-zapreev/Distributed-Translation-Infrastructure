/* 
 * File:   ordered_list.hpp
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
 * Created on May 4, 2016, 1:56 PM
 */

#ifndef ORDERED_LIST_HPP
#define ORDERED_LIST_HPP

#include "common/utils/logging/logger.hpp"
#include "common/utils/exceptions.hpp"

using namespace std;
using namespace uva::utils::exceptions;
using namespace uva::utils::logging;

namespace uva {
    namespace utils {
        namespace containers {

            /**
             * This is the class implementing the ordered list of elements which has a limited capacity.
             * If the element does not pass within the list then it gets destructured.
             * @param elem_type the type of the element that the list can store.
             */
            template<typename elem_type>
            class ordered_list {
            public:

                /**
                 * This is the container structure to store the pointer to the
                 * stored elements and to link the list elements between each other
                 */
                struct elem_container {
                    //The pointer to the stored element
                    elem_type * m_elem;
                    //The pointer to the next element wrapper
                    elem_container * m_prev;
                    //The pointer to the previous element wrapper
                    elem_container * m_next;

                    /**
                     * The basic constructor for the element
                     * @param elem the pointer to the element to be stored, not NULL
                     */
                    elem_container(elem_type * elem) : m_elem(elem), m_prev(NULL), m_next(NULL) {
                    }

                    /**
                     * The basic destructor, that deleted the element stored, through the pointer
                     */
                    ~elem_container() {
                        //Destroy the stored element
                        delete m_elem;
                    }

                    /**
                     * The operator for comparing two element containers, which does it by
                     * comparing the stored elements.
                     * @param other the other element container to compare with
                     * @return true if the element stored in this container is < the element stored in the other container
                     */
                    inline bool operator<(const elem_container & other) const {
                        return *m_elem < *(other.m_elem);
                    }
                    
                    /**
                     * Allows to get the reference to the stored object
                     * @return the reference to the stored element
                     */
                    inline elem_type & operator *() const {
                        return *m_elem;
                    }
                };

                /**
                 * 
                 * @param capacity
                 */
                ordered_list(size_t capacity) : m_capacity(capacity), m_size(0), m_first(NULL) {
                }

                /**
                 * The basic destructor
                 */
                ~ordered_list() {
                    //Declare the cursor
                    elem_container * curr = m_first;

                    //Destroy all the elements in the list
                    while (curr != NULL) {
                        //Store the next element
                        elem_container * next = curr->m_next;
                        
                        //Delete the current element
                        delete curr;
                        
                        //Move on to the next element
                        curr = next;
                    }
                }

                /**
                 * Allows to retrieve the first container element from the list
                 * @return the pointer to the first container element of the list.
                 */
                elem_container * get_first(){
                    return m_first;
                }
                
                /**
                 * This method is needed to add the new element to the list.
                 * In case the element does not pass into the list it gets deleted.
                 * If some other elements get pushed out if the list by this then they also get deleted.
                 * @param elem the, NOT NULL, pointer to the element to be added.
                 */
                inline void add_elemenent(elem_type * elem) {
                    //Create the new element to add
                    elem_container * new_elem = new elem_container(elem);

                    //Look for the situation we are in
                    if (m_first == NULL) {
                        //Add the new element as the first one
                        add_as_first(new_elem);
                    } else {
                        //Declare the cursor
                        elem_container * curr = m_first;

                        //Check if the new element is smaller than the first one
                        if (*new_elem < *curr) {
                            //Declare the index of the element that will be given to the new one
                            size_t idx = 1;
                            //Search until there is no further elements
                            while (curr != NULL) {
                                //If this is not the last element in the list
                                if ((curr->m_next != NULL) && (*new_elem < *(curr->m_next))) {
                                    //Move on to the next element
                                    curr = curr->m_next;
                                    //Increment the index
                                    ++idx;
                                } else {
                                    //The new element is larger than the next one then 
                                    //add it here between the current and the next ones
                                    add_after_this(curr, idx, new_elem);
                                    //Make the loop stop
                                    curr = NULL;
                                }
                            }
                        } else {
                            //Add the new element as the first one
                            add_as_first(new_elem);
                        }
                    }
                }

                /**
                 * Allows to get the number of elements currently stored in the list
                 * @return the number of elements stored in the list
                 */
                inline size_t get_size(){
                    return m_size;
                }
                
            protected:

                /**
                 * Allows to add a new element that will be given a certain index after the current element
                 * @param curr the current element to insert the element after
                 * @param idx the index the new element will get
                 * @param new_elem the new element to be inserted after the current one
                 */
                inline void add_after_this(elem_container * curr, size_t idx, elem_container * new_elem) {
                    elem_container * next_next = curr->m_next;
                    curr->m_next = new_elem;
                    new_elem->m_next = next_next;
                    new_elem->m_prev = curr;
                    //If there is one more element then loop it back to the new first one
                    if (new_elem->m_next != NULL) {
                        new_elem->m_next->m_prev = new_elem;
                    }

                    //Increment the size
                    ++m_size;

                    //Prune some elements out, if needed
                    prune(idx, new_elem);
                }

                /**
                 * Allows to add the new element as the first one
                 * @param new_elem the new element to add
                 */
                inline void add_as_first(elem_container * new_elem) {
                    //Add the element as the first one in the list
                    new_elem->m_next = m_first;
                    m_first = new_elem;
                    //If there is one more element then loop it back to the new first one
                    if (m_first->m_next != NULL) {
                        m_first->m_next->m_prev = m_first;
                    }

                    //Increment the size
                    ++m_size;

                    //Prune some elements out, if needed
                    prune(0, new_elem);
                }

                /**
                 * Allows to prune the exceeding elements
                 * @param idx the index under which the new element was added
                 * @param elem the new element that was added
                 */
                inline void prune(size_t idx, elem_container * elem) {
                    //Check if we need to prune
                    if (m_size > m_capacity) {
                        //Check the sanity 
                        ASSERT_SANITY_THROW((m_size > (m_capacity + 1)),
                                "There is more than one extra element in the list!!!");

                        //Find the exceeding element
                        while (idx != m_size) {
                            elem = elem->m_next;
                            ++idx;
                        }
                        
                        //Now that the exceeding element is found, remove it
                        elem->m_prev->m_next = NULL;
                        //Delete the element 
                        delete elem;
                        //Decrement the size
                        m_size--;
                    }
                }

            private:
                //Stores the list capacity
                size_t m_capacity;
                //Stores the current number of elements
                size_t m_size;
                //Stores the pointer to the first element of the list
                elem_container * m_first;
            };
        }
    }
}


#endif /* ORDERED_LIST_HPP */

