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

#include <stdint.h>

#include "trans_task_id.hpp"
#include "trans_info_provider.hpp"

#include "common/utils/id_manager.hpp"
#include "common/utils/threads/threads.hpp"
#include "common/utils/logging/logger.hpp"
#include "common/messaging/trans_session_id.hpp"
#include "common/messaging/job_id.hpp"
#include "common/messaging/status_code.hpp"

#include "server/decoder/de_configurator.hpp"
#include "server/decoder/sentence/sentence_decoder.hpp"
#include "server/messaging/trans_sent_data_out.hpp"

using namespace std;

using namespace uva::utils;
using namespace uva::utils::exceptions;
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

                    /**
                     * The basic constructor allowing to initialize the main class constants
                     * @param session_id the session id of the task, is used for logging
                     * @param job_id the job id of the task, is used for logging
                     * @param priority the translation task priority
                     * @param source_text the sentence to be translated
                     */
                    trans_task(const session_id_type session_id, const job_id_type job_id,
                            const int32_t priority, const string & source_text,
                            done_task_notifier notify_task_done_func)
                    : m_is_stop(false), m_session_id(session_id), m_job_id(job_id), m_priority(priority),
                    m_task_id(m_id_mgr.get_next_id()), m_status_code(status_code::RESULT_UNDEFINED),
                    m_status_msg(""), m_source_text(source_text),
                    m_notify_task_done_func(notify_task_done_func), m_target_text(""),
                    m_decoder(de_configurator::get_params(), m_is_stop, m_source_text, m_target_text) {
                        LOG_DEBUG1 << "/session id=" << m_session_id << ", job id="
                                << m_job_id << ", NEW task id=" << m_task_id
                                << "/ text: " << m_source_text << END_LOG;
                    }

                    /**
                     * The basic destructor
                     */
                    virtual ~trans_task() {
                        //Nothing to be done here
                    }

                    /**
                     * Allows to cancel the translation task
                     */
                    inline void cancel() {
                        LOG_DEBUG1 << "Canceling the task " << m_task_id << " translation ..." << END_LOG;

                        //Synchronize to avoid canceling the job that is already finished.
                        {
                            recursive_guard guard_end(m_end_lock);

                            LOG_DEBUG1 << "The task " << m_task_id << " is to be interrupted!" << END_LOG;

                            //Set the stopping flag to true
                            m_is_stop = true;

                            //Do nothing else, just set the flag, that is needed to avoid deadlocking
                            //The finished task, and job processing is to be done from a separate thread.
                        }

                        LOG_DEBUG1 << "The task " << m_task_id << " translation is canceled!" << END_LOG;
                    }

                    /**
                     * Performs the translation for the given sentence
                     */
                    inline void execute() {
                        LOG_DEBUG1 << "Starting the task " << m_task_id << " translation ..." << END_LOG;

                        //Perform the decoding task
                        try {
                            LOG_DEBUG1 << "Invoking the sentence translation for task " << m_task_id << END_LOG;
                            m_decoder.translate();
                        } catch (std::exception & ex) {
                            //Set the response code
                            m_status_code = status_code::RESULT_ERROR;
                            //Set the error message for the client
                            m_status_msg = ex.what();
                            //Do local logging
                            LOG_DEBUG << "SERVER ERROR: " << m_target_text << END_LOG;
                        }

                        LOG_DEBUG1 << "The task " << m_task_id << " translation part is over." << END_LOG;

                        //Synchronize to avoid canceling the job that is already finished.
                        {
                            recursive_guard guard_end(m_end_lock);

                            LOG_DEBUG1 << "The task " << m_task_id << " is to be finished!" << END_LOG;

                            //Produce the task result
                            process_task_result();

#if IS_SERVER_TUNING_MODE
                            //Dump the search lattice for the sentence if needed
                            const de_parameters & de_params = de_configurator::get_params();
                            LOG_DEBUG1 << "Dumping the search lattice for task " << m_task_id
                                    << " is " << (de_params.m_is_gen_lattice ? "" : "NOT ")
                                    << "needed!" << END_LOG;
                            if (!m_is_stop && de_params.m_is_gen_lattice) {
                                dump_search_lattice(de_params);
                            }
#endif

                        }

                        LOG_DEBUG1 << "The task " << m_task_id << " translation is done, notifying!" << END_LOG;

                        //Call the task-done notification function to report that we are finished!s
                        m_notify_task_done_func(this);

                        LOG_DEBUG1 << "The task " << m_task_id << " translation is done!" << END_LOG;
                    }

                    /**
                     * Allows to get the task priority
                     * @return the priority of this task
                     */
                    inline int32_t get_priority() const {
                        return m_priority;
                    }

                    /**
                     * Allows to retrieve the translation task result code
                     * @return the translation task result code
                     */
                    inline status_code get_status_code() const {
                        return m_status_code;
                    }

                    /**
                     * Allows to retrieve the error message for the sentence
                     * @return the error's text
                     */
                    inline const string & get_status_msg() const {
                        return m_status_msg;
                    }

                    /**
                     * Allows to retrieve the sentence in the source language
                     * @return the sentence in the source language
                     */
                    inline const string & get_source_text() const {
                        return m_source_text;
                    }

                    /**
                     * Allows to obtain the translation info for the translation task.
                     * @param sent_data [in/out] the container object for the translation task info
                     */
                    inline void get_trans_info(trans_sent_data_out & sent_data) const {
                        //Fill in the translation task info
                        m_decoder.get_trans_info(sent_data);
                    }

                    /**
                     * Allows to retrieve the sentence in the target language or an error message
                     * @return the sentence in the target language or an error message
                     */
                    inline const string & get_target_text() {
                        LOG_DEBUG1 << "Retrieving the target text of task: " << m_task_id << END_LOG;
                        {
                            recursive_guard guard_end(m_end_lock);

                            return m_target_text;
                        }
                    }

                protected:

                    /**
                     * Close the files in case they are open
                     * @param scores_file the scores file
                     * @param lattice_file the lattice file
                     */
                    inline void close_lattice_files(ofstream & scores_file, ofstream & lattice_file) const {
                        if (scores_file.is_open()) {
                            scores_file.close();
                        }
                        if (lattice_file.is_open()) {
                            lattice_file.close();
                        }
                    }

#if IS_SERVER_TUNING_MODE

                    /**
                     * Allows to dump the search lattice in case it is needed.
                     * Note that, in this code we do not care if in case of an
                     * error we do not close the files. This is because lattice
                     * dumping is only needed while model training, plus it should
                     * work without throwing any errors. So if it happens then
                     * it is not a big problem.
                     * @param de_params the decoder parameters
                     */
                    inline void dump_search_lattice(const de_parameters & de_params) const {
                        LOG_DEBUG1 << "Dumping the search lattice for the translation task " << m_task_id << END_LOG;

                        //Create the file names, we do not add the session id and job id as
                        //the tasks in training mode are issued unique task ids.
                        const string file_name = de_params.m_lattices_folder + "/" + to_string(m_task_id) + ".";
                        const string scores_file_name = file_name + de_params.m_scores_file_ext;
                        const string lattice_file_name = file_name + de_params.m_lattice_file_ext;

                        //Open the output stream to the files
                        ofstream scores_file(scores_file_name), lattice_file(lattice_file_name);

                        try {
                            //Check that the files are open
                            ASSERT_CONDITION_THROW(!scores_file.is_open(), string("Could not open: ") +
                                    scores_file_name + string(" for writing"));
                            ASSERT_CONDITION_THROW(!lattice_file.is_open(), string("Could not open: ") +
                                    lattice_file_name + string(" for writing"));

                            //Call the sentence decoder to do dumping.
                            m_decoder.dump_search_lattice(lattice_file, scores_file);
                        } catch (std::exception & ex) {
                            //Close the lattice files
                            LOG_ERROR << ex.what() << END_LOG;
                            close_lattice_files(scores_file, lattice_file);
                            //Re-throw the exception
                            throw ex;
                        }
                        //Close the lattice files
                        close_lattice_files(scores_file, lattice_file);
                    }
#endif

                    /**
                     * Allows to process the translation task result in case of a successful and abnormal task termination.
                     * This includes sending the notification to the translation job that the task is finished.
                     * NOTE: This method is not thread safe!
                     */
                    inline void process_task_result() {
                        //Set the task is not canceled then set the result, otherwise set the canceled code.
                        if (m_status_code == status_code::RESULT_ERROR) {
                            //If there was an error during translation send back the original text
                            m_target_text = m_source_text;
                        } else {
                            //Unless it was an error while translating there should be an undefined status
                            ASSERT_SANITY_THROW((m_status_code != status_code::RESULT_UNDEFINED),
                                    string("Unexpected translation code: ") + to_string(m_status_code) +
                                    string(" must be UNDEFINED!"));

                            //Check if we were interrupted or not
                            if (m_is_stop) {
                                //If the translation has been canceled just send back the source
                                m_status_code = status_code::RESULT_CANCELED;
                                m_status_msg = "Canceled by the server!";
                                m_target_text = m_source_text;
                            } else {
                                //If the translation has been finished send back the target
                                m_status_code = status_code::RESULT_OK;
                                m_status_msg = "";
                                m_target_text = m_target_text;
                            }
                        }
                    }

                private:
                    //Stores the flag that indicates that we need to stop the translation algorithm
                    a_bool_flag m_is_stop;

                    //Stores the translation task session id, is needed for logging
                    const session_id_type m_session_id;

                    //Stores the translation task job id, is needed for logging
                    const job_id_type m_job_id;

                    //Stores the translation task priority
                    const int32_t m_priority;

                    //Stores the translation task id
                    const task_id_type m_task_id;

                    //Stores the translation task result code
                    status_code m_status_code;

                    //Stores the error text for the not ok code
                    string m_status_msg;

                    //Stores the sentence to be translated
                    const string m_source_text;

                    //Stores the task-done notifier function for this task
                    done_task_notifier m_notify_task_done_func;

                    //Stores the translated sentence or an error message
                    string m_target_text;

                    //Stores the pointer to the sentence decoder instance
                    sentence_decoder m_decoder;

                    //Stores the synchronization mutex for synchronization on task canceling
                    //Have a recursive miutex as the last task that will be finished will
                    //also collect the translation job result into one, this shall recursively
                    //enter the guarded area
                    recursive_mutex m_end_lock;

                    //Stores the static instance of the id manager
                    static id_manager<task_id_type> m_id_mgr;

                    friend ostream & operator<<(ostream & stream, const trans_task & taks);
                };

                /**
                 * Allows to log the translation task into an output stream
                 * @param stream the output stream
                 * @param task the task to log
                 * @return the reference to the same output stream send back for chaining
                 */
                ostream & operator<<(ostream & stream, const trans_task & task);
            }
        }
    }
}

#endif /* TRANS_TASK_HPP */

