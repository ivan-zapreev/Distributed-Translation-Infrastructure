/* 
 * File:   processor_job.hpp
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
 * Created on July 25, 2016, 11:41 AM
 */

#ifndef PROCESSOR_JOB_HPP
#define PROCESSOR_JOB_HPP

#include <stdio.h>
#include <ostream>
#include <fstream>

#include "common/utils/id_manager.hpp"
#include "common/utils/exceptions.hpp"
#include "common/utils/text/utf8_utils.hpp"
#include "common/utils/logging/logger.hpp"
#include "common/utils/threads/threads.hpp"

#include "common/messaging/msg_base.hpp"
#include "common/messaging/trans_session_id.hpp"
#include "common/messaging/status_code.hpp"

#include "processor/processor_consts.hpp"
#include "processor/processor_parameters.hpp"

#include "processor/messaging/proc_req_in.hpp"
#include "processor/messaging/proc_resp_out.hpp"

using namespace std;

using namespace uva::utils;
using namespace uva::utils::text;
using namespace uva::utils::exceptions;
using namespace uva::utils::threads;
using namespace uva::utils::logging;

using namespace uva::smt::bpbd::common::messaging;
using namespace uva::smt::bpbd::processor::messaging;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace processor {

                //Forward class declaration
                class processor_job;
                //Typedef the processor job pointer
                typedef processor_job * proc_job_ptr;

                //Declare the function that will be used to send the translation response to the client
                typedef function<bool (const session_id_type, const msg_base &) > session_response_sender;

                //Declare the function that will be used to create a processor response
                typedef function<proc_resp_out * (const string &, const status_code, const string &) > response_creator;

                /**
                 * Allows to log the processor job into an output stream
                 * @param stream the output stream
                 * @param job the job to log
                 * @return the reference to the same output stream send back for chaining
                 */
                ostream & operator<<(ostream & stream, const processor_job & job);

                /**
                 * This is the processor job class:
                 * Responsibilities:
                 *    - A base class for the pre and post processor jobs
                 */
                class processor_job {
                public:

                    //Define the job id type for this job
                    typedef string job_id_type;

                    //Define the function type for the function used to set the job result
                    typedef function<void(proc_job_ptr) > done_job_notifier;

                    /**
                     * The basic constructor
                     * @param config the language configuration, might be undefined.
                     * @param session_id the id of the session from which the translation request is received
                     * @param job_token a unique server wide job identifier
                     * @param req the pointer to the processor request, not NULL
                     * @param resp_crt_func the function to create the response
                     * @param resp_send_func the function to send the translation response to the client
                     */
                    processor_job(const language_config & config, const session_id_type session_id,
                            const string job_token, proc_req_in *req, const response_creator & resp_crt_func,
                            const session_response_sender & resp_send_func)
                    : m_is_canceled(false), m_is_file_gen(false), m_config(config), m_session_id(session_id),
                    m_job_token(job_token), m_priority(req->get_priority()), m_exp_num_chunks(req->get_num_chunks()),
                    m_res_lang(""), m_req_tasks(NULL), m_act_num_chunks(0), m_notify_job_done_func(NULL),
                    m_resp_crt_func(resp_crt_func), m_resp_send_func(resp_send_func) {
                        LOG_DEBUG << "Creating a processor job with " << m_exp_num_chunks << " chunks" << END_LOG;
                        //Allocate the required-size array for storing the processor requests
                        m_req_tasks = new proc_req_in_ptr[m_exp_num_chunks]();
                        //Add the request
                        add_request(req);
                    }

                    /**
                     * The basic destructor
                     */
                    virtual ~processor_job() {
                        //Delete the requests, only the received ones, not NULL
                        for (size_t idx = 0; idx < m_exp_num_chunks; ++idx) {
                            if (m_req_tasks[idx] != NULL) {
                                delete m_req_tasks[idx];
                            }
                        }
                        //Delete the array of tasks
                        delete[] m_req_tasks;
                    }

                    /**
                     * Allows to get the task priority
                     * @return the priority of this task
                     */
                    inline int32_t get_priority() const {
                        return m_priority;
                    }

                    /**
                     * Allows to retrieve the session id
                     * @return the session id
                     */
                    inline session_id_type get_session_id() const {
                        return m_session_id;
                    }

                    /**
                     * Allows to retrieve the unique server-wide job identifier
                     * @return the unique server-wide job identifier
                     */
                    inline const string & get_job_id() const {
                        return m_job_token;
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
                     * Performs the processor job
                     */
                    virtual void execute() = 0;

                    /**
                     * Allows to wait until the job is finished, this
                     * includes the notification of the job pool.
                     * This method is synchronized on final lock.
                     */
                    inline void synch_job_finished() {
                        recursive_guard guard(m_f_lock);
                    }

                    /**
                     * Allows to cancel the given processor job. Calling this method
                     * indicates that the job is canceled due to the client disconnect.
                     * This method is NOT synchronized on files lock. So the job might
                     * be in progress but we still mark it as canceled and move on.
                     */
                    inline void cancel() {
                        //recursive_guard guard(m_file_lock);
                        m_is_canceled = true;
                    }

                    /**
                     * Allows to add a new pre-processor request to the job
                     * @param req the pre-processor request
                     */
                    inline void add_request(proc_req_in * req) {
                        unique_guard guard(m_req_tasks_lock);

                        //Get the task index
                        uint64_t chunk_idx = req->get_chunk_idx();

                        //Assert sanity
                        ASSERT_SANITY_THROW((chunk_idx >= m_exp_num_chunks),
                                string("Improper chunk index: ") + to_string(chunk_idx) +
                                string(", must be <= ") + to_string(m_exp_num_chunks));

                        //Assert sanity
                        ASSERT_SANITY_THROW((m_req_tasks[chunk_idx] != NULL),
                                string("The chunk index ") + to_string(chunk_idx) +
                                string(" of the job request ") + m_job_token +
                                string(" from session ") + to_string(m_session_id) +
                                string(" is already set!"));

                        LOG_DEBUG << "Storing the job: " << req->get_job_token()
                                << " chunk: " << chunk_idx << END_LOG;

                        //Store the task
                        m_req_tasks[chunk_idx] = req;

                        //Increment the number of tasks
                        ++m_act_num_chunks;
                    }

                    /**
                     * Allows to check if the job is complete and is ready for execution.
                     * This method is synchronized on requests.
                     * @return true if the job is complete and is ready for execution otherwise false
                     */
                    inline bool is_complete() {
                        unique_guard guard(m_req_tasks_lock);

                        return (m_act_num_chunks == m_exp_num_chunks);
                    }

                protected:
                    //This is the file lock which is used when the job is being canceled.
                    //The file lock makes sure the job can not be canceled when some
                    //file descriptors of the job or child processes are begin open
                    //This is done to make sure that if the job is canceled we can
                    //clean up the files after it.
                    recursive_mutex m_file_lock;
                    //Stores the flag indicating whether the job is canceled
                    //or not. If the job is canceled then there is no need
                    //to do anything including sending the responses.
                    a_bool_flag m_is_canceled;
                    //Stores the flag indicating whether there were files generated to the disk
                    a_bool_flag m_is_file_gen;

                    /**
                     * Allows to delete the files from the given job.
                     * Note that the files will only be attempted to
                     * be deleted if some files were generated to the disk.
                     * The method is synchronized on the files lock.
                     */
                    template<bool is_pnp>
                    inline void delete_files() {
                        recursive_guard guard(m_file_lock);

                        //Delete the files only if the script was called
                        if (m_is_file_gen) {
                            delete_files<is_pnp>(m_config.get_work_dir(), m_job_token);
                        }
                    }

                    /**
                     * Allows to delete the files from the given wildcards
                     * @param work_dir the work directory
                     * @param job_token the job token
                     */
                    template<bool is_pnp>
                    static inline void delete_files(const string & work_dir, const string & job_token) {
                        //Create the command
                        const string cmd = string("rm -f ") +
                                get_text_file_name<is_pnp, true>(work_dir, job_token) + " " +
                                get_text_file_name<is_pnp, false>(work_dir, job_token);

                        //Try to execute the action a number of times,
                        //report an error only if failed multiple times.
                        int dir_err = 0;
                        size_t attempts = 0;
                        while (true) {
                            //Make the system call
                            dir_err = system(cmd.c_str());

                            //If the result is bad and we still have attempts, then re-try
                            if ((-1 == dir_err) && (attempts < MAX_NUM_CONSOLE_ATTEMPTS)) {
                                //Increment the number of attempts
                                ++attempts;
                                //Sleep for the requested number of milliseconds
                                std::this_thread::sleep_for(std::chrono::milliseconds(CONSOLE_RE_TRY_TIME_OUT_MILLISEC));
                            } else {
                                //Either the result is good or we exceeded
                                //the maximum number of attempts - stop.
                                break;
                            }
                        }

                        //Check the execution status and report an error
                        if (-1 == dir_err) {
                            THROW_EXCEPTION(string("Tried ") + to_string(attempts) +
                                    string(" times but failed to execute: ") + cmd);
                        }
                    }

                    /**
                     * Allows to construct the text file name, differs depending
                     * on whether this is a source or target text.
                     * This method is NOT synchronized.
                     * @tparam is_pnp if true then this is a pre-processor job, if false then a post-processor
                     * @tparam is_ino if true then this is for the input file of the job, if false then for the output
                     * @param work_dir the work directory string
                     * @param job_token the job token
                     * @return the name of the text file, should be unique
                     */
                    template<bool is_pnp, bool is_ino>
                    static inline string get_text_file_name(const string & work_dir, const string & job_token) {
                        return work_dir + "/" + job_token + "." +
                                (is_pnp ? "pre" : "post") + "." +
                                (is_ino ? "in" : "out") + ".txt";
                    }

                    /**
                     * Allows to dump the request text into the file with the given name.
                     * This method is synchronized on files lock.
                     * @param file_name the file name to dump the file into
                     * @throws uva_exception if the text could not be saved
                     */
                    inline void store_text_to_file(const string & file_name) {
                        recursive_guard guard(m_file_lock);

                        if (!m_is_canceled) {
                            //Check if the requests complete
                            ASSERT_SANITY_THROW(!is_complete(),
                                    string("The processor job is not complete, #tasks: ") +
                                    to_string(m_exp_num_chunks) + string(", #received: ") +
                                    to_string(m_act_num_chunks));

                            //Open the output stream to the file
                            ofstream out_file(file_name);

                            //Check that the file is open
                            ASSERT_CONDITION_THROW(!out_file.is_open(),
                                    string("Could not open: ") +
                                    file_name + string(" for writing"));

                            //Iterate and output
                            for (size_t idx = 0; (idx < m_exp_num_chunks) && !m_is_canceled; ++idx) {
                                //Output the text to the file, do not add any new lines, put text as it is.
                                out_file << m_req_tasks[idx]->get_chunk();
                                //Log the chunks for info
                                LOG_DEBUG << m_req_tasks[idx]->get_chunk() << END_LOG;
                            }

                            //Close the file
                            out_file.close();

                            //Set the flag to true, the file was generated
                            m_is_file_gen = true;

                            LOG_DEBUG << "The text is stored into: " << file_name << END_LOG;
                        }
                    }

                    /**
                     * Allows to get the reference to the language config.
                     * It is possible that the configuration is not defined!
                     * This method is NOT synchronized.
                     * @return the reference to the language config
                     */
                    inline const language_config & get_lang_config() {
                        return m_config;
                    }

                    /**
                     * Allows to construct the text file name, differs depending
                     * on whether this is a source or target text.
                     * This method is NOT synchronized.
                     * @tparam is_pnp if true then this is a pre-processor job, if false then a post-processor
                     * @tparam is_ino if true then this is for the input file of the job, if false then for the output
                     * @param job_uid [out] will be set to the job uid string
                     * @return the name of the text file, should be unique
                     */
                    template<bool is_pnp, bool is_ino>
                    inline const string get_text_file_name(string & job_uid) {
                        //Compute the file name
                        return get_text_file_name<is_pnp, is_ino>(m_config.get_work_dir(), m_job_token);
                    }

                    /**
                     * Allows to get the language string from the request.
                     * This method is NOT synchronized.
                     * @return the language string from the request.
                     */
                    inline const string get_language() {
                        return m_req_tasks[0]->get_language();
                    }

                    /**
                     * Allows to send a response to the server
                     * This method is NOT synchronized
                     * @param msg the message
                     */
                    inline void send_response(const msg_base & msg) {
                        try {
                            if (!m_is_canceled) {
                                m_resp_send_func(m_session_id, msg);
                            }
                        } catch (std::exception &ex) {
                            LOG_ERROR << "Could not send a pre-processor job response: " << ex.what() << END_LOG;
                        }
                    }

                    /**
                     * Calls the processor script and reads from its output, the script is expected to do one of:
                     * 1. Execute normally and then to write the detected source language
                     *    name into its output, for the pre-processor requests
                     * 2. Fail to execute the job - return an error code - and then to
                     *    write an error into the output
                     * This method is synchronized on files lock.
                     * @param call_str the call string
                     * @param output [out] the string to put the output of the script into
                     * @return true if the processor finished the job without errors, otherwise false
                     */
                    inline bool call_processor_script(const string &call_str, string & output) {
                        recursive_guard guard(m_file_lock);

                        //Check if the job is not being canceled
                        if (!m_is_canceled) {
                            LOG_DEBUG << "Trying to call the script: " << call_str << END_LOG;
                            size_t attempts = 0;
                            while (true) {
                                FILE *fp = popen(call_str.c_str(), "r");
                                LOG_DEBUG << "Checking on the script's pipeline file status" << END_LOG;
                                if (fp != NULL) {
                                    //The script was successfully called, process its results
                                    return process_script_results(call_str, fp, output);
                                } else {
                                    //Check if we shall stop or re-try
                                    if (attempts < MAX_NUM_CONSOLE_ATTEMPTS) {
                                        //Increment the number of attempts
                                        ++attempts;
                                        //Sleep for the requested number of milliseconds
                                        std::this_thread::sleep_for(std::chrono::milliseconds(CONSOLE_RE_TRY_TIME_OUT_MILLISEC));
                                    } else {
                                        THROW_EXCEPTION(string("Tried ") + to_string(attempts) +
                                                string(" times but failed to execute: ") + call_str);
                                    }
                                }
                            }
                        } else {
                            //The job has been canceled
                            return false;
                        }
                    }

                    /**
                     * Allows to process the results of successfully started processor script
                     * @param call_str the call string for logging
                     * @param fp the opened pipeline from the script to the program to read from
                     * @param output the output string to write into
                     * @return true if the script executed successfully, otherwise false
                     */
                    inline bool process_script_results(const string &call_str, FILE *fp, string & output) {
                        //The buffer itself
                        char buffer[MAX_PROCESSOR_OUTPUT_BYTES];

                        //Read from the pipeline - we shall get the resulting language or an error message
                        LOG_DEBUG << "Reading from the script's pipeline file" << END_LOG;
                        while (fgets(buffer, sizeof (buffer), fp) != NULL) {
                            output += string(buffer);
                        }

                        //Reduce the string to remove new lines and other whitespaces
                        (void) reduce(output);

                        LOG_DEBUG << "Closing the script's pipeline" << END_LOG;
                        //Wait until the process finishes and analyze its status
                        int status = pclose(fp);

                        //Set the flag to true, the file was generated
                        m_is_file_gen = true;

                        LOG_DEBUG << "Checking if we can get the script exit status" << END_LOG;
                        if (status == -1) {
                            //Error the process status is not possible to retrieve!
                            THROW_EXCEPTION(string("Could not get the script ") +
                                    call_str + (" execution status!"));
                        } else {
                            LOG_DEBUG << "Checking checking on the script exit status" << END_LOG;
                            if (WIFEXITED(status) != 0) {
                                //The script terminated normally, check if there were errors
                                return ((WEXITSTATUS(status) == 0) || (WEXITSTATUS(status) == EXIT_SUCCESS));
                            } else {
                                //This is a situation in which the script crashed
                                THROW_EXCEPTION(string("The processor script ") +
                                        call_str + string(" terminated abnormally!"));
                            }
                        }
                    }

                    /**
                     * Allows to send an error response to the server
                     * This method is NOT synchronized
                     * @param msg_str the error message string
                     */
                    inline void send_error_response(const string & msg_str) {
                        if (!m_is_canceled) {
                            //Get the error response
                            proc_resp_out * resp = m_resp_crt_func(m_job_token, status_code::RESULT_ERROR, msg_str);

                            //Attempt to send the job response
                            processor_job::send_response(*resp);

                            //Delete the response
                            delete resp;
                        }
                    }

                    /**
                     * Allows to send a chunk of utf8 characters to the client
                     * @param chunk the string storing the read chunk
                     * @param num_chunks the total number of chunks to send 
                     * @param chunk_idx the current chunk index starting with 0.
                     */
                    inline void send_utf8_chunk_msg(const string & chunk, const size_t num_chunks, const size_t chunk_idx) {
                        //Get the error response
                        proc_resp_out * resp = m_resp_crt_func(m_job_token, status_code::RESULT_OK, "");

                        //Set the language
                        resp->set_language(m_res_lang);

                        //Set text the text piece index and the number of text pieces
                        resp->set_chunk(chunk, chunk_idx, num_chunks);

                        //Attempt to send the job response
                        processor_job::send_response(*resp);

                        //Delete the response
                        delete resp;
                    }

                    /**
                     * Allows to send an success response to the server
                     * This method is synchronized on files lock.
                     * @tparam is_pnp if true then this is a pre-processor job, if false then a post-processor
                     * @param res_lang the "detected" file language
                     */
                    template<bool is_pnp>
                    inline void send_success_response(const string & res_lang) {
                        recursive_guard guard(m_file_lock);

                        if (!m_is_canceled) {
                            //Get the language from the stream
                            m_res_lang = res_lang;

                            //Get the output file name
                            const string file_name = get_text_file_name<is_pnp, false>(m_config.get_work_dir(), m_job_token);

                            LOG_DEBUG << "Opening the job output file: " << file_name << END_LOG;

                            //Open the input file and read from it in chunks
                            ifstream file(file_name, ifstream::binary);

                            //Assert that the file could be opened!
                            ASSERT_CONDITION_THROW(!file.is_open(), string("The resulting file: ") +
                                    file_name + string(" could not be opened!"));

                            //Process the text in chunks
                            process_utf8_chunks<MESSAGE_MAX_CHAR_LEN>(file,
                                    bind(&processor_job::send_utf8_chunk_msg, this, _1, _2, _3));
                        }
                    }

                    /**
                     * Performs the processor job
                     * @tparam is_pnp if true then this is a pre-processor job, if false then a post-processor
                     */
                    template<bool is_pnp>
                    inline void process() {
                        //Notify that the job is now finished. Synchronize on
                        //final lock to prevent premature object destruction.
                        recursive_guard guard(m_f_lock);

                        LOG_DEBUG << "is_pnp = " << is_pnp << END_LOG;

                        //Check if the job is not canceled yet
                        if (!m_is_canceled) {
                            //Check if the provided language configuration is defined
                            const language_config & conf = this->get_lang_config();
                            if (conf.is_defined()) {
                                const string file_name = get_text_file_name<is_pnp, true>(m_config.get_work_dir(), m_job_token);

                                try {
                                    //Save the file to the disk
                                    this->store_text_to_file(file_name);

                                    //Get the string needed to call the processor script
                                    const string call_str = conf.get_call_string(m_job_token, this->get_language());
                                    LOG_DEBUG << "call_str = " << call_str << END_LOG;

                                    //Call the processor script
                                    string output;
                                    if (call_processor_script(call_str, output)) {
                                        //Send the responses to the client.
                                        send_success_response<is_pnp>(output);
                                    } else {
                                        //In case there is an empty error create one
                                        if (output.empty()) {
                                            output += string("Failed to execute: '") + call_str + string("': ") +
                                                    string("An internal script error or a missing script!");
                                        }
                                        //Report an error to the client. The
                                        //output must contain the error message.
                                        LOG_DEBUG << "Processor script error: " << output << END_LOG;
                                        //Report an error to the client.
                                        send_error_response(output);
                                    }
                                } catch (std::exception & ex) {
                                    stringstream sstr;
                                    sstr << "Could not process: " << file_name << ", language: " <<
                                            this->get_language() << ", error: " << ex.what();
                                    LOG_DEBUG << sstr.str() << END_LOG;
                                    //Report an error to the client.
                                    send_error_response(sstr.str());
                                }
                            } else {
                                stringstream sstr;
                                sstr << "The language configuration is empty, meaning " <<
                                        "that the language '" << this->get_language() << "' is not " <<
                                        "supported, and there is not default processor!";
                                LOG_DEBUG << sstr.str() << END_LOG;
                                //Report an error to the client.
                                send_error_response(sstr.str());
                            }
                        }

                        //Notify that this job id done
                        m_notify_job_done_func(this);
                    }

                private:
                    //Stores the reference to the language config, might be undefined
                    const language_config & m_config;
                    //Stores the translation client session id
                    const session_id_type m_session_id;
                    //Stores the job unique identifier.
                    const string m_job_token;
                    //Stores the job priority
                    const int32_t m_priority;
                    //Stores the number of text pieces this job consists of
                    const uint64_t m_exp_num_chunks;
                    //Stores the resulting language string
                    string m_res_lang;
                    //Stores the lock for accessing the tasks array
                    mutex m_req_tasks_lock;
                    //Stores the array of pointers to the processor job requests
                    proc_req_in_ptr * m_req_tasks;
                    //Stores the current number of received requests
                    uint64_t m_act_num_chunks;

                    //The final lock needed to guard the job ready notification and
                    //waiting for it is finished before the job is deleted.
                    recursive_mutex m_f_lock;
                    //The done job notifier
                    done_job_notifier m_notify_job_done_func;

                    //Stores the response creator function
                    const response_creator m_resp_crt_func;
                    //Stores the reference to response sending function
                    const session_response_sender & m_resp_send_func;

                    //Make a friend of the output operator
                    friend ostream & operator<<(ostream & stream, const processor_job & job);
                };
            }
        }
    }
}




#endif /* PROCESSOR_JOB_HPP */

