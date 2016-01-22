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
#include <thread>

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
                    typedef websocketpp::lib::unique_lock<std::mutex> unique_lock;

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
                    : m_is_stopping(false), m_is_stopped(false), m_job_count(0),
                    m_jobs_thread(bind(&trans_job_pool::monitore_done_jobs, this)) {
                    }

                    /**
                     * he basic destructor
                     */
                    virtual ~trans_job_pool() {
                        //Stop if we have not stopped yet
                        if (!m_is_stopped) {
                            stop();
                        }
                    }

                    /**
                     * Allows to stop all the running jobs and try to send all the responses and then exit
                     */
                    void stop() {
                        //Cancel all the remaining jobs
                        cancel_all_jobs();

                        //Set the stopping flag
                        m_is_stopping = true;

                        //Wait until the job processing thread finishes
                        m_jobs_thread.join();

                        //We have stopped set the flag
                        m_is_stopped = true;
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
                    void schedule_job(trans_job_ptr trans_job) {
                        scoped_lock guard_map(m_s_map_lock);

                        //Throw an exception if we are stopping
                        ASSERT_CONDITION_THROW((m_is_stopping || m_is_stopped),
                                "The server is stopping/stopped, no service!");

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

                        //Increment the jobs count
                        m_job_count++;
                    }

                    /**
                     * Allows to cancel all the currently running translation jobs in the server
                     */
                    void cancel_all_jobs() {
                        scoped_lock guard_map(m_s_map_lock);

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
                        scoped_lock guard_map(m_s_map_lock);

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
                     * Allows notify the job pool that the given job is done.
                     * @param trans_job the pointer to the finished translation job 
                     */
                    void notify_job_done(trans_job_ptr trans_job) {
                        unique_lock guard_done(m_is_jd_lock);

                        //Add the job to the finished jobs list
                        {
                            scoped_lock guard_list(m_dj_list_lock);

                            m_done_jobs_list.push_back(trans_job);
                        }

                        //Notify the thread that there is a finished job to be processed
                        m_is_job_done.notify_all();
                    }

                    /**
                     * Allows to process the finished translation jobs
                     */
                    void monitore_done_jobs() {
                        unique_lock guard_done(m_is_jd_lock);

                        //Stop iteration only when we are stopping and there are no jobs left
                        while (!(m_is_stopping && m_job_count == 0)) {
                            //Wait the thread to be notified
                            m_is_job_done.wait(guard_done);
                            //the thread is notified, process the finished jobs
                            {
                                scoped_lock guard_list(m_dj_list_lock);

                                for (jobs_list_iter_type iter = m_done_jobs_list.begin(); iter != m_done_jobs_list.end(); ++iter) {
                                    //Get the translation job pointer
                                    trans_job_ptr trans_job = *iter;

                                    //Send the job response
                                    m_set_job_result_func(trans_job);

                                    //Erase the processed job from the list of finished jobs
                                    m_done_jobs_list.erase(iter);

                                    //Remove the job from the pool's administration 
                                    {
                                        scoped_lock guard_map(m_s_map_lock);

                                        m_sessions_map[trans_job->get_session_id()].erase(trans_job->get_job_id());

                                        //Decrement the jobs count 
                                        m_job_count--;
                                    }

                                    //Delete the job as it is not needed any more
                                    delete trans_job;
                                }
                            }
                        }
                    }

                private:
                    //Stores the synchronization mutex for working with the m_sessions_map
                    websocketpp::lib::mutex m_s_map_lock;

                    //Stores the mapping from the session id to the active translation jobs from the session
                    sessions_map_type m_sessions_map;

                    //Stores the reply sender functional
                    job_result_setter m_set_job_result_func;

                    //Stores the flag that indicates that we are stopping
                    bool m_is_stopping;
                    //Stores the flag that indicates we have stopped
                    bool m_is_stopped;
                    //Stores the active jobs count
                    uint64_t m_job_count;

                    //Stores the synchronization mutex for working with the conditional wait/notify
                    websocketpp::lib::mutex m_is_jd_lock;
                    //The conditional variable for tracking the done jobs
                    websocketpp::lib::condition_variable m_is_job_done;

                    //Stores the thread that manages finished jobs
                    websocketpp::lib::thread m_jobs_thread;

                    //Stores the synchronization mutex for working with the m_done_jobs
                    websocketpp::lib::mutex m_dj_list_lock;

                    //The list of finished jobs pending to be processed
                    jobs_list_type m_done_jobs_list;
                };

            }
        }
    }
}

#endif /* TRANS_JOB_POOL_HPP */

