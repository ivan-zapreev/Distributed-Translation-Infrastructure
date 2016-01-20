/* 
 * File:   trans_job_pool.hpp
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
 * Created on January 20, 2016, 3:01 PM
 */

#include <websocketpp/server.hpp>

#include "trans_session.hpp"
#include "common/messaging/trans_job_request.hpp"

using namespace std;
using namespace uva::smt::decoding::common::messaging;

#ifndef TRANS_JOB_POOL_HPP
#define TRANS_JOB_POOL_HPP

namespace uva {
    namespace smt {
        namespace decoding {
            namespace server {

                /**
                 * This class is used to schedule the translation jobs.
                 * Each translation job consists of a number of sentences to translate.
                 * Each sentence will be translated in its own thread with its own decoder instance.
                 * The job of this class is to split the translation job into a number
                 * of translation tasks and schedule them. This class is synchronized
                 * and has its own thread to schedule the translation tasks.
                 */
                class trans_job_pool {
                public:
                    typedef websocketpp::lib::function<void(const session_id_type session_id, const job_id_type job_id, const string & text) > response_sender;

                    /**
                     * The basic constructor
                     * 
                     * ToDo: Possibly limit the number of translation jobs to be scheduled
                     */
                    trans_job_pool() {
                    }

                    /**
                     * he basic destructor
                     */
                    virtual ~trans_job_pool() {
                        //ToDo: Cancel the scheduled translation tasks and stop the internal thread
                    }

                    /**
                     * Allows to set the response sender function for sending the replies to the client
                     * @param sender the s ender functional to be set
                     */
                    void set_response_sender(response_sender sender) {
                        m_sender_func = sender;
                    }

                    /**
                     * Allows to schedule a new translation job. The execution of the job is deferred and asynchronous.
                     */
                    void schedule_job(const session_id_type session_id, const trans_job_request_ptr job) {
                        //ToDo: Implement
                    }

                    /**
                     * Allows to cancel all translation jobs for the given session id.
                     * @param session_id the session id to cancel the jobs for
                     */
                    void cancel_jobs(const session_id_type session_id) {
                        //ToDo: Implement
                    }
                protected:

                    /**
                     * Allows to cancel all translation job defined by the session and job ids,
                     * @param session_id the session id of the translation client
                     * @param job_id the job id issues by the translation client to a translation job
                     */
                    void cancel_job(const session_id_type session_id, const job_id_type job_id) {
                        //ToDo: Implement
                    }
                private:

                    //Stores the reply sender functional
                    response_sender m_sender_func;
                };

            }
        }
    }
}

#endif /* TRANS_JOB_POOL_HPP */

