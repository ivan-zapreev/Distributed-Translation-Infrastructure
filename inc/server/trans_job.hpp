/* 
 * File:   trans_job.hpp
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
 * Created on January 21, 2016, 4:08 PM
 */

#ifndef TRANS_JOB_HPP
#define TRANS_JOB_HPP

#include <string>
#include <vector>

#include "trans_task.hpp"
#include "common/utils/threads.hpp"
#include "common/messaging/trans_session_id.hpp"
#include "common/messaging/trans_job_request.hpp"
#include "common/messaging/trans_job_id.hpp"
#include "common/messaging/status_code.hpp"

using namespace std;
using namespace std::placeholders;
using namespace uva::smt::bpbd::common::messaging;
using namespace uva::utils::threads;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {

                //Declare the translation job pointer
                class trans_job;
                typedef trans_job* trans_job_ptr;

                /**
                 * This class represents the translation job. Each translation job
                 * belongs to a session and contains a translation request.
                 * Every translation request is a text consisting of multiple sentences.
                 * The translation job therefore splits this request into a number
                 * of translation tasks each of which translates one sentence.
                 */
                class trans_job {
                public:

                    //Define the function type for the function used to set the translation job resut
                    typedef function<void(trans_job_ptr trans_job) > done_job_notifier;

                    //Declare the tasks list type and its iterator
                    typedef vector<trans_task_ptr> tasks_list_type;
                    typedef tasks_list_type::iterator tasks_iter_type;
                    typedef tasks_list_type::const_iterator tasks_const_iter_type;

                    /**
                     * The basic constructor allowing to initialize the main class constants
                     * @param session_id the id of the session from which the translation request is received
                     * @param trans_req the reference to the translation job request to get the data from.
                     */
                    trans_job(const session_id_type session_id, const trans_job_request & trans_req)
                    : m_session_id(session_id), m_job_id(trans_req.get_job_id()),
                    m_is_trans_info(trans_req.is_trans_info()), m_done_tasks_count(0),
                    m_status_code(status_code::RESULT_UNDEFINED),
                    m_status_msg(""), m_target_text("") {
                        LOG_DEBUG << "Creating a new translation job " << this << " with job_id: "
                                << m_job_id << " session id: " << m_session_id << END_LOG;

                        //Get the text to be translated
                        auto source_text = trans_req.get_source_text();

                        //Read the text line by line, each line must be one sentence
                        //to translate. For each read line create a translation task.
                        for (size_t idx = 0; idx < source_text.size(); ++idx) {
                            m_tasks.push_back(new trans_task(m_session_id, m_job_id,
                                    source_text[idx], bind(&trans_job::notify_task_done, this, _1)));
                        }
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
                     * The basic destructor
                     */
                    virtual ~trans_job() {
                        LOG_DEBUG << "Deleting the translation job " << this << " with job_id: "
                                << m_job_id << " session id: " << m_session_id << END_LOG;

                        LOG_DEBUG << "Start deleting translation tasks of job " << this << END_LOG;

                        //Delete the translation tasks
                        for (tasks_iter_type it = m_tasks.begin(); it != m_tasks.end(); ++it) {
                            LOG_DEBUG << "Deleting translation tasks" << this->get_job_id() << "/" << **it << END_LOG;
                            delete *it;
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
                     * Allows to retrieve the job id
                     * @return the job id
                     */
                    inline job_id_type get_job_id() const {
                        return m_job_id;
                    }

                    /**
                     * Allows to get the list of translation tasks
                     * @return the list of translation tasks of this job
                     */
                    inline const tasks_list_type& get_tasks() const {
                        return m_tasks;
                    }

                    /**
                     * Allows to retrieve the translation task result code
                     * @return the translation task result code
                     */
                    inline status_code get_status_code() const {
                        return m_status_code;
                    }

                    /**
                     * Allows to retrieve the translation job error message
                     * @return the translation job error message
                     */
                    inline const string & get_status_msg() const {
                        return m_status_msg;
                    }

                    /**
                     * Allows to retrieve the translation task result text
                     * @return the translation task result text
                     */
                    inline const string & get_target_text() const {
                        return m_target_text;
                    }

                    /**
                     * Allows to cancel the given translation job by telling all the translation tasks to stop.
                     */
                    inline void cancel() {
                        //Note: no need to synchronize this method as the tasks list is
                        //created in the constructor and deleted in the destructor.

                        //Iterate through the translation tasks and cancel them
                        for (tasks_iter_type it = m_tasks.begin(); it != m_tasks.end(); ++it) {
                            //Cancel the translation task
                            (*it)->cancel();
                        }
                    }

                    /**
                     * Allows to wait until the job is finished, this
                     * includes the notification of the job pool.
                     */
                    inline void synch_job_finished() {
                        recursive_guard guard(m_tasks_lock);
                    }

                protected:

                    /**
                     * Allows to check if the job is finished by checking the number
                     * of finished tasks. The check is NOT synchronized.
                     * @return true if all the job's tasks are finished, otherwise false
                     */
                    inline bool is_job_finished() {
                        LOG_DEBUG1 << "Checking if the job " << this << " is finished!" << END_LOG;

                        LOG_DEBUG1 << "The number of active tasks of job " << this
                                << " is: " << (m_tasks.size() - m_done_tasks_count)
                                << "/" << m_tasks.size() << END_LOG;

                        return (m_done_tasks_count == m_tasks.size());
                    }


                    /**
                     * Is used from the translation task to notify the translation
                     * job that the task is ready. This method is thread safe.
                     * \todo {Do a strict check on the tasks reporting to be finished,
                     * these should be the ones from the m_tasks list and they must
                     * report themselves only ones. (Optional - for safety).}
                     * @param task the translation task that is finished
                     */
                    inline void notify_task_done(const trans_task_ptr task) {
                        LOG_DEBUG1 << "The task " << *task << " is done!" << END_LOG;

                        {
                            recursive_guard guard_tasks(m_tasks_lock);

                            //Increment the finished tasks count
                            m_done_tasks_count++;

                            LOG_DEBUG1 << "The number of finished tasks of job " << this << " is "
                                    << m_done_tasks_count << "/" << m_tasks.size() << END_LOG;

                            //If all the tasks are translated
                            if (is_job_finished()) {
                                LOG_DEBUG << "The translation job " << this << " is finished!" << END_LOG;

                                //Do the sanity check assert
                                ASSERT_SANITY_THROW(!m_notify_job_done_func,
                                        "The translation job's result setting function is not set!");

                                LOG_DEBUG << "Notifying the manager that the job " << this << " is finished!" << END_LOG;

                                //Notify that this job id done
                                m_notify_job_done_func(this);
                            }
                        }
                    }

                public: 
                    
                    /**
                     * Allows to compile the end job result, e.g. based on the task results,
                     * come up with the job's result code and the translated text.
                     * @param data [out] the object to store the translation job response data to be sent
                     */
                    inline void collect_job_results(trans_job_resp_data & data) {
                        LOG_DEBUG << "Combining the job " << this << " result!" << END_LOG;

                        //ToDo: Set the data into the translation job response data
                        
                        //Declare the variables for counting the number of CANCELED/ERROR tasks
                        uint32_t num_canceled = 0, num_error = 0;

                        //Define the translation task info object
                        trans_info info;

                        //Iterate through the translation tasks and combine the results
                        for (tasks_iter_type it = m_tasks.begin(); it != m_tasks.end(); ++it) {
                            //Get the task pointer for future use
                            trans_task_ptr task = *it;

                            //Get the translation job result
                            const status_code status = task->get_status_code();

                            //Count the number of CANCELLED/ERROR tasks
                            switch (status) {
                                case status_code::RESULT_ERROR:
                                    num_error++;
                                    break;
                                case status_code::RESULT_CANCELED:
                                    num_canceled++;
                                    break;
                                default:
                                    break;
                            }

                            //Append the next task result
                            m_target_text += task->get_target_text() + "\n";

                            //Append the task translation info if needed and the translation was finished
                            if (m_is_trans_info && (status == status_code::RESULT_OK)) {
                                //Get the translation task info
                                task->get_trans_info(info);
                                m_target_text += info.serialize() + "\n";
                            }

                            LOG_DEBUG1 << "The target text of task: " << *task << " has been retrieved!" << END_LOG;
                        }

                        LOG_DEBUG2 << "The translation job " << this << " result is:\n" << m_target_text << END_LOG;

                        //Remove the last sentence new line symbol
                        m_target_text.substr(0, m_target_text.size() - 1);

                        //Decide on the result code
                        if ((num_canceled == 0) && (num_error == 0)) {
                            //If there is no canceled jobs then the result is good
                            m_status_code = status_code::RESULT_OK;
                            m_status_msg = "The text was fully translated!";
                        } else {
                            if (num_canceled == m_done_tasks_count) {
                                //All of the tasks have been canceled
                                m_status_code = status_code::RESULT_CANCELED;
                                m_status_msg = "The translation job has been canceled!";
                            } else {
                                //Some of the sentences were translated but not all of them
                                m_status_code = status_code::RESULT_PARTIAL;
                                m_status_msg = "The text was partially translated!";
                            }
                        }

                        LOG_DEBUG << "The translation job " << this << " result is ready!" << END_LOG;
                    }

                private:
                    //Stores the synchronization mutex for working with the m_sessions_map
                    recursive_mutex m_tasks_lock;

                    //Stores the translation client session id
                    const session_id_type m_session_id;

                    //Stores the translation job id
                    const job_id_type m_job_id;

                    //Stores the flag of whether the translation info is to be sent back or not
                    const bool m_is_trans_info;

                    //The count of the finished tasks
                    atomic<uint32_t> m_done_tasks_count;

                    //The done job notifier
                    done_job_notifier m_notify_job_done_func;

                    //Stores the list of translation tasks of this job
                    tasks_list_type m_tasks;

                    //Stores the translation job result code
                    status_code m_status_code;

                    //Stores the translation job error message
                    string m_status_msg;

                    //Stores the translation job result text
                    string m_target_text;
                };
            }
        }
    }
}

#endif /* TRANS_JOB_RESULT_HPP */

