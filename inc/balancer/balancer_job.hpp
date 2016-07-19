/* 
 * File:   balancer_job.hpp
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
 * Created on July 7, 2016, 12:11 PM
 */

#ifndef BALANCER_JOB_HPP
#define BALANCER_JOB_HPP

#include <ostream>

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"
#include "common/utils/threads/threads.hpp"

#include "common/messaging/trans_session_id.hpp"
#include "common/messaging/trans_job_id.hpp"
#include "common/messaging/status_code.hpp"

#include "client/messaging/trans_job_req_out.hpp"
#include "client/messaging/trans_job_resp_in.hpp"
#include "server/messaging/trans_job_req_in.hpp"
#include "server/messaging/trans_job_resp_out.hpp"

using namespace std;

using namespace uva::utils::exceptions;
using namespace uva::utils::threads;
using namespace uva::utils::logging;

using namespace uva::smt::bpbd::common::messaging;
using namespace uva::smt::bpbd::client::messaging;
using namespace uva::smt::bpbd::server::messaging;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace balancer {

                //Forward class declaration
                class balancer_job;
                //Typedef the balancer job pointer
                typedef balancer_job * bal_job_ptr;

                /**
                 * This is the translation job class:
                 * Responsibilities:
                 *      Stores the session id/handler
                 *      Stores the original job id
                 *      Stores the newly issued job id
                 *      Stores the text to be translated
                 *      Stores the translation job response
                 *      Gets the server adapter
                 *      Notify about a failed job dispatch
                 *      Send translation request
                 *      Send the translation response
                 *      Remember in which state the job is:
                 *         Waiting for sending request
                 *         Waiting for receiving reply
                 */
                class balancer_job {
                public:

                    //Define the function type for the function used to set the translation job result
                    typedef function<void(bal_job_ptr) > done_job_notifier;
                    //Define the function that is to be called to remove the task from the pool
                    typedef function<void(bal_job_ptr) > task_pool_remover;

                    /**
                     * The basic constructor allowing to initialize the main class constants
                     * @param session_id the id of the session from which the translation request is received
                     * @param trans_req the reference to the translation job request to get the data from.
                     */
                    balancer_job(const session_id_type session_id, trans_job_req_in * trans_req)
                    : m_session_id(session_id), m_trans_req(trans_req), m_notify_job_done_func(NULL) {
                    }

                    /**
                     * The basic constructor
                     */
                    virtual ~balancer_job() {
                        if (m_trans_req != NULL) {
                            delete m_trans_req;
                        }
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
                        return m_trans_req->get_job_id();
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
                    void set_from_pool_remover(task_pool_remover pool_task_remove_func) {
                        //Do not store the function, we do not want to remove it from the pool.
                        //This job, even if canceled will be executed by the pool's worker
                        //and will be removed from the pool by him as well. 
                    }

                    /**
                     * Allows to cancel the given translation job by telling all the translation tasks to stop.
                     */
                    inline void cancel() {
                        //ToDo: Implement
                    }

                    /**
                     * Allows to wait until the job is finished, this
                     * includes the notification of the job pool.
                     */
                    inline void synch_job_finished() {
                        //ToDo: Implement
                    }

                    /**
                     * Performs balancer job actions depending on the internal job state
                     */
                    void execute() {
                        //ToDo: Implement
                    }

                protected:
                private:
                    //Stores the translation client session id
                    const session_id_type m_session_id;

                    //Stores the pointer to the incoming translation job request, not NULL
                    trans_job_req_in * m_trans_req;

                    //The done job notifier
                    done_job_notifier m_notify_job_done_func;
                };

                /**
                 * Allows to log the balancer job into an output stream
                 * @param stream the output stream
                 * @param job the job to log
                 * @return the reference to the same output stream send back for chaining
                 */
                ostream & operator<<(ostream & stream, const balancer_job & job);
            }
        }
    }
}

#endif /* BALANCER_JOB_HPP */

