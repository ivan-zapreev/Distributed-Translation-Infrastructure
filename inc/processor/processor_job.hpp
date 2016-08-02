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
#include "common/utils/string_utils.hpp"
#include "common/utils/logging/logger.hpp"
#include "common/utils/threads/threads.hpp"

#include "common/messaging/msg_base.hpp"
#include "common/messaging/trans_session_id.hpp"
#include "common/messaging/job_id.hpp"
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
                typedef function<proc_resp_out * (const job_id_type, const status_code, const string &) > response_creator;

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

                    //Define the function type for the function used to set the job result
                    typedef function<void(proc_job_ptr) > done_job_notifier;

                    /**
                     * The basic constructor
                     * @param config the language configuration, might be undefined.
                     * @param session_id the id of the session from which the translation request is received
                     * @param req the pointer to the processor request, not NULL
                     * @param resp_crt_func the function to create the response
                     * @param resp_send_func the function to send the translation response to the client
                     */
                    processor_job(const language_config & config, const session_id_type session_id,
                            proc_req_in *req, const response_creator & resp_crt_func,
                            const session_response_sender & resp_send_func)
                    : m_is_canceled(false), m_config(config), m_session_id(session_id),
                    m_job_id(req->get_job_id()), m_num_tps(req->get_num_chunks()),
                    m_res_lang(""), m_req_tasks(NULL), m_tasks_count(0), m_notify_job_done_func(NULL),
                    m_resp_crt_func(resp_crt_func), m_resp_send_func(resp_send_func) {
                        //Allocate the required-size array for storing the processor requests
                        m_req_tasks = new proc_req_in_ptr[m_num_tps]();
                        //Add the request
                        add_request(req);
                    }

                    /**
                     * The basic destructor
                     */
                    virtual ~processor_job() {
                        //Delete the requests, only the received ones, not NULL
                        for (size_t idx = 0; idx < m_num_tps; ++idx) {
                            if (m_req_tasks[idx] != NULL) {
                                delete m_req_tasks[idx];
                            }
                        }
                        //Delete the array of tasks
                        delete[] m_req_tasks;
                    }

                    /**
                     * Allows to retrieve the session id
                     * @return the session id
                     */
                    inline session_id_type get_session_id() const {
                        return m_session_id;
                    }

                    /**
                     * Allows to retrieve the job id as given by the client.
                     * The job id given by the balancer is retrieved by another method.
                     * @return the job id
                     */
                    inline job_id_type get_job_id() const {
                        return m_job_id;
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
                    void synch_job_finished() {
                        recursive_guard guard(m_f_lock);
                    }

                    /**
                     * Allows to cancel the given processor job. Calling this method
                     * indicates that the job is canceled due to the client disconnect.
                     * This method is synchronized on files lock.
                     */
                    inline void cancel() {
                        recursive_guard guard(m_file_lock);

                        m_is_canceled = true;
                    }

                    /**
                     * Allows to add a new pre-processor request to the job
                     * @param req the pre-processor request
                     */
                    inline void add_request(proc_req_in * req) {
                        unique_guard guard(m_req_tasks_lock);

                        //Get the task index
                        uint64_t piece_idx = req->get_chunk_idx();

                        //Assert sanity
                        ASSERT_SANITY_THROW((piece_idx >= m_num_tps),
                                string("Improper tasks index: ") + to_string(piece_idx) +
                                string(", must be <= ") + to_string(m_num_tps));

                        //Assert sanity
                        ASSERT_SANITY_THROW((m_req_tasks[piece_idx] != NULL),
                                string("The task index ") + to_string(piece_idx) +
                                string(" of the job request ") + to_string(m_job_id) +
                                string(" from session ") + to_string(m_session_id) +
                                string(" is already set!"));

                        //Store the task
                        m_req_tasks[piece_idx] = req;

                        //Increment the number of tasks
                        ++m_tasks_count;
                    }

                    /**
                     * Allows to check if the job is complete and is ready for execution.
                     * This method is synchronized on requests.
                     * @return true if the job is complete and is ready for execution otherwise false
                     */
                    inline bool is_complete() {
                        unique_guard guard(m_req_tasks_lock);

                        return (m_tasks_count == m_num_tps);
                    }

                    /**
                     * Allows to delete the files from the given session
                     * @param work_dir the work directory
                     * @param session_id the session id
                     */
                    static inline void delete_session_files(const string & work_dir, const session_id_type session_id) {
                        //Create the job_uid wildcard, "session_id.job_id"
                        const string wildcard = get_job_uid_str(session_id, job_id::UNDEFINED_JOB_ID);
                        //Remove all sorts of files request, response, input, output
                        delete_files(work_dir, wildcard);
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

                    /**
                     * Allows to get the unique job identifier string for the given session id and job id.
                     * If the session id or job id is not known then a "*" symbol is used in its place.
                     * @param sid the session id
                     * @param jid the job id
                     * @return the job unique identifier
                     */
                    static inline string get_job_uid_str(const session_id_type sid, const job_id_type jid) {
                        return ( sid == session_id::UNDEFINED_SESSION_ID ? "*" : to_string(sid))
                                + "." +
                                (jid == job_id::UNDEFINED_JOB_ID ? "*" : to_string(jid));
                    }

                    /**
                     * Allows to delete the files from the given job
                     */
                    inline void delete_job_files() {
                        //Create the job_uid wildcard, "session_id.job_id"
                        const string wildcard = get_job_uid_str(session_id::UNDEFINED_SESSION_ID, m_job_id);
                        //Remove all sorts of files request, response, input, output
                        delete_files(m_config.get_work_dir(), wildcard);
                    }

                    /**
                     * Allows to delete the files from the given wildcards
                     * @param work_dir the work directory
                     * @param wildcard the wildcard to remove files
                     */
                    static inline void delete_files(const string & work_dir, const string wildcard) {
                        //Create the command
                        const string cmd = string("rm -f ") +
                                get_text_file_name<true, true>(work_dir, wildcard) + " " +
                                get_text_file_name<true, false>(work_dir, wildcard) + " " +
                                get_text_file_name<false, true>(work_dir, wildcard) + " " +
                                get_text_file_name<false, false>(work_dir, wildcard);

                        //make the system call
                        const int dir_err = system(cmd.c_str());

                        //Check the execution status
                        if (-1 == dir_err) {
                            LOG_ERROR << "Could not delete files with command: " << cmd << END_LOG;
                        }
                    }

                    /**
                     * Allows to construct the text file name, differs depending
                     * on whether this is a source or target text.
                     * This method is NOT synchronized.
                     * @param is_pnp if true then this is a pre-processor job, if false then a post-processor
                     * @param is_ino if true then this is for the input file of the job, if false then for the output
                     * @param job_uid_str [out] will be set to the job uid string
                     * @return the name of the text file, should be unique
                     */
                    template<bool is_pnp, bool is_ino>
                    static inline string get_text_file_name(const string & work_dir, const string & job_uid_str) {
                        return work_dir + "/" + job_uid_str + "." +
                                (is_pnp ? "pre" : "post") + "." +
                                (is_ino ? "in" : "out") + ".txt";
                    }

                    /**
                     * Shall be called once the balancer job is done
                     * This method is synchronized on final lock.
                     */
                    inline void notify_job_done() {
                        recursive_guard guard(m_f_lock);

                        //Notify that this job id done
                        m_notify_job_done_func(this);
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
                                    to_string(m_num_tps) + string(", #received: ") +
                                    to_string(m_tasks_count));

                            //Open the output stream to the file
                            ofstream out_file(file_name);

                            //Check that the file is open
                            ASSERT_CONDITION_THROW(!out_file.is_open(),
                                    string("Could not open: ") +
                                    file_name + string(" for writing"));

                            //Iterate and output
                            for (size_t idx = 0; (idx < m_num_tps); ++idx) {
                                //Output the text to the file, do not add any new lines, put text as it is.
                                out_file << m_req_tasks[idx]->get_chunk();
                            }

                            //Close the file
                            out_file.close();

                            LOG_USAGE << "The text is stored into: " << file_name << END_LOG;
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
                     * @param is_pnp if true then this is a pre-processor job, if false then a post-processor
                     * @param is_ino if true then this is for the input file of the job, if false then for the output
                     * @param job_uid_str [out] will be set to the job uid string
                     * @return the name of the text file, should be unique
                     */
                    template<bool is_pnp, bool is_ino>
                    inline const string get_text_file_name(string & job_uid_str) {
                        //Set the job uid
                        job_uid_str = get_job_uid_str(m_session_id, m_job_id);
                        //Compute the file name
                        return get_text_file_name<is_pnp, is_ino>(m_config.get_work_dir(), job_uid_str);
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
                     * @param output the output of the script
                     * @return true if the processor finished the job without errors, otherwise false
                     */
                    inline bool call_processor_script(const string &call_str, stringstream & output) {
                        recursive_guard guard(m_file_lock);

                        if (!m_is_canceled) {
                            FILE *fp = popen(call_str.c_str(), "r");
                            if (fp != NULL) {
                                //The buffer itself
                                char buffer[MAX_PROCESSOR_OUTPUT_BYTES];
                                //Read from the pipeline - we shall get the resulting language or an error message
                                while (fgets(buffer, sizeof (buffer), fp) != NULL) {
                                    output << buffer;
                                }

                                //Wait until the process finishes and analyze its status
                                int status = pclose(fp);
                                if (status == -1) {
                                    //Error the process status is not possible to retrieve!
                                    THROW_EXCEPTION(string("Could not get the script ") +
                                            call_str + (" execution status!"));
                                } else {
                                    if (WIFEXITED(status) != 0) {
                                        //The script terminated normally, check if there were errors
                                        return ((WEXITSTATUS(status) == 0) || (WEXITSTATUS(status) == EXIT_SUCCESS));
                                    } else {
                                        THROW_EXCEPTION(string("The processor script ") +
                                                call_str + string(" terminated abnormally!"));
                                    }
                                }
                            } else {
                                THROW_EXCEPTION(string("Failed to call the pre-processor script: ") + call_str);
                            }
                        } else {
                            //The job has been canceled
                            return false;
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
                            proc_resp_out * resp = m_resp_crt_func(this->get_job_id(), status_code::RESULT_ERROR, msg_str);

                            //Attempt to send the job response
                            processor_job::send_response(*resp);

                            //Delete the response
                            delete resp;
                        }
                    }

                    /**
                     * Allows to send a chunk of utf8 characters to the client
                     * @param buffer the buffer storing the characters
                     * @param num_chunks the total number of chunks to send 
                     * @param chunk_idx the current chunk index starting with 0.
                     */
                    inline void send_utf8_chunk_msg(wchar_t * buffer, const size_t num_chunks, const size_t chunk_idx) {
                        //Get the error response
                        proc_resp_out * resp = m_resp_crt_func(this->get_job_id(), status_code::RESULT_OK, "");

                        //Set the language
                        resp->set_language(m_res_lang);

                        //Set text the text piece index and the number of text pieces
                        wstring ws(buffer);
                        resp->set_chunk(string(ws.begin(), ws.end()), chunk_idx, num_chunks);

                        //Attempt to send the job response
                        processor_job::send_response(*resp);

                        //Delete the response
                        delete resp;
                    }

                    /**
                     * Allows to send an success response to the server
                     * This method is synchronized on files lock.
                     * @param is_pnp if true then this is a pre-processor job, if false then a post-processor
                     * @param msg_str the success message string
                     */
                    template<bool is_pnp>
                    inline void send_success_response(const stringstream & msg_str) {
                        recursive_guard guard(m_file_lock);

                        if (!m_is_canceled) {
                            //Get the language from the stream
                            m_res_lang = msg_str.str();

                            //Get the output file name
                            string uid;
                            const string file_name = this->get_text_file_name<is_pnp, false>(uid);

                            //Open the input file and read from it in chunks
                            ifstream file(file_name, ifstream::binary | ios::ate);

                            //Assert that the file could be opened!
                            ASSERT_CONDITION_THROW(!file.is_open(), string("The resulting file: ") +
                                    file_name + string(" could not be opened!"));

                            //Process the text in chunks
                            process_utf8_chunks<MESSAGE_MAX_WCHARS_LEN>(file,
                                    bind(&processor_job::send_utf8_chunk_msg, this, _1, _2, _3));
                        }
                    }

                    /**
                     * Performs the processor job
                     * @param is_pnp if true then this is a pre-processor job, if false then a post-processor
                     */
                    template<bool is_pnp>
                    inline void process() {
                        //Check if the job is not canceled yet
                        if (!m_is_canceled) {
                            //Check if the provided language configuration is defined
                            const language_config & conf = this->get_lang_config();
                            if (conf.is_defined()) {
                                //Define the job uid string
                                string job_uid_str;
                                //Create the file name for the text we need to process.
                                const string file_name = this->template get_text_file_name<is_pnp, true>(job_uid_str);

                                try {
                                    //Save the file to the disk
                                    this->store_text_to_file(file_name);

                                    //Get the string needed to call the processor script
                                    const string call_str = conf.get_call_string(job_uid_str, this->get_language());

                                    //Call the processor script
                                    stringstream output;
                                    if (call_processor_script(call_str, output)) {
                                        //Send the responses to the client.
                                        send_success_response<is_pnp>(output);
                                    } else {
                                        //Report an error to the client. The
                                        //output must contain the error message.
                                        LOG_DEBUG << output.str() << END_LOG;
                                        //Report an error to the client.
                                        send_error_response(output.str());
                                    }
                                } catch (std::exception & ex) {
                                    stringstream sstr;
                                    sstr << "Could not process: " << file_name << ", language: " <<
                                            this->get_language() << ", error: " << ex.what();
                                    LOG_ERROR << sstr.str() << END_LOG;
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

                        //Notify that the job is now finished.
                        notify_job_done();
                    }

                private:
                    //Stores the reference to the language config, might be undefined
                    const language_config & m_config;
                    //Stores the translation client session id
                    const session_id_type m_session_id;
                    //Stores the job id for an easy access
                    const job_id_type m_job_id;
                    //Stores the number of text pieces this job consists of
                    const uint64_t m_num_tps;
                    //Stores the resulting language string
                    string m_res_lang;
                    //Stores the lock for accessing the tasks array
                    mutex m_req_tasks_lock;
                    //Stores the array of pointers to the processor job requests
                    proc_req_in_ptr * m_req_tasks;
                    //Stores the current number of received requests
                    uint64_t m_tasks_count;

                    //The final lock needed to guard the job ready notification and
                    //waiting for it is finished before the job is deleted.
                    recursive_mutex m_f_lock;
                    //The done job notifier
                    done_job_notifier m_notify_job_done_func;

                    //Stores the response creator function
                    const response_creator m_resp_crt_func;
                    //Stores the reference to response sending function
                    const session_response_sender & m_resp_send_func;
                };
            }
        }
    }
}




#endif /* PROCESSOR_JOB_HPP */

