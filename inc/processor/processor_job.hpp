/* 
 * File:   processor_job.hpp
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
 * Created on July 25, 2016, 11:41 AM
 */

#ifndef PROCESSOR_JOB_HPP
#define PROCESSOR_JOB_HPP

#include <ostream>

#include "common/utils/id_manager.hpp"
#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"
#include "common/utils/threads/threads.hpp"

#include "common/messaging/msg_base.hpp"
#include "common/messaging/trans_session_id.hpp"
#include "common/messaging/trans_job_id.hpp"
#include "common/messaging/status_code.hpp"

using namespace std;

using namespace uva::utils;
using namespace uva::utils::exceptions;
using namespace uva::utils::threads;
using namespace uva::utils::logging;

using namespace uva::smt::bpbd::common::messaging;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace processor {

                //Forward class declaration
                class processor_job;
                //Typedef the processor job pointer
                typedef processor_job * proc_job_ptr;

                /**
                 * Allows to log the processor job into an output stream
                 * @param stream the output stream
                 * @param job the job to log
                 * @return the reference to the same output stream send back for chaining
                 */
                ostream & operator<<(ostream & stream, const processor_job & job);

                /**
                 * This is the processor job class:
                 * Responsibilities:
                 * 
                 */
                class processor_job {
                public:

                    //Define the function type for the function used to set the job result
                    typedef function<void(proc_job_ptr) > done_job_notifier;
                    //Define the function that is to be called to remove the task from the pool
                    typedef function<void(proc_job_ptr) > task_pool_remover;

                    /**
                     * The basic constructor
                     * @param session_id the id of the session from which the translation request is received
                     */
                    processor_job(const session_id_type session_id)
                    : m_session_id(session_id), m_notify_job_done_func(NULL) {
                    }

                    /**
                     * The basic destructor
                     */
                    virtual ~processor_job() {
                    }

                    /**
                     * Allows to retrieve the session id
                     * @return the session id
                     */
                    inline session_id_type get_session_id() const {
                        return m_session_id;
                    }

                    /**
                     * Allows to retrieve the job id as given by the client.
                     * The job id given by the balancer is retrieved by another method.
                     * @return the job id
                     */
                    inline job_id_type get_job_id() const {
                        //ToDo: Implement
                        THROW_NOT_IMPLEMENTED();
                    }

                    /**
                     * Allows to set the function that should be called when the job is done
                     * @param notify_job_done_func the job done notification function
                     *              to be called when the translation job is finished
                     */
                    inline void set_done_job_notifier(done_job_notifier notify_job_done_func) {
                        m_notify_job_done_func = notify_job_done_func;
                    }

                    /**
                     * Allows to set in the method that shall remove the task from the pool if called
                     * @param notify_task_cancel_func the function to call in case this task is being canceled.
                     */
                    inline void set_from_pool_remover(task_pool_remover pool_task_remove_func) {
                        //ToDo: Implement
                        THROW_NOT_IMPLEMENTED();
                    }

                    /**
                     * Performs the processor job
                     */
                    inline void execute() {
                        //ToDo: Implement
                        THROW_NOT_IMPLEMENTED();
                    }

                    /**
                     * Allows to wait until the job is finished, this
                     * includes the notification of the job pool.
                     */
                    inline void synch_job_finished() {
                        //ToDo: Implement
                        THROW_NOT_IMPLEMENTED();
                    }
                    
                    /**
                     * Allows to cancel the given translation job by telling all the translation tasks to stop.
                     * Calling this method indicates that the job is canceled due to the client disconnect
                     */
                    inline void cancel() {
                        //ToDo: Implement
                        THROW_NOT_IMPLEMENTED();
                    }

                protected:

                private:

                    //Stores the translation client session id
                    const session_id_type m_session_id;

                    //The done job notifier
                    done_job_notifier m_notify_job_done_func;
                };
            }
        }
    }
}




#endif /* PROCESSOR_JOB_HPP */

