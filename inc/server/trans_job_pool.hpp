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

#include "dummy_trans_task.hpp"
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
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::placeholders::_3;
using websocketpp::lib::placeholders::_4;

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
                    typedef websocketpp::lib::function<void(const session_id_type session_id, const job_id_type job_id, const string & text) > response_sender;
                    typedef std::map<task_id_type, dummy_trans_task_ptr> tasks_map_type;
                    typedef std::map<session_id_type, task_id_type> sessions_map_type;

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
                        scoped_lock guard(m_lock);

                        //ToDo: Split the translation job into translation tasks, per sentence.

                        //ToDo: Add the task id to the list of tasks associated with the given session

                        //Issue the new task id
                        task_id_type task_id = m_task_id_mgr.get_next_id();

                        //Instantiate the new task and add it to the list of running tasks
                        tasks_map[task_id] = new dummy_trans_task(task_id, session_id, job,
                                bind(&trans_job_pool::report_task_result, this, _1, _2, _3, _4));

                        //Do sanity check on that there is no other translation task id associated with this session
                        ASSERT_SANITY_THROW((sessions_map[session_id] != session::UNDEFINED_SESSION_ID),
                                string("Session ") + to_string(session_id) + " already has a running task!");

                        //Add the session to translation tasks mapping
                        sessions_map[session_id] = task_id;
                    }

                    /**
                     * Allows to cancel all translation jobs for the given session id.
                     * @param session_id the session id to cancel the jobs for
                     */
                    void cancel_jobs(const session_id_type session_id) {
                        scoped_lock guard(m_lock);

                        //Get the tasks associated with the given session
                        task_id_type task_id = sessions_map[session_id];

                        //Remove the session to task ids mapping
                        sessions_map.erase(session_id);

                        //if there is a task associated with the given session then cancel it
                        if (task_id != session::UNDEFINED_SESSION_ID) {
                            //Stop the translation task
                            tasks_map[task_id]->stop_simulation();
                            //Delete the translation task
                            delete tasks_map[task_id];
                            //Erase the translation task mapping
                            tasks_map.erase(task_id);
                        }

                        //ToDo: Once a session has many jobs and those have many
                        //tasks the cancellation process shall be made more involved
                    }

                protected:

                    /**
                     * Allows to report the translation task report
                     * @param task_id the translation task id
                     * @param session_id the session id
                     * @param job_id the job id
                     * @param text the translated text
                     */
                    void report_task_result(const task_id_type task_id, const session_id_type session_id, const job_id_type job_id, const string & text) {
                        //Declare the dummy task
                        dummy_trans_task_ptr trans_task = NULL;
                        //Do the task retrieval synchronized to avoid collisions
                        {
                            scoped_lock guard(m_lock);

                            //Obtain the task from the mapping
                            trans_task = tasks_map[task_id];

                            //Remove the task from the list of scheduled tasks
                            tasks_map.erase(task_id);

                            //Remove the session to task id mapping
                            sessions_map.erase(session_id);
                        }

                        //If the dummy task was found then send the response and delete it
                        if (trans_task != NULL) {
                            //Send the response
                            m_sender_func(session_id, job_id, text);

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
                    response_sender m_sender_func;

                    //Stores the mapping from the translation task id to the translation tasks
                    tasks_map_type tasks_map;

                    //Stores the mapping from the session id to the translation task ids
                    sessions_map_type sessions_map;
                };

            }
        }
    }
}

#endif /* TRANS_JOB_POOL_HPP */

