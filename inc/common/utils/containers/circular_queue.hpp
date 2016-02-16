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

#include "common/utils/logging/logger.hpp"
#include "common/utils/exceptions.hpp"

using namespace std;
using namespace uva::utils::exceptions;
using namespace uva::utils::logging;

namespace uva {
    namespace utils {
        namespace containers {

            /**
             * This class represents a circular queue class that
             * is needed to store a limited nd fixed amount of
             * elements. To avoid unneeded copy actions this class
             * stores the pointers to the elements!
             * 
             * WARNING: This class is not responsible for destroying
             * the elements it stores pointers to.
             */
            template<typename elem_type, size_t MAX_NUM_ELEMS>
            class circular_queue {
            public:

                /**
                 * The basic constructor
                 */
                circular_queue() {
                }

                /**
                 * The basic destructor
                 */
                ~circular_queue() {
                }
                
                /**
                 * Allows to put the new element to the end of the queue,
                 * potentially pushing out the beginning of the queue element.
                 * The latter happens only if the maximum number of elements
                 * has been reached before this new element was pushed.
                 * NOTE: The queue qill only store the pointer to the element but not the object!
                 * @param elem the element pointer to which will be stored inside the queue
                 */
                void push_back(const elem_type & elem){
                    //ToDo: Implement
                }
                
            protected:
            private:
            };
        }
    }
}

#endif /* CIRCULAR_QUEUE_HPP */

