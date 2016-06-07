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
#include "common/messaging/trans_job_code.hpp"

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
                     * @param job_id the translation job id
                     * @param task_ids the list of task ids from which this job consists of
                     */
                    trans_job(trans_job_request_ptr request_ptr)
                    : m_request_ptr(request_ptr), m_done_tasks_count(0), m_code(trans_job_code::RESULT_UNDEFINED), m_target_text("") {
                        LOG_DEBUG << "Creating a new translation job " << this << " with job_id: "
                                << m_request_ptr->get_job_id() << " session id: "
                                << m_request_ptr->get_session_id() << END_LOG;

                        //Get the text to be translated
                        string text = m_request_ptr->get_text();
                        //Obtain the text to be parsed
                        text_piece_reader reader(text.c_str(), text.length());
                        //This reader will store the read text sentence
                        text_piece_reader sentence;

                        //Get the session and job id for the translation task constructor
                        const session_id_type session_id = request_ptr->get_session_id();
                        const job_id_type job_id = request_ptr->get_job_id();

                        //Read the text line by line, each line must be one sentence
                        //to translate. For each read line create a translation task.
                        while (reader.get_first<trans_job_request::TEXT_SENTENCE_DELIMITER>(sentence)) {
                            m_tasks.push_back(new trans_task(session_id, job_id,
                                    sentence.str(), bind(&trans_job::notify_task_done, this, _1)));
                        }
                    }

                    /**
                     * Allows to set the function that should be called when the job is done
                     * @param notify_job_done_func
                     */
                    void set_done_job_notifier(done_job_notifier notify_job_done_func) {
                        m_notify_job_done_func = notify_job_done_func;
                    }

                    /**
                     * The basic destructor
                     */
                    virtual ~trans_job() {
                        LOG_DEBUG << "Deleting the translation job " << this << " with job_id: "
                                << m_request_ptr->get_job_id() << " session id: "
                                << m_request_ptr->get_session_id() << END_LOG;

                        LOG_DEBUG << "Start deleting translation tasks of job " << this << END_LOG;

                        //Delete the translation tasks
                        for (tasks_iter_type it = m_tasks.begin(); it != m_tasks.end(); ++it) {
                            LOG_DEBUG << "Deleting translation tasks" << this->get_job_id() << "/" << **it << END_LOG;
                            delete *it;
                        }

                        LOG_DEBUG << "The translation tasks of job " << this << " are deleted" << END_LOG;

                        //If the job with its tasks is deleted then the request is not needed any more
                        if (m_request_ptr != NULL) {
                            delete m_request_ptr;
                        }
                    }

                    /**
                     * Allows to retrieve the session id
                     * @return the session id
                     */
                    const session_id_type get_session_id() const {
                        return m_request_ptr->get_session_id();
                    }

                    /**
                     * Allows to retrieve the job id
                     * @return the job id
                     */
                    const job_id_type get_job_id() const {
                        return m_request_ptr->get_job_id();
                    }

                    /**
                     * Allows to get the list of translation tasks
                     * @return the list of translation tasks of this job
                     */
                    const tasks_list_type& get_tasks() {
                        return m_tasks;
                    }

                    /**
                     * Allows to retrieve the translation task result code
                     * @return the translation task result code
                     */
                    virtual const trans_job_code get_code() const {
                        return m_code;
                    }

                    /**
                     * Allows to retrieve the translation task result text
                     * @return the translation task result text
                     */
                    virtual const string & get_text() const {
                        return m_target_text;
                    }

                    /**
                     * Allows to cancel the given translation job by telling all the translation tasks to stop.
                     */
                    void cancel() {
                        //Note: no need to synchronize this method as the tasks list is
                        //created in the constructor and deleted in the destructor.

                        //Iterate through the translation tasks and cancel them
                        for (tasks_iter_type it = m_tasks.begin(); it != m_tasks.end(); ++it) {
                            //Cancel the translation task
                            (*it)->cancel();
                        }
                    }

                    /**
                     * Allows to wait until the notification of that this job is finished is complete
                     */
                    void wait_notify_finished() {
                        recursive_guard guard(m_tasks_lock);
                    }

                protected:

                    /**
                     * Allows to check if the job is finished by checking the number
                     * of finished tasks. The check is synchronized.
                     * @return true if all the job's tasks are finished, otherwise false
                     */
                    bool is_job_finished() {
                        LOG_DEBUG1 << "Checking if the job " << this << " is finished!" << END_LOG;
                        {
                            recursive_guard guard_tasks(m_tasks_lock);

                            LOG_DEBUG1 << "The number of active tasks of job " << this
                                    << " is: " << (m_tasks.size() - m_done_tasks_count)
                                    << "/" << m_tasks.size() << END_LOG;

                            return (m_done_tasks_count == m_tasks.size());
                        }
                    }

                    /**
                     * Is used from the translation task to notify the translation
                     * job that the task is ready. This method is thread safe.
                     * \todo {Do a strict check on the tasks reporting to be finished,
                     * these should be the ones from the m_tasks list and they must
                     * report themselves only ones. (Optional - for safety).}
                     * @param task the translation task that is finished
                     */
                    void notify_task_done(const trans_task_ptr task) {
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

                                //Combine the task results into the job result
                                if (m_request_ptr->is_trans_info()) {
                                    combine_job_result<true>();
                                } else {
                                    combine_job_result<false>();
                                }

                                //Do the sanity check assert
                                ASSERT_SANITY_THROW(!m_notify_job_done_func,
                                        "The translation job's result setting function is not set!");

                                LOG_DEBUG << "Notifying the manager that the job " << this << " is finished!" << END_LOG;

                                //Notify that this job id done
                                m_notify_job_done_func(this);
                            }
                        }
                    }

                    /**
                     * Allows to compile the end job result, e.g. based on the task results,
                     * come up with the job's result code and the translated text.
                     */
                    template<bool is_trans_info>
                    void combine_job_result() {
                        LOG_DEBUG << "Combining the job " << this << " result!" << END_LOG;

                        //Declare the variable for counting the number of CANCELED tasks
                        uint32_t num_canceled = 0;

                        //Define the translation task info object
                        trans_info info;

                        //Iterate through the translation tasks and combine the results
                        for (tasks_iter_type it = m_tasks.begin(); it != m_tasks.end(); ++it) {
                            //Get the task pointer for future use
                            trans_task_ptr task = *it;

                            //Count the number of canceled tasks and leave text as an empty line then
                            if (task->get_code() == trans_job_code::RESULT_CANCELED) {
                                num_canceled++;
                            }

                            //Append the next task result
                            m_target_text += task->get_target_text() + "\n";

                            //Append the task translation info if needed
                            if (is_trans_info) {
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
                        if (num_canceled == 0) {
                            //If there is no canceled jobs then the result is good
                            m_code = trans_job_code::RESULT_OK;
                        } else {
                            if (num_canceled == m_done_tasks_count) {
                                //All of the tasks have been canceled
                                m_code = trans_job_code::RESULT_CANCELED;
                            } else {
                                //Some of the sentences were translated but not all of them
                                m_code = trans_job_code::RESULT_PARTIAL;
                            }
                        }

                        LOG_DEBUG << "The translation job " << this << " result is ready!" << END_LOG;
                    }

                private:
                    //Stores the synchronization mutex for working with the m_sessions_map
                    recursive_mutex m_tasks_lock;

                    //The done job notifier
                    done_job_notifier m_notify_job_done_func;

                    //Stores the original translation request
                    trans_job_request_ptr m_request_ptr;

                    //Stores the list of translation tasks of this job
                    tasks_list_type m_tasks;

                    //The count of the finished tasks
                    atomic<uint32_t> m_done_tasks_count;

                    //Stores the translation job result code
                    trans_job_code m_code;

                    //Stores the translation job result text
                    string m_target_text;
                };
            }
        }
    }
}

#endif /* TRANS_JOB_RESULT_HPP */

