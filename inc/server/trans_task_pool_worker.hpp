/* 
 * File:   task_pool_worker.hpp
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
 * Created on January 25, 2016, 1:46 PM
 */

#ifndef TASK_POOL_WORKER_HPP
#define TASK_POOL_WORKER_HPP

#include "trans_task.hpp"
#include "common/utils/threads.hpp"

using namespace std;
using namespace uva::utils::threads;

namespace uva {
    namespace smt {
        namespace translation {
            namespace server {

                //Do a forward declaration of the task pool
                class trans_task_pool;

                /**
                 * This class represents a translation tasks pool worker. This is
                 * class is to be used around the actual translation task inside
                 * the translation tasks pool. We need this class as a synchronization
                 * layer for the thread pool, as each of instances of this class will
                 * be run by a thread.
                 */
                class trans_task_pool_worker {
                public:

                    /**
                     * This is a basic constructor that needs the thread pool reference as an argument.
                     * @param pool the task pool reference
                     */
                    trans_task_pool_worker(trans_task_pool & pool) : m_pool(pool) {
                    }
                    
                    /**
                     * The basic destructor
                     */
                    virtual ~trans_task_pool_worker(){
                        //Nothing to be done, the thread is destroyed, no resources to free
                    }

                    /**
                     * This operator will be called to run the thread, its implementation
                     * will run the tasks scheduled in the thread pool.
                     */
                    void operator()();
                    
                protected:
                private:
                    trans_task_pool & m_pool;
                };
            }
        }
    }
}


#endif /* TASK_POOL_WORKER_HPP */

