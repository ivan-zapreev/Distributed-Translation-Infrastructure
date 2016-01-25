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
#include <vector>

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
                    //Define the lock type to synchronize map operations
                    typedef websocketpp::lib::lock_guard<websocketpp::lib::mutex> scoped_lock;
                    //Define the unique lock needed for wait/notify
                    typedef websocketpp::lib::unique_lock<websocketpp::lib::mutex> unique_lock;

                    //Define the function type for the function used to set the translation job resut
                    typedef websocketpp::lib::function<void(trans_job_ptr trans_job) > job_result_setter;

                    //Define the job id to job and session id to jobs maps and iterators thereof
                    typedef std::map<job_id_type, trans_job_ptr> jobs_map_type;
                    typedef jobs_map_type::iterator jobs_map_iter_type;
                    typedef std::map<session_id_type, jobs_map_type> sessions_map_type;
                    typedef sessions_map_type::iterator sessions_map_iter_type;

                    //Define the vector of jobs type and its iterator
                    typedef vector<trans_job_ptr> jobs_list_type;
                    typedef jobs_list_type::iterator jobs_list_iter_type;

                    /**
                     * The basic constructor,  starts the finished jobs processing thread.
                     */
                    trans_job_pool()
                    : m_is_stopping(false), m_job_count(0),
                    m_jobs_thread(bind(&trans_job_pool::process_finished_jobs, this)) {
                    }

                    /**
                     * he basic destructor
                     */
                    virtual ~trans_job_pool() {
                        stop();
                    }

                    /**
                     * Allows to stop all the running jobs and try to send all the responses and then exit
                     */
                    void stop() {
                        //Make sure this does not interfere with any adding new job activity
                        {
                            scoped_lock guard_stopping(m_stopping_lock);

                            //If we are not stopping yet, do a stop, else return
                            if (!m_is_stopping) {
                                //Set the stopping flag
                                m_is_stopping = true;
                            } else {
                                return;
                            };
                        }

                        //Cancel all the remaining jobs, do that without the stopping lock synchronization
                        //If put inside the above synchronization block will most likely cause a deadlock(?).
                        cancel_all_jobs();

                        //Wait until the job processing thread finishes
                        m_jobs_thread.join();

                        //In case that we have stopped with running jobs - report an error
                        if (m_job_count != 0) {
                            LOG_ERROR << "The jobs pool has stopped but there are still "
                                    << m_job_count << " running jobs left!" << END_LOG;
                        }
                    }

                    /**
                     * Allows to set the response sender function for sending the replies to the client
                     * @param set_job_result_func the setter functional to be set
                     */
                    void set_job_result_setter(job_result_setter set_job_result_func) {
                        m_set_job_result_func = set_job_result_func;
                    }

                    /**
                     * Allows to schedule a new translation job.
                     * The execution of the job is deferred and asynchronous.
                     * @oaram trans_job the translation job to be scheduled
                     */
                    void plan_new_job(trans_job_ptr trans_job) {
                        //Make sure that we are not being stopped before or during this method call
                        scoped_lock guard_stopping(m_stopping_lock);

                        //Throw an exception if we are stopping
                        ASSERT_CONDITION_THROW(m_is_stopping,
                                "The server is stopping/stopped, no service!");

                        //Add the notification handler to the job
                        trans_job->set_done_job_notifier(bind(&trans_job_pool::notify_job_done, this, _1));

                        //Add the translation job into the administration
                        add_job(trans_job);
                    }

                    /**
                     * Allows to cancel all translation jobs for the given session id.
                     * @param session_id the session id to cancel the jobs for
                     */
                    void cancel_jobs(const session_id_type session_id) {
                        scoped_lock guard_all_jobs(m_all_jobs_lock);

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
                     * Allows to cancel all the currently running translation jobs in the server
                     */
                    void cancel_all_jobs() {
                        scoped_lock guard_all_jobs(m_all_jobs_lock);

                        //Cancel the scheduled translation tasks and stop the internal thread
                        for (sessions_map_iter_type iter = m_sessions_map.begin(); iter != m_sessions_map.end(); ++iter) {
                            //Call the canceling method on the task
                            cancel_jobs(iter->first);
                        }
                    }

                    /**
                     * Allows to add a new job to the administration. In case the
                     * session is not known or the job id is already in use an
                     * exception is thrown. Also the job count is incremented
                     * @param trans_job the job to be added to the administration
                     */
                    void add_job(trans_job_ptr trans_job) {
                        scoped_lock guard_all_jobs(m_all_jobs_lock);

                        //Get the session id for future use
                        const session_id_type session_id = trans_job->get_session_id();
                        //Get the job id for future use
                        const job_id_type job_id = trans_job->get_job_id();

                        //Check that the job with the same id does not exist
                        ASSERT_CONDITION_THROW((m_sessions_map[session_id][job_id] != NULL),
                                string("The job with id ") + to_string(job_id) + (" for session ") +
                                to_string(session_id) + (" already exists!"));

                        //The session is present, so we need to add it into the pool
                        m_sessions_map[session_id][job_id] = trans_job;

                        //Increment the jobs count
                        m_job_count++;

                        //ToDo: Add the job tasks to the tasks' pool

                        //ToDo: The tasks pool shall be chosen based on the source and target language
                    }

                    /**
                     * Allows to delete the given job from the administration,
                     * decrement the jobs count and destroy the job object.
                     * @param trans_job the job to be deleted
                     */
                    void delete_job(trans_job_ptr trans_job) {
                        //Get and store the session and job ids for later use
                        const session_id_type session_id = trans_job->get_session_id();
                        const job_id_type job_id = trans_job->get_job_id();

                        //Remove the job from the pool's administration 
                        {
                            scoped_lock guard_all_jobs(m_all_jobs_lock);

                            //Erase the job from the jobs mapping
                            m_sessions_map[session_id].erase(job_id);

                            //If there are not jobs left for this session,
                            //then remove the mapping to save space.
                            if (m_sessions_map[session_id].empty()) {
                                m_sessions_map.erase(session_id);
                            }

                            //Decrement the jobs count 
                            m_job_count--;
                        }

                        //Delete the job as it is not needed any more
                        delete trans_job;
                    }

                    /**
                     * Allows to check if the finished jobs processing loop has to stop.
                     * @return true if the finished jobs processing loop has to stop, otherwise false
                     */
                    bool is_stop_running() {
                        //Make sure that we are not being stopped before or during this method call
                        scoped_lock guard_stopping(m_stopping_lock);
                        //Make sure not one adds/removes jobs meanwhile
                        scoped_lock guard_all_jobs(m_all_jobs_lock);

                        //We shall stop if we are being asked to stop and there are no jobs left
                        return (m_is_stopping && m_job_count == 0);
                    }

                    /**
                     * Allows notify the job pool that the given job is done.
                     * @param trans_job the pointer to the finished translation job 
                     */
                    void notify_job_done(trans_job_ptr trans_job) {
                        unique_lock guard_finished_jobs(m_finished_jobs_lock);

                        //Add the job to the finished jobs list
                        m_done_jobs_list.push_back(trans_job);

                        //Notify the thread that there is a finished job to be processed
                        m_is_job_done.notify_one();
                    }

                    /**
                     * Allows to process the finished translation jobs
                     */
                    void process_finished_jobs() {
                        unique_lock guard_finished_jobs(m_finished_jobs_lock);

                        //Stop iteration only when we are stopping and there are no jobs left
                        while (!is_stop_running()) {
                            //Wait the thread to be notified
                            m_is_job_done.wait(guard_finished_jobs);

                            //The thread is notified, process the finished jobs
                            for (jobs_list_iter_type iter = m_done_jobs_list.begin(); iter != m_done_jobs_list.end(); ++iter) {
                                //Get the translation job pointer
                                trans_job_ptr trans_job = *iter;

                                //Send the job response
                                m_set_job_result_func(trans_job);

                                //Erase the processed job from the list of finished jobs
                                m_done_jobs_list.erase(iter);

                                //Remove the job from the pool's administration and destroy
                                delete_job(trans_job);
                            }
                        }
                    }

                private:
                    //Stores the synchronization mutex for working with the m_sessions_map
                    websocketpp::lib::mutex m_all_jobs_lock;

                    //Stores the mapping from the session id to the active translation jobs from the session
                    sessions_map_type m_sessions_map;

                    //Stores the reply sender functional
                    job_result_setter m_set_job_result_func;

                    //Stores the synchronization mutex for administering stopping
                    websocketpp::lib::mutex m_stopping_lock;
                    //Stores the flag that indicates that we are stopping, made an atomic just in case
                    atomic<bool> m_is_stopping;
                    //Stores the active jobs count, made an atomic just in case
                    atomic<uint64_t> m_job_count;

                    //Stores the synchronization mutex for working with the conditional wait/notify
                    websocketpp::lib::mutex m_finished_jobs_lock;
                    //The conditional variable for tracking the done jobs
                    websocketpp::lib::condition_variable m_is_job_done;

                    //Stores the thread that manages finished jobs
                    websocketpp::lib::thread m_jobs_thread;

                    //The list of finished jobs pending to be processed
                    jobs_list_type m_done_jobs_list;
                };
            }
        }
    }
}

#endif /* TRANS_JOB_POOL_HPP */

