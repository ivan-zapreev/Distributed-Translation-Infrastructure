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

#ifndef TRANS_TASK_HPP
#define TRANS_TASK_HPP

#include "common/utils/threads.hpp"
#include "common/utils/logging/logger.hpp"
#include "common/messaging/trans_session_id.hpp"
#include "common/messaging/trans_job_id.hpp"
#include "common/messaging/trans_job_code.hpp"
#include "trans_task_id.hpp"

#include "server/decoder/de_configurator.hpp"
#include "server/decoder/sentence/sentence_decoder.hpp"

using namespace std;

using namespace uva::utils::threads;
using namespace uva::utils::logging;

using namespace uva::smt::bpbd::common::messaging;

using namespace uva::smt::bpbd::server::decoder;
using namespace uva::smt::bpbd::server::decoder::sentence;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {

                //Define the translation task pointer
                class trans_task;
                typedef trans_task * trans_task_ptr;

                /**
                 * This class represents the translation task. Every translation task is a sentence to be translated and its id.
                 */
                class trans_task {
                public:
                    //Define the done job notifier function type
                    typedef function<void(trans_task_ptr) > done_task_notifier;
                    //Define the canceled job notifier function type
                    typedef function<void(trans_task_ptr) > cancel_task_notifier;

                    /**
                     * The basic constructor allowing to initialize the main class constants
                     * @param session_id the session id of the task, is used for logging
                     * @param job_id the job id of the task, is used for logging
                     * @param task_id the id of the translation task within the translation job
                     * @param source_sentence the sentence to be translated
                     */
                    trans_task(const session_id_type session_id, const job_id_type job_id, const task_id_type task_id,
                            const string & source_sentence, done_task_notifier notify_task_done_func)
                    : m_is_interrupted(false), m_session_id(session_id), m_job_id(job_id),
                    m_task_id(task_id), m_code(trans_job_code::RESULT_UNDEFINED), m_source_text(source_sentence),
                    m_notify_task_done_func(notify_task_done_func), m_target_text("") {
                        LOG_DEBUG1 << "/session id=" << m_session_id << ", job id="
                                << m_job_id << ", NEW task id=" <<  m_task_id
                                << "/ text: " << m_source_text << END_LOG;
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
                        LOG_DEBUG << "Setting the cancel task notifier function!" << END_LOG;
                        m_notify_task_cancel_func = notify_task_cancel_func;
                    }

                    /**
                     * Allows to cancel the translation task
                     */
                    void cancel() {
                        LOG_DEBUG1 << "Canceling the task " << m_task_id << " translation ..." << END_LOG;

                        //Synchronize to avoid canceling the job that is already finished.
                        {
                            recursive_guard guard_end(m_end_lock);

                            LOG_DEBUG1 << "The task " << m_task_id << " is to be interrupted!" << END_LOG;

                            //Set the stopping flag to true
                            m_is_interrupted = true;

                            //Do nothing else, just set the flag, that is needed to avoid deadlocking
                            //The finished task, and job processing is to be done from a separate thread.
                        }

                        LOG_DEBUG1 << "The task " << m_task_id << " translation is canceled!" << END_LOG;
                    }

                    /**
                     * Performs the translation for the given sentence
                     */
                    void translate() {
                        LOG_DEBUG1 << "Starting the task " << m_task_id << " translation ..." << END_LOG;

                        //Obtain the new decoder instance
                        sentence_decoder & dec = de_configurator::allocate_decoder(m_is_interrupted, m_source_text, m_target_text);

                        LOG_DEBUG1 << "Invoking the sentence translation for task " << m_task_id << END_LOG;
                        
                        //Perform the decoding task
                        try {
                            dec.translate();
                        } catch (Exception & ex) {
                            //Set the response code
                            m_code = trans_job_code::RESULT_ERROR;
                            //Set the error message to be the target text to be sent back
                            m_target_text = ex.get_message();
                            //Do local logging
                            LOG_DEBUG << "SERVER ERROR: " << m_target_text << END_LOG;
                        }

                        //Dispose the decoder instance 
                        de_configurator::dispose_decoder(dec);

                        LOG_DEBUG1 << "The task " << m_task_id << " translation part is over." << END_LOG;

                        //Synchronize to avoid canceling the job that is already finished.
                        {
                            recursive_guard guard_end(m_end_lock);

                            LOG_DEBUG1 << "The task " << m_task_id << " is to be finished!" << END_LOG;

                            //Produce the task result
                            process_task_result();
                        }

                        LOG_DEBUG1 << "The task " << m_task_id << " translation is done!" << END_LOG;
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
                    const trans_job_code get_code() const {
                        return m_code;
                    }

                    /**
                     * Allows to retrieve the sentence in the source language
                     * @return the sentence in the source language
                     */
                    const string & get_source_text() const {
                        return m_source_text;
                    }

                    /**
                     * Allows to retrieve the sentence in the target language or an error message
                     * @return the sentence in the target language or an error message
                     */
                    const string & get_target_text() {
                        LOG_DEBUG1 << "Retrieving the target text of task: " << m_task_id << END_LOG;
                        {
                            recursive_guard guard_end(m_end_lock);

                            return m_target_text;
                        }
                    }

                protected:

                    /**
                     * Allows to process the translation task result in case of a successful and abnormal task termination.
                     * This includes sending the notification to the translation job that the task is finished.
                     * NOTE: This method is not thread safe!
                     */
                    void process_task_result() {
                        //Set the task is not canceled then set the result, otherwise set the canceled code.
                        if (m_code == trans_job_code::RESULT_ERROR) {
                            //If there was an error during translation send back the error message stored in the target texts
                            m_target_text = string("<error>: ") + m_target_text;
                        } else {
                            //Unless it was an error while translating there should be an undefined status
                            ASSERT_SANITY_THROW((m_code != trans_job_code::RESULT_UNDEFINED),
                                    string("Unexpected translation code: ") + to_string(m_code) +
                                    string(" must be UNDEFINED!"));

                            //Check if we were interrupted or not
                            if (m_is_interrupted) {
                                //If the translation has been canceled just send back the source
                                m_code = trans_job_code::RESULT_CANCELED;
                                m_target_text = string("<canceled>: ") + m_source_text;
                            } else {
                                //If the translation has been finished send back the target
                                m_code = trans_job_code::RESULT_OK;
                                m_target_text = string("<finished>: ") + m_target_text;
                            }
                        }

                        LOG_DEBUG1 << "The task " << m_task_id << " translation is done, notifying!" << END_LOG;

                        //Call the task-done notification function to report that we are finished!s
                        m_notify_task_done_func(this);
                    }

                private:
                    //Stores the flag that indicates that we need to stop the translation algorithm
                    atomic<bool> m_is_interrupted;

                    //Stores the translation task session id, is needed for logging
                    const session_id_type m_session_id;

                    //Stores the translation task job id, is needed for logging
                    const job_id_type m_job_id;

                    //Stores the translation task id
                    const task_id_type m_task_id;

                    //Stores the translation task result code
                    trans_job_code m_code;

                    //Stores the sentence to be translated
                    const string m_source_text;

                    //Stores the task-done notifier function for this task
                    done_task_notifier m_notify_task_done_func;

                    //Stores the translated sentence or an error message
                    string m_target_text;

                    //Stores the synchronization mutex for synchronization on task canceling
                    //Have a recursive miutex as the last task that will be finished will
                    //also collect the translation job result into one, this shall recursively
                    //enter the guarded area
                    recursive_mutex m_end_lock;

                    //Stores the function to be called in case the tasks is cancelled
                    cancel_task_notifier m_notify_task_cancel_func;
                };
            }
        }
    }
}

#endif /* TRANS_TASK_HPP */

