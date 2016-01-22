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

#include "decoder_stub.hpp"
#include "trans_task.hpp"
#include "trans_session.hpp"
#include "common/messaging/trans_job_request.hpp"

#include "common/utils/Exceptions.hpp"
#include "common/utils/logging/Logger.hpp"

using namespace std;
using namespace uva::utils::logging;
using namespace uva::utils::exceptions;
using namespace uva::smt::decoding::common::messaging;
using namespace uva::smt::decoding::server::dummy;

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
                    typedef websocketpp::lib::function<void(const session_id_type session_id, const job_id_type job_id,
                            const trans_job_result code, const string & text) > job_result_setter;
                    typedef std::map<task_id_type, dummy_trans_task_ptr> tasks_map_type;
                    typedef std::map<session_id_type, task_id_type> sessions_map_type;
                    typedef sessions_map_type::iterator sessions_map_iter_type;

                    /**
                     * The basic constructor
                     * 
                     * ToDo: Possibly limit the number of translation jobs to be scheduled
                     */
                    trans_job_pool() : m_task_id_mgr(trans_task::MINIMUM_TASK_ID) {
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

                        //Create a translation job 
                        
                        //Add the translation job to the map
                        
                        //Schedule the translation tasks

                        const session_id_type session_id;
                        
                        //ToDo: Split the translation job into translation tasks, per sentence.

                        //ToDo: Add the task id to the list of tasks associated with the given session

                        //Issue the new task id
                        task_id_type task_id = m_task_id_mgr.get_next_id();

                        //Instantiate the new task and add it to the list of running tasks
                        m_tasks_map[task_id] = new decoder_stub(task_id, session_id, job,
                                bind(&trans_job_pool::set_task_result, this, _1));

                        //Do sanity check on that there is no other translation task id associated with this session
                        ASSERT_SANITY_THROW((m_sessions_map[session_id] != session::UNDEFINED_SESSION_ID),
                                string("Session ") + to_string(session_id) + " already has a running task!");

                        //Add the session to translation tasks mapping
                        m_sessions_map[session_id] = task_id;
                    }

                    /**
                     * Allows to cancel all the currently running translation jobs in the server
                     */
                    void cancel_all_jobs() {
                        scoped_lock guard(m_lock);

                        //Cancel the scheduled translation tasks and stop the internal thread
                        for (sessions_map_iter_type iter = m_sessions_map.begin(); iter != m_sessions_map.end(); ++iter) {
                            //Call the canceling method on the task
                            m_tasks_map[iter->second]->cancel();
                        }
                    }

                    /**
                     * Allows to cancel all translation jobs for the given session id.
                     * @param session_id the session id to cancel the jobs for
                     */
                    void cancel_jobs(const session_id_type session_id) {
                        scoped_lock guard(m_lock);

                        //Call the canceling method on the task
                        m_tasks_map[m_sessions_map[session_id]]->cancel();

                        //ToDo: Once a session has many jobs and those have many
                        //tasks the cancellation process shall be made more involved
                    }

                protected:

                    /**
                     * Allows to report the translation task report, will be called from another thread
                     * @param task_result the translation task result
                     */
                    void set_task_result(const trans_task & task_result) {
                        //Declare and initialize the needed constants
                        const session_id_type session_id = task_result.get_session_id();
                        const job_id_type job_id = task_result.get_job_id();
                        const task_id_type task_id = task_result.get_task_id();
                        
                        //Declare the dummy task
                        dummy_trans_task_ptr trans_task = NULL;
                        //Do the task retrieval synchronized to avoid collisions
                        {
                            scoped_lock guard(m_lock);

                            //Obtain the task from the mapping
                            trans_task = m_tasks_map[task_id];

                            //Remove the task from the list of scheduled tasks
                            m_tasks_map.erase(task_id);

                            //Remove the session to task id mapping
                            m_sessions_map.erase(session_id);
                        }

                        //If the dummy task was found then send the response and delete it
                        if (trans_task != NULL) {
                            //ToDo: Create a job result and set it if the job is finished!
                            //m_set_job_result_func(job_result);

                            //Delete the task
                            delete trans_task;
                        } else {
                            LOG_ERROR << "Could not find a dummy task for: " << task_id << "/" << session_id << "/" << job_id << END_LOG;
                        }
                    }

                private:
                    //Stores the synchronization mutex
                    websocketpp::lib::mutex m_lock;

                    //Stores the instance of the id manager
                    id_manager<task_id_type> m_task_id_mgr;

                    //Stores the reply sender functional
                    job_result_setter m_set_job_result_func;

                    //Stores the mapping from the translation task id to the translation tasks
                    tasks_map_type m_tasks_map;

                    //Stores the mapping from the session id to the translation task ids
                    sessions_map_type m_sessions_map;
                };

            }
        }
    }
}

#endif /* TRANS_JOB_POOL_HPP */

