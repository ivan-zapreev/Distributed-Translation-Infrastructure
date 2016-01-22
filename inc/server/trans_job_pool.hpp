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

#include <map>

#include <websocketpp/common/thread.hpp>
#include <websocketpp/server.hpp>

#include "trans_task_pool.hpp"
#include "common/messaging/trans_session_id.hpp"
#include "common/messaging/trans_job_id.hpp"
#include "common/messaging/trans_job_request.hpp"

#include "common/utils/Exceptions.hpp"
#include "common/utils/logging/Logger.hpp"
#include "trans_task_id.hpp"
#include "trans_job.hpp"

using namespace std;
using namespace uva::utils::logging;
using namespace uva::utils::exceptions;
using namespace uva::smt::decoding::common::messaging;

using websocketpp::lib::bind;
using websocketpp::lib::placeholders::_1;

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
                    typedef websocketpp::lib::lock_guard<websocketpp::lib::mutex> scoped_lock;
                    typedef websocketpp::lib::function<void(trans_job_ptr trans_job) > job_result_setter;

                    typedef std::map<job_id_type, trans_job_ptr> jobs_map_type;
                    typedef jobs_map_type::iterator jobs_map_iter_type;
                    typedef std::map<session_id_type, jobs_map_type> sessions_map_type;
                    typedef sessions_map_type::iterator sessions_map_iter_type;

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
                        //Cancel all the remaining jobs
                        cancel_all_jobs();
                    }

                    /**
                     * Allows to set the response sender function for sending the replies to the client
                     * @param set_job_result_func the s ender functional to be set
                     */
                    void set_response_sender(job_result_setter set_job_result_func) {
                        m_set_job_result_func = set_job_result_func;
                    }

                    /**
                     * Allows to schedule a new translation job.
                     * The execution of the job is deferred and asynchronous.
                     * @oaram trans_job the translation job to be scheduled
                     */
                    void schedule_job(trans_job_ptr trans_job) {
                        scoped_lock guard(m_lock);

                        //Get the session id for future use
                        const session_id_type session_id = trans_job->get_session_id();

                        //Check if the session with the given id is present
                        ASSERT_CONDITION_THROW((m_sessions_map.find(session_id) == m_sessions_map.end()),
                                string("The job: ") + to_string(trans_job->get_job_id()) +
                                string(" is scheduled for session: ") + to_string(trans_job->get_session_id()) +
                                string(", the this session does not exist!"));

                        //Get the job id for future use
                        const job_id_type job_id = trans_job->get_job_id();

                        //Check that the job with the same id does not exist
                        ASSERT_CONDITION_THROW((m_sessions_map[session_id][job_id] != NULL),
                                string("The job with id ") + to_string(job_id) + (" for session ") +
                                to_string(session_id) + (" already exists!"));

                        //The session is present, so we need to add it into the pool
                        m_sessions_map[session_id][trans_job->get_job_id()] = trans_job;
                    }

                    /**
                     * Allows to cancel all the currently running translation jobs in the server
                     */
                    void cancel_all_jobs() {
                        scoped_lock guard(m_lock);

                        //Cancel the scheduled translation tasks and stop the internal thread
                        for (sessions_map_iter_type iter = m_sessions_map.begin(); iter != m_sessions_map.end(); ++iter) {
                            //Call the canceling method on the task
                            cancel_jobs(iter->first);
                        }
                    }

                    /**
                     * Allows to cancel all translation jobs for the given session id.
                     * @param session_id the session id to cancel the jobs for
                     */
                    void cancel_jobs(const session_id_type session_id) {
                        scoped_lock guard(m_lock);

                        //Get the jobs jobs for the given session
                        if (m_sessions_map.find(session_id) != m_sessions_map.end()) {
                            //Get the known jobs map for the given session
                            jobs_map_type & m_jobs_map = m_sessions_map[session_id];

                            //Iterate through the available jobs and cancel them
                            for (jobs_map_iter_type iter = m_jobs_map.begin(); iter != m_jobs_map.end(); ++iter) {
                                //Call the canceling method on the job
                               iter->second->cancel();
                            }
                        } else {
                            LOG_ERROR << "There is no session registered for id: " << session_id << END_LOG;
                        }
                    }

                protected:

                    /**
                     * Allows to set the non-error translation result,
                     * this will also send the response to the client.
                     * WARNING: The translation job then deleted via this method
                     * @param trans_job the pointer to the finished translation job 
                     */
                    void set_job_result(trans_job_ptr trans_job) {
                        //ToDo: Remove the job from the pool's administration 
                        {
                            scoped_lock guard(m_lock);

                            //ToDo: Implement
                        }

                        //Call the translation manager to send the translation
                        //request response. This call will also destroy the job.
                        m_set_job_result_func(trans_job);
                    }

                private:
                    //Stores the synchronization mutex
                    websocketpp::lib::mutex m_lock;

                    //Stores the reply sender functional
                    job_result_setter m_set_job_result_func;

                    //Stores the mapping from the session id to the active translation jobs from the session
                    sessions_map_type m_sessions_map;
                };

            }
        }
    }
}

#endif /* TRANS_JOB_POOL_HPP */

