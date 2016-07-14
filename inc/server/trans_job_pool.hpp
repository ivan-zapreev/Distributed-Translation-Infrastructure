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

#include <unordered_map>
#include <vector>

#include <websocketpp/server.hpp>

#include "common/utils/threads/threads.hpp"
#include "common/messaging/trans_session_id.hpp"
#include "common/messaging/trans_job_id.hpp"

#include "common/utils/threads/threads.hpp"
#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"
#include "common/utils/threads/task_pool.hpp"

#include "server/trans_task_id.hpp"
#include "server/trans_job.hpp"

using namespace std;
using namespace std::placeholders;
using namespace uva::utils::logging;
using namespace uva::utils::exceptions;
using namespace uva::utils::threads;
using namespace uva::smt::bpbd::common::messaging;

#ifndef TRANS_JOB_POOL_HPP
#define TRANS_JOB_POOL_HPP

namespace uva {
    namespace smt {
        namespace bpbd {
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

                    //Define the function type for the function used to set the translation job resut
                    typedef function<void(trans_job_ptr trans_job) > finished_job_notifier;

                    //Define the job id to job and session id to jobs maps and iterators thereof
                    typedef std::unordered_map<job_id_type, trans_job_ptr> jobs_map_type;
                    typedef jobs_map_type::iterator jobs_map_iter_type;
                    typedef std::unordered_map<session_id_type, jobs_map_type> sessions_map_type;
                    typedef sessions_map_type::iterator sessions_map_iter_type;

                    //Define the vector of jobs type and its iterator
                    typedef vector<trans_job_ptr> jobs_list_type;
                    typedef jobs_list_type::iterator jobs_list_iter_type;

                    /**
                     * The basic constructor,  starts the finished jobs processing thread.
                     * @param num_threads the number of translation threads to run
                     */
                    trans_job_pool(const size_t num_threads)
                    : m_tasks_pool(num_threads), m_is_stopping(false), m_job_count(0),
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
                    inline void stop() {
                        LOG_DEBUG << "Request stopping the job pool!" << END_LOG;

                        //Make sure this does not interfere with any adding new job activity
                        {
                            scoped_guard guard_stopping(m_stopping_lock);

                            //If we are not stopping yet, do a stop, else return
                            if (!m_is_stopping) {
                                //Set the stopping flag
                                m_is_stopping = true;
                            } else {
                                return;
                            };
                        }

                        LOG_DEBUG << "The stopping flag is set!" << END_LOG;

                        //Cancel all the remaining jobs, do that without the stopping lock synchronization
                        //If put inside the above synchronization block will most likely cause a deadlock(?).
                        cancel_all_jobs();

                        LOG_DEBUG << "All the existing jobs are canceled!" << END_LOG;

                        //Wake up the jobs thread for the case there is no jobs being processed
                        start_processing_finished_jobs();

                        //Wait until the job processing thread finishes
                        m_jobs_thread.join();

                        LOG_DEBUG << "The result processing thread is finished!" << END_LOG;

                        //In case that we have stopped with running jobs - report an error
                        if (m_job_count != 0) {
                            LOG_ERROR << "The jobs pool has stopped but there are still "
                                    << m_job_count << " running jobs left!" << END_LOG;
                        }
                    }

                    /**
                     * Allows to set the new number of worker threads.
                     * This operation should be safe as the new threads
                     * are just added to the list and the deleted ones
                     * are let to finish their translation task execution. 
                     * @param num_threads the new number of worker threads
                     */
                    inline void set_num_threads(const size_t num_threads) {
                        m_tasks_pool.set_num_threads(num_threads);
                    }

                    /**
                     * Allows to report the runtime information.
                     */
                    inline void report_run_time_info() {
                        //Remove the job from the pool's administration 
                        {
                            recursive_guard guard_all_jobs(m_all_jobs_lock);

                            LOG_USAGE << "#open sessions: " << m_sessions_map.size()
                                    << ", #scheduled jobs: " << m_job_count << END_LOG;
                        }

                        //Report data from the tasks pool
                        m_tasks_pool.report_run_time_info("Translation tasks pool");
                    }

                    /**
                     * Allows to set the response sender function for sending the replies to the client
                     * @param notify_job_finished_func the setter functional to be set
                     */
                    inline void set_job_result_setter(finished_job_notifier notify_job_finished_func) {
                        m_notify_job_finished_func = notify_job_finished_func;
                    }

                    /**
                     * Allows to schedule a new translation job.
                     * The execution of the job is deferred and asynchronous.
                     * @oaram trans_job the translation job to be scheduled
                     */
                    inline void plan_new_job(trans_job_ptr trans_job) {
                        //Make sure that we are not being stopped before or during this method call
                        scoped_guard guard_stopping(m_stopping_lock);

                        LOG_DEBUG << "Request adding a new job " << trans_job << " to the pool!" << END_LOG;

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
                    inline void cancel_jobs(const session_id_type session_id) {
                        recursive_guard guard_all_jobs(m_all_jobs_lock);

                        LOG_DEBUG << "Canceling the jobs of session: " << session_id << END_LOG;

                        //Get the jobs jobs for the given session
                        if (m_sessions_map.find(session_id) != m_sessions_map.end()) {
                            //Get the known jobs map for the given session
                            jobs_map_type & m_jobs_map = m_sessions_map[session_id];

                            //Iterate through the available jobs and cancel them
                            for (jobs_map_iter_type iter = m_jobs_map.begin(); iter != m_jobs_map.end(); ++iter) {
                                //Get the translation job for future use
                                trans_job_ptr job = iter->second;
                                LOG_DEBUG << "Canceling the translation job " << this << " with id " << job->get_job_id() << END_LOG;
                                //Call the canceling method on the job
                                job->cancel();
                            }
                            LOG_DEBUG << "Finished canceling the jobs of session: " << session_id << END_LOG;
                        } else {
                            LOG_DEBUG << "The session with id: " << session_id << " has no jobs!" << END_LOG;
                        }
                    }

                protected:

                    /**
                     * Allows to cancel all the currently running translation jobs in the server
                     */
                    inline void cancel_all_jobs() {
                        recursive_guard guard_all_jobs(m_all_jobs_lock);

                        LOG_DEBUG << "Start canceling all server jobs" << END_LOG;

                        //Cancel the scheduled translation tasks and stop the internal thread
                        for (sessions_map_iter_type iter = m_sessions_map.begin(); iter != m_sessions_map.end(); ++iter) {
                            //Call the canceling method on the task
                            cancel_jobs(iter->first);
                        }

                        LOG_DEBUG << "Finished canceling all server jobs" << END_LOG;
                    }

                    /**
                     * Allows to add a new job to the administration. In case the
                     * session is not known or the job id is already in use an
                     * exception is thrown. Also the job count is incremented
                     * \todo {Later, the tasks pool shall be chosen based on the
                     * source and target language. This is for when a server can
                     * translate from multiple languages to multiple languages.}
                     * @param trans_job the job to be added to the administration
                     */
                    inline void add_job(trans_job_ptr trans_job) {
                        recursive_guard guard_all_jobs(m_all_jobs_lock);

                        LOG_DEBUG << "Adding the job with ptr: " << trans_job << " to the job pool" << END_LOG;

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

                        //Add the job tasks to the tasks' pool
                        const trans_job::tasks_list_type& tasks = trans_job->get_tasks();
                        for (trans_job::tasks_const_iter_type it = tasks.begin(); it != tasks.end(); ++it) {
                            m_tasks_pool.plan_new_task(*it);
                        }

                        LOG_DEBUG << "Adding the job with ptr: " << trans_job << " to the job pool is done!" << END_LOG;
                    }

                    /**
                     * Allows to delete the given job from the administration,
                     * decrement the jobs count and destroy the job object.
                     * @param trans_job the job to be deleted
                     */
                    inline void delete_job(trans_job_ptr trans_job) {
                        //Get and store the session and job ids for later use
                        const session_id_type session_id = trans_job->get_session_id();
                        const job_id_type job_id = trans_job->get_job_id();

                        LOG_DEBUG << "Requested the job " << trans_job << " deletion from the administration" << END_LOG;

                        //Remove the job from the pool's administration 
                        {
                            recursive_guard guard_all_jobs(m_all_jobs_lock);

                            //Erase the job from the jobs mapping
                            m_sessions_map[session_id].erase(job_id);

                            //If there are not jobs left for this session,
                            //then remove the mapping to save space.
                            if (m_sessions_map[session_id].empty()) {
                                m_sessions_map.erase(session_id);
                            }

                            //Decrement the jobs count 
                            m_job_count--;

                            LOG_DEBUG << "Deleted the job " << trans_job << " from the administration, the jobs count is " << m_job_count << END_LOG;
                        }

                        //Make sure that the job-finished notification is indeed complete
                        trans_job->synch_job_finished();

                        //Delete the job object itself
                        delete trans_job;

                        LOG_DEBUG << "Deleted the job " << job_id << " object instance" << END_LOG;
                    }

                    /**
                     * Allows to check if the finished jobs processing loop has to stop.
                     * @return true if the finished jobs processing loop has to stop, otherwise false
                     */
                    inline bool is_stop_running() {
                        LOG_DEBUG << "is_stop_running check requested" << END_LOG;
                        {
                            //Make sure that we are not being stopped before or during this method call
                            scoped_guard guard_stopping(m_stopping_lock);

                            LOG_DEBUG << "m_stopping_lock lock is passed" << END_LOG;
                            {
                                //Make sure not one adds/removes jobs meanwhile
                                recursive_guard guard_all_jobs(m_all_jobs_lock);

                                LOG_DEBUG << "m_all_jobs_lock lock is passed" << END_LOG;

                                //We shall stop if we are being asked to stop and there are no jobs left
                                const bool result = (m_is_stopping && m_job_count == 0);

                                LOG_DEBUG << "is_stop_running = " << to_string(result)
                                        << " is stopping flag: " << to_string(m_is_stopping)
                                        << " active jobs count: " << m_job_count << END_LOG;

                                return result;
                            }
                        }
                    }

                    /**
                     * Allows to wake up the thread processing the finished jobs.
                     */
                    inline void start_processing_finished_jobs() {
                        unique_guard guard_finished_jobs(m_finished_jobs_lock);

                        //Notify the thread that there is a finished job to be processed
                        m_is_job_done.notify_one();
                    }

                    /**
                     * Allows notify the job pool that the given job is done.
                     * @param trans_job the pointer to the finished translation job 
                     */
                    inline void notify_job_done(trans_job_ptr trans_job) {
                        LOG_DEBUG1 << "The job " << trans_job << " has called in finished!" << END_LOG;
                        {
                            unique_guard guard_finished_jobs(m_finished_jobs_lock);

                            LOG_DEBUG << "The job " << trans_job << " is finished!" << END_LOG;

                            //Add the job to the finished jobs list
                            m_done_jobs_list.push_back(trans_job);

                            //Notify the thread that there is a finished job to be processed
                            m_is_job_done.notify_one();
                        }
                        LOG_DEBUG1 << "The job " << trans_job << " is marked as finished!" << END_LOG;
                    }

                    /**
                     * Allows to process the finished translation jobs
                     */
                    inline void process_finished_jobs() {
                        unique_guard guard_finished_jobs(m_finished_jobs_lock);

                        //Stop iteration only when we are stopping and there are no jobs left
                        while (!is_stop_running()) {
                            //Wait the thread to be notified
                            m_is_job_done.wait(guard_finished_jobs);

                            LOG_DEBUG << "Processing finished jobs!" << END_LOG;

                            //The thread is notified, process the finished jobs
                            for (jobs_list_iter_type iter = m_done_jobs_list.begin(); iter != m_done_jobs_list.end();
                                    /*The shift is done by itself when erasing the element!*/) {

                                //Get the translation job pointer
                                trans_job_ptr trans_job = *iter;

                                LOG_DEBUG << "The finished job " << trans_job << " id is " << trans_job->get_job_id() <<
                                        " session id is " << trans_job->get_session_id() << END_LOG;

                                //Do the sanity check assert
                                ASSERT_SANITY_THROW(!m_notify_job_finished_func,
                                        "The job pool's result setting function is not set!");

                                //Send the job response
                                m_notify_job_finished_func(trans_job);

                                LOG_DEBUG << "Erasing the job " << trans_job << " from the done jobs list" << END_LOG;

                                //Erase the processed job from the list of finished jobs
                                m_done_jobs_list.erase(iter);

                                LOG_DEBUG << "Deleting the job " << trans_job << " instance" << END_LOG;

                                //Remove the job from the pool's administration and destroy
                                delete_job(trans_job);

                                LOG_DEBUG << "The job " << trans_job << " is processed and deleted!" << END_LOG;
                            }

                            LOG_DEBUG << "Finished processing finished jobs, back to sleep or stop!" << END_LOG;
                        }
                    }

                private:
                    //Stores the tasks pool
                    task_pool<trans_task> m_tasks_pool;

                    //Stores the synchronization mutex for working with the m_sessions_map
                    recursive_mutex m_all_jobs_lock;

                    //Stores the mapping from the session id to the active translation jobs from the session
                    sessions_map_type m_sessions_map;

                    //Stores the reply sender functional
                    finished_job_notifier m_notify_job_finished_func;

                    //Stores the synchronization mutex for administering stopping
                    mutex m_stopping_lock;
                    //Stores the flag that indicates that we are stopping, made an atomic just in case
                    a_bool_flag m_is_stopping;
                    //Stores the active jobs count, made an atomic just in case
                    atomic<uint64_t> m_job_count;

                    //Stores the synchronization mutex for working with the conditional wait/notify
                    mutex m_finished_jobs_lock;
                    //The conditional variable for tracking the done jobs
                    condition_variable m_is_job_done;

                    //Stores the thread that manages finished jobs
                    thread m_jobs_thread;

                    //The list of finished jobs pending to be processed
                    jobs_list_type m_done_jobs_list;
                };
            }
        }
    }
}

#endif /* TRANS_JOB_POOL_HPP */

