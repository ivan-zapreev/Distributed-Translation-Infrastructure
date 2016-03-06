/* 
 * File:   circular_queue.hpp
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
 * Created on February 16, 2016, 5:58 PM
 */

#ifndef CIRCULAR_QUEUE_HPP
#define CIRCULAR_QUEUE_HPP

#include <ostream>
#include <cstring>
#include <algorithm>

#include "common/utils/logging/logger.hpp"
#include "common/utils/exceptions.hpp"
#include "common/utils/string_utils.hpp"

using namespace std;

using namespace uva::utils::exceptions;
using namespace uva::utils::logging;
using namespace uva::utils::text;

namespace uva {
    namespace utils {
        namespace containers {

            /**
             * This class represents a circular queue class that
             * is needed to store a limited and fixed amount of
             * elements. This queue is designed to store only basic type elements
             * WARNING: Class does a shallow copy of elements using the memcpy!
             * So do not store here complex data structures with pointers and
             * the overridden assign operator!
             * WARNING: When comparing elements of the queue does a byte
             * comparison with memcmp and not the == operator!
             */
            template<typename elem_type, size_t capacity>
            class circular_queue {
            public:

                /**
                 * The basic constructor
                 */
                circular_queue() : m_size(0) {
                    LOG_DEBUG2 << "The queue capacity: " << capacity << END_LOG;

                    LOG_DEBUG2 << "The queue state: " << *this << END_LOG;
                }

                /**
                 * The basic constructor
                 * @param num_elems the number of elements to put into the queue
                 * @param elems the elements to put into the queue
                 */
                circular_queue(const size_t num_elems, const elem_type * elems) : m_size(0) {
                    LOG_DEBUG2 << "Creating with: " << array_to_string<elem_type>(num_elems, elems)
                            << ", the queue capacity: " << capacity << END_LOG;

                    //Push back the array of values
                    push_back(num_elems, elems);

                    LOG_DEBUG2 << "The queue state: " << *this << END_LOG;
                }

                /**
                 * The special case of a copy constructor, it allows to take the
                 * parameter queue together with the extra elements and copy them
                 * into the given queue. This is done in a smart way to optimize
                 * performance.
                 * @param other the other queue to copy from
                 * @param num_elems the number of extra elements
                 * @param elems the pointer to the array with the extra elements
                 */
                circular_queue(const circular_queue & other, const size_t num_elems, const elem_type * elems) : m_size(0) {
                    LOG_DEBUG2 << "Creating with queue: " << other << ", and array: "
                            << array_to_string<elem_type>(num_elems, elems)
                            << ", the queue capacity: " << capacity << END_LOG;

                    if ((num_elems >= capacity) || (other.m_size == 0)) {
                        LOG_DEBUG2 << "The number of array elements: " << num_elems
                                << ", is larget or equal to the queue capacity: " << capacity
                                << ", or the other queue size (" << other.m_size << ") is zero" << END_LOG;

                        //The number of elements in the array is larger or equal
                        //to the queue capacity or the other queue is empty, then
                        //we can just ignore the other queue.
                        push_back(num_elems, elems);

                        LOG_DEBUG2 << "The queue state: " << *this << END_LOG;
                    } else {
                        //The number of elements in the array is smaller than the
                        //capacity so we will fit both elements from the queue and
                        //from the array. 

                        //Compute the maximum number of elements from the queue that we can keep
                        const size_t max_num_from_queue = (capacity - num_elems);
                        //Compute the number of elements from the queue to keep
                        const size_t num_from_queue = min(other.m_size, max_num_from_queue);

                        LOG_DEBUG2 << "The max num of elem(s) to copy from the other queue is: " << max_num_from_queue
                                << ", the number of elem(s) that will be copied is: " << num_from_queue << END_LOG;

                        //Add the other queue elements, only the ones that we need to take over
                        push_back(num_from_queue, other.m_elems + (other.m_size - num_from_queue));

                        LOG_DEBUG2 << "The queue state: " << *this << END_LOG;

                        //Add the array elements
                        push_back(num_elems, elems);

                        LOG_DEBUG2 << "The queue state: " << *this << END_LOG;
                    }
                }

                /**
                 * The basic destructor
                 */
                ~circular_queue() {
                }

                /**
                 * Allows to check if a certain amount of this queue last elements
                 * is equal to the certain amount of the other queue last elements.
                 * @param other the other queue to compare with
                 * @param num_elems the number of last elements to compare
                 * @return true or false depending on?
                 * true if both queues have more than or equal to num_elems elements and the last num_elems are equal.
                 * true if both elements have less than num_elems but the number of elements is equal and the elements are equal
                 * false otherwise
                 */
                bool is_equal_last(const circular_queue & other, const size_t num_elems) {
                    if ((m_size >= num_elems) && (other.m_size >= num_elems)) {
                        //Compare the last num_elems elements
                        return (memcmp((m_elems + (m_size - num_elems)),
                                (other.m_elems + (other.m_size - num_elems)), num_elems) == 0);
                    } else {
                        if (m_size == other.m_size) {
                            //The number of elements is equal so compare the last m_size elements
                            return (memcmp(m_elems, other.m_elems, m_size) == 0);
                        } else {
                            //The number of last elements is different and
                            //at least one queue has less than num_elems
                            return false;
                        }
                    }
                }

                /**
                 * Allows to empty the queue
                 */
                void empty_queue() {

                    m_size = 0;
                }

                /**
                 * Allows to obtain the number of stored elements
                 * @return the number of stored elements
                 */
                size_t get_size() const {

                    return m_size;
                }

                /**
                 * Allows to obtain the pointer to the array storing the elements
                 * @return the pointer to the array storing the elements
                 */
                const elem_type * get_elems() const {

                    return m_elems;
                }

                /**
                 * Allows to obtain the maximum number of elements to store
                 * @return the maximum number of elements to store
                 */
                size_t get_capacity() const {

                    return capacity;
                }

                /**
                 * Allows to put the new element to the end of the queue,
                 * potentially pushing out the beginning of the queue element.
                 * The latter happens only if the maximum number of elements
                 * has been reached before this new element was pushed.
                 * @param elem the element to be stored in the queue
                 */
                void push_back(const elem_type & elem) {

                    push_back(1, &elem);
                }

                /**
                 * Allows to push back an entire array 
                 */
                void push_back(const size_t num_elems, const elem_type * elems) {
                    LOG_DEBUG2 << "Pushing array: " << array_to_string<elem_type>(num_elems, elems)
                            << " to the queue: " << *this << ", capacity: " << capacity << END_LOG;

                    //Compute the new size right away
                    const size_t new_size = min((m_size + num_elems), capacity);

                    LOG_DEBUG2 << "The new queue size will be " << new_size << END_LOG;

                    //Check if we can fit the data without shifting or not
                    if ((m_size + num_elems) <= capacity) {
                        //There is space to accommodate everything without shifting

                        LOG_DEBUG2 << "Copying " << num_elems << " elem(s) from "
                                << "position 0 to position: " << m_size << END_LOG;

                        //Copy the new data to the remainder of the queue
                        memcpy(m_elems + m_size, elems, num_elems * sizeof (elem_type));
                    } else {
                        //The number of elements will become larger than the queue capacity

                        //Check if the number of elements we have to add larger than the capacity
                        if (num_elems >= capacity) {
                            //The number of elements we are to add is larger than
                            //the queue capacity so we need to push all of the old 
                            //elements out and may be some of the new elements.

                            LOG_DEBUG2 << "The added number of elements " << num_elems
                                    << " is larger the capacity: " << capacity << END_LOG;

                            //Compute the sub-array element index to copy from
                            const size_t begin_idx = (num_elems - capacity);

                            LOG_DEBUG2 << "Copying " << capacity << " elem(s) from position: "
                                    << begin_idx << " to position 0 of the target " << END_LOG;

                            //Compute the position to start copying from, we only need last "capacity" elements
                            const elem_type * sub_arr = elems + begin_idx;

                            //Copy the last part of the data, overwriting everything there was
                            memcpy(m_elems, sub_arr, capacity * sizeof (elem_type));
                        } else {
                            //The number of elements we are to add is smaller than
                            //the queue capacity so they will all fit, but we need
                            //to push some of the old elements out

                            //Compute the number of free slots

                            const size_t num_empty = capacity - m_size;
                            //Compute the number of slots we need to free
                            const size_t num_push_out = num_elems - num_empty;
                            //Compute the number of elements to keep
                            const size_t num_keep = m_size - num_push_out;

                            LOG_DEBUG2 << "Number of free slots is: " << num_empty << ", yet to free "
                                    << num_push_out << " slots, the number of elements to keep is: "
                                    << num_keep << END_LOG;

                            LOG_DEBUG << "Moving " << num_keep << " elements from position: "
                                    << num_push_out << " to position 0" << END_LOG;

                            //Move the elements we have to keep to the beginning of the queue
                            memmove(m_elems, m_elems + num_push_out, num_keep * sizeof (elem_type));

                            LOG_DEBUG << "Copying " << num_elems << " elem(s) from "
                                    << "position 0 to position: " << num_keep << END_LOG;

                            //Copy the elements into the freed remainder of the array
                            memcpy(m_elems + num_keep, elems, num_elems * sizeof (elem_type));
                        }
                    }

                    //Store the new number of elements
                    m_size = new_size;
                }

            private:
                //Stores the array of elements
                elem_type m_elems[capacity];

                //Stores the number of stored elements
                size_t m_size;
            };

            /**
             * Allows to output the circular_queue object to the stream
             * @param stream the stream to output into
             * @param queue the queue object
             * @return the stream that we output into
             */
            template<typename elem, size_t capacity>
            static inline std::ostream& operator<<(std::ostream& stream, const circular_queue<elem, capacity> & queue) {
                return stream << array_to_string<elem>(queue.get_size(), queue.get_elems());
            }
        }
    }
}

#endif /* CIRCULAR_QUEUE_HPP */

