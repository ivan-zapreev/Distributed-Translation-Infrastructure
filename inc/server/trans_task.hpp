/* 
 * File:   trans_task.hpp
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
 * Created on January 21, 2016, 10:39 AM
 */

#include <websocketpp/common/thread.hpp>

using namespace std;

#ifndef TRANS_TASK_HPP
#define TRANS_TASK_HPP

namespace uva {
    namespace smt {
        namespace decoding {
            namespace server {

                //Define the translation task pointer
                class trans_task;
                typedef trans_task * trans_task_ptr;

                /**
                 * This class represents the translation task. Every translation task is a sentence to be translated and its id.
                 */
                class trans_task {
                public:
                    //Define the lock type to synchronize map operations
                    typedef websocketpp::lib::lock_guard<websocketpp::lib::mutex> scoped_lock;
                    //Define the done job notifier function type
                    typedef websocketpp::lib::function<void(trans_task_ptr) > done_task_notifier;
                    //Define the canceled job notifier function type
                    typedef websocketpp::lib::function<void(trans_task_ptr) > cancel_task_notifier;

                    /**
                     * The basic constructor allowing to initialize the main class constants
                     * @param task_id the id of the translation task within the translation job
                     * @param source_sentence the sentence to be translated
                     */
                    trans_task(const task_id_type task_id, const string & source_sentence, done_task_notifier notify_task_done_func)
                    : m_is_stop(false), m_task_id(task_id), m_code(trans_job_code::RESULT_UNDEFINED),
                    m_source_text(source_sentence), m_target_text(""), m_notify_task_done_func(notify_task_done_func) {
                        LOG_DEBUG << "Creating a translation task with id: " << m_task_id
                                << ", text: " << m_source_text << END_LOG;
                    }

                    /**
                     * The basic destructor
                     */
                    virtual ~trans_task() {
                        //Nothing to be done
                    }

                    /**
                     * Allows to set the function which must be called by the tasks if it is being cancelled.
                     * @param notify_task_cancel_func the function to call in case this task is being cancelled.
                     */
                    void set_cancel_task_notifier(cancel_task_notifier notify_task_cancel_func) {
                        m_notify_task_cancel_func = notify_task_cancel_func;
                    }

                    /**
                     * Allows to cancel the translation task
                     */
                    void cancel() {
                        scoped_lock guard_cancel(m_cancel_lock);

                        //Set the stopping flag to true
                        m_is_stop = true;

                        //Call the cancel notifier
                        m_notify_task_cancel_func(this);
                    }

                    /**
                     * Performs the translation for the given sentence
                     */
                    void translate() {
                        //ToDo: Implement, implement the translation process
                        string trans_result = "--------/Some sentence translation/-------";

                        //If the task is not canceled at this point yet then prevent it from being canceled in parallel
                        {
                            scoped_lock guard_cancel(m_cancel_lock);

                            //Set the task is not canceled then set the result, otherwise set the canceled code.
                            if (!m_is_stop) {
                                m_code = trans_job_code::RESULT_OK;
                                m_target_text = trans_result;
                            } else {
                                m_code = trans_job_code::RESULT_CANCELED;
                            }
                        }

                        //Call the task-done notification function to report that we are finished!s
                        m_notify_task_done_func(this);
                    }

                    /**
                     * Allows to retrieve the task id
                     * @return the task id
                     */
                    const task_id_type get_task_id() const {
                        return m_task_id;
                    }

                    /**
                     * Allows to retrieve the translation task result code
                     * @return the translation task result code
                     */
                    virtual const trans_job_code get_code() const {
                        return m_code;
                    }

                    /**
                     * Allows to retrieve the sentence in the source language
                     * @return the sentence in the source language
                     */
                    virtual const string & get_source_text() const {
                        return m_source_text;
                    }

                    /**
                     * Allows to retrieve the sentence in the target language or an error message
                     * @return the sentence in the target language or an error message
                     */
                    virtual const string & get_target_text() const {
                        return m_target_text;
                    }

                private:
                    //Stores the flag that indicates that we need to stop the translation algorithm
                    atomic<bool> m_is_stop;

                    //Stores the translation task id
                    const task_id_type m_task_id;

                    //Stores the translation task result code
                    trans_job_code m_code;

                    //Stores the sentence to be translated
                    const string m_source_text;

                    //Stores the translated sentence or an error message
                    string m_target_text;

                    //Stores the task-done notifier function for this task
                    done_task_notifier m_notify_task_done_func;

                    //Stores the synchronization mutex for synchronization on task cancelling
                    websocketpp::lib::mutex m_cancel_lock;

                    //Stores the function to be called in case the tasks is cancelled
                    cancel_task_notifier m_notify_task_cancel_func;
                };
            }
        }
    }
}

#endif /* TRANS_TASK_HPP */

