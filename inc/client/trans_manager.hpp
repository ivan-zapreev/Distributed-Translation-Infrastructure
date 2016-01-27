/* 
 * File:   trans_manager.hpp
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
 * Created on January 26, 2016, 1:06 PM
 */

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <fstream>

#include "client_config.hpp"
#include "translation_client.hpp"
#include "trans_job.hpp"
#include "trans_job_status.hpp"
#include "common/messaging/trans_job_code.hpp"
#include "common/messaging/id_manager.hpp"
#include "common/messaging/trans_job_id.hpp"
#include "common/messaging/trans_job_request.hpp"
#include "common/messaging/trans_job_response.hpp"
#include "common/utils//file/CStyleFileReader.hpp"

using namespace std;
using namespace uva::smt::decoding::client::status;

#ifndef TRANS_MANAGER_HPP
#define TRANS_MANAGER_HPP

namespace uva {
    namespace smt {
        namespace decoding {
            namespace client {

                /**
                 * This is the client side translation manager class. It's task
                 * is to get the source text from a file and then split it into
                 * a number of translation jobs that will be sent to the
                 * translation server. The finished translation jobs are collected
                 * and once all of them are finished the resulting text is written
                 * into the output file.
                 */
                class trans_manager {
                public:
                    //Stores the absolute allowed minimum of sentences to be sent by translation request.
                    //Note that if the translation text is smaller then this value is overruled.
                    static constexpr uint64_t MIN_SENTENCES_PER_REQUEST = 5;

                    //Define the unique lock needed for wait/notify
                    typedef unique_lock<mutex> unique_lock;

                    //Define the type for the list of the translation data objects
                    typedef vector<trans_job_ptr> jobs_list_type;
                    typedef jobs_list_type::iterator jobs_list_iter_type;
                    //Define the type for the map from job id to job data
                    typedef unordered_map<job_id_type, trans_job_ptr> jobs_map_type;
                    typedef jobs_map_type::iterator jobs_map_iter_type;

                    /**
                     * This is the basic constructor needed to 
                     * @param params the translation client parameters
                     */
                    trans_manager(const client_config & params)
                    : m_params(params),
                    m_client(m_params.m_server, m_params.m_port,
                    bind(&trans_manager::set_job_response, this, _1),
                    bind(&trans_manager::notify_conn_closed, this)),
                    m_source_file(params.m_source_file),
                    m_sending_thread_ptr(NULL) {
                        //If the input file could not be opened, we through!
                        ASSERT_CONDITION_THROW(!m_source_file.is_open(),
                                string("Could not open the source text file: ") + params.m_source_file);

                        //Log the source file reader info
                        m_source_file.log_reader_type_usage_info();

                        //Check that the minimum is not larger than the maximum
                        ASSERT_CONDITION_THROW((m_params.m_max_sent < m_params.m_min_sent),
                                string("The minimum number of sentences to be sent (") + to_string(m_params.m_min_sent)
                                + string(") is larger that the maximum (") + to_string(m_params.m_max_sent) + string(")!"));

                        //Check that the minimum is not too small
                        ASSERT_CONDITION_THROW((m_params.m_min_sent < MIN_SENTENCES_PER_REQUEST),
                                string("The minimum number of sentences to be sent (") + to_string(m_params.m_min_sent)
                                + string(") should be larger or equal than") + to_string(MIN_SENTENCES_PER_REQUEST));

                        //Set the stopping flag to false
                        m_is_stopping = false;
                        //Set the sending flag to false
                        m_is_all_jobs_sent = false;
                        //Set the receiving flag to false
                        m_is_all_jobs_done = false;
                        //Set the nuber of done jobs
                        m_num_done_jobs = 0;
                    }

                    /**
                     * The basic destructor class
                     */
                    virtual ~trans_manager() {
                        //Close the source file
                        m_source_file.close();

                        //Clean the internal administration, just delete the translation jobs 
                        for (jobs_list_iter_type it = m_jobs_list.begin(); (it != m_jobs_list.end()); ++it) {
                            delete (*it);
                        }

                        //Destroy the translation jobs sending thread
                        if (m_sending_thread_ptr != NULL) {
                            delete m_sending_thread_ptr;
                        }
                    }

                    /**
                     * Allows to start the translation process
                     */
                    void start() {
                        LOG_INFO << "Starting the translation process!" << END_LOG;

                        if (m_client.connect()) {
                            //Run the translation job sending thread
                            m_sending_thread_ptr = new thread(bind(&trans_manager::send_translation_jobs, this));
                        } else {
                            THROW_EXCEPTION(string("Could not open the connection to: ") + m_client.get_uri());
                        }
                    }

                    /**
                     * Allows to wait until the translations are done
                     */
                    void wait() {
                        LOG_INFO << "Waiting for the the translation process to finish ..." << END_LOG;

                        //Wait until all the jobs are sent
                        {
                            //Make sure that translation-waiting activity is synchronized
                            unique_lock guard(m_jobs_sent_lock);

                            //Wait for the translations jobs to be sent, use the time out to prevent missing notification
                            while (!m_is_all_jobs_sent && (m_jobs_sent_cond.wait_for(guard, chrono::seconds(1)) == cv_status::timeout)) {
                            }
                        }

                        //Wait until all the jobs are finished or the connection is closed
                        {
                            //Make sure that translation-waiting activity is synchronized
                            unique_lock guard(m_jobs_done_lock);

                            //Wait for the translations jobs to be received, use the time out to prevent missing notification
                            while (!m_is_all_jobs_done && (m_jobs_done_cond.wait_for(guard, chrono::seconds(1)) == cv_status::timeout)) {
                            }
                        }
                    }

                    /**
                     * This method allows to stop the translation client and
                     * to write the resulting translations into the file.
                     */
                    void stop() {
                        LOG_INFO << "Stopping the translation process!" << END_LOG;

                        //Stop the translation job sending thread
                        if (m_sending_thread_ptr != NULL) {
                            m_is_stopping = true;
                            m_sending_thread_ptr->join();
                        }

                        //Disconnect from the server
                        m_client.disconnect();

                        //Write the translations we have so far into the file
                        write_result_to_file();
                    };

                protected:

                    /**
                     * Allows to write the received translation job replies into the file
                     * @param fis the first sentence number 
                     * @param lis the last sentence number
                     * @param job the translation job data
                     * @param target_file the file to write to
                     */
                    void write_received_job_result(const uint32_t fis, const uint32_t lis,
                            const trans_job_ptr job, ofstream & target_file) {
                        //The job response is received but it can still be fully or partially canceled or be an error
                        const trans_job_code code = job->m_response->get_code();
                        switch( code ){
                            case trans_job_code::RESULT_OK : 
                            case trans_job_code::RESULT_PARTIAL : 
                                //ToDo: If the result is canceled or partial then just put the text into the file
                                break;
                            case trans_job_code::RESULT_ERROR : 
                            case trans_job_code::RESULT_CANCELED : 
                            default:
                                    //Report a warning
                                    LOG_WARNING << "Sentences from " << fis << " to " << lis << " are not "
                                            << "translated, job code: '" << code << "'" << END_LOG;

                                    //Write data to file
                                    target_file << "<--------- Error: Sentences [" << fis << ":" << lis
                                            << "] are not translated, status: '"
                                            << code << "'-------->";
                        }
                    }

                    /**
                     * Allows to generate the translation result file.
                     */
                    void write_result_to_file() {
                        //Open the report file
                        ofstream target_file;
                        target_file.open(m_params.m_target_file);
                        //Declare the variable to store the line cursor
                        uint32_t line_cursor = 0;

                        //Go through the translation job data and write it into the files
                        for (jobs_list_iter_type it = m_jobs_list.begin(); (it != m_jobs_list.end()); ++it) {
                            //Get the translation job data
                            const trans_job_ptr job = *it;

                            //Increment the line cursor
                            ++line_cursor;

                            //Compute the first and last sentence numbers for the case the job is not completed
                            const uint32_t fis = line_cursor;
                            const uint32_t lis = fis + job->m_num_sentences - 1;

                            //Check on the translation job status
                            switch (job->m_status) {
                                case trans_job_status::STATUS_RES_RECEIVED:
                                    write_received_job_result(fis, lis, job, target_file);
                                    break;
                                case trans_job_status::STATUS_REQ_SENT_GOOD:
                                case trans_job_status::STATUS_REQ_SENT_FAIL:
                                case trans_job_status::STATUS_REQ_INITIALIZED:
                                case trans_job_status::STATUS_UNDEFINED:
                                default:
                                    //Report a warning
                                    const char * const status_str = get_status_str(job->m_status);
                                    LOG_WARNING << "Sentences from " << fis << " to " << lis << " are not "
                                            << "translated, job status: '" << status_str << "'" << END_LOG;

                                    //Write data to file
                                    target_file << "<--------- Error: Sentences [" << fis << ":" << lis
                                            << "] are not translated, status: '"
                                            << status_str << "'-------->";
                            }
                        }

                        //Close the report file
                        target_file.close();
                    }

                    /**
                     * Allows to process the server job request response
                     * @param trans_job_resp the translation job response coming from the server
                     */
                    void set_job_response(trans_job_response * trans_job_resp) {
                        const job_id_type job_id = trans_job_resp->get_job_id();

                        //Check if the job is valid and if there is something for this job id
                        if (job_id != job_id::UNDEFINED_JOB_ID) {
                            //Check if the job with the given id is known
                            if (m_ids_to_jobs_map.find(job_id) != m_ids_to_jobs_map.end()) {
                                //Register the job in the administration
                                m_ids_to_jobs_map[job_id]->m_response = trans_job_resp;
                                
                                //Set the translation job status as received 
                                m_ids_to_jobs_map[job_id]->m_status = trans_job_status::STATUS_RES_RECEIVED;

                                //Increment the number of received jobs
                                m_num_done_jobs++;
                            } else {
                                LOG_ERROR << "The received job response id " << job_id << " is not known!" << END_LOG;
                            }
                        } else {
                            LOG_ERROR << "One of the job responses could not be parsed!" << END_LOG;
                        }

                        //If we received all the jobs then notify that all the jobs are received!
                        if (m_is_all_jobs_sent && (m_num_done_jobs == m_jobs_list.size())) {
                            notify_jobs_done();
                        }
                    }

                    /**
                     * This function will be called if the connection is closed during the translation process
                     */
                    void notify_conn_closed() {
                        //If the connection is closed we should have no new job responses, so start processing the results
                        notify_jobs_done();
                    }

                    /**
                     * Allows to notify the threads waiting on the translation jobs to be sent
                     */
                    void notify_jobs_sent() {
                        //Make sure that translation-waiting activity is synchronized
                        unique_lock guard(m_jobs_sent_lock);

                        //Notify that the translation is finished
                        m_jobs_sent_cond.notify_all();
                    }

                    /**
                     * Allows to notify the threads waiting on the translation jobs to be received
                     */
                    void notify_jobs_done() {
                        //Make sure that translation-waiting activity is synchronized
                        unique_lock guard(m_jobs_done_lock);

                        //Notify that the translation is finished
                        m_jobs_done_cond.notify_all();
                    }

                    /**
                     * This function shall be run in a separate thread and send a number of translation job requests to the server.
                     * @param params the client parameters storing, e.g., the input text file name
                     */
                    void send_translation_jobs() {
                        //Declare the variable to store the sentence line
                        TextPieceReader line;

                        LOG_DEBUG << "Reading text from the source file: " << END_LOG;
                        while (!m_is_stopping) {
                            //Get the number of sentences to send in the next request
                            const uint64_t num_to_sent = get_num_of_sentences();
                            //Stores the number of read sentences
                            uint64_t num_read = 0;
                            //text to send for translation
                            string source_text;

                            while (m_source_file.get_first_line(line) && (num_read < num_to_sent)) {
                                LOG_DEBUG << "Read line: '" << line.str() << "'" << END_LOG;

                                //Append the new line to the text to be sent
                                source_text += line.str() + "\n";

                                //Increment the number of read sentences
                                ++num_read;
                            }

                            //If there were lines read then do translation
                            if (num_read != 0) {
                                //Get the new job id
                                const job_id_type job_id = m_id_mgr.get_next_id();

                                //Remove the last sentence new line symbol
                                source_text.substr(0, source_text.size() - 1);

                                //Create the new job data object
                                trans_job_ptr data = new trans_job();

                                //Create the translation job request 
                                data->m_request = new trans_job_request(job_id, m_params.m_source_lang, source_text, m_params.m_target_lang);
                                //Store the number of sentences in the translation request
                                data->m_num_sentences = num_read;
                                //Mark the job sending as good in the administration
                                data->m_status = trans_job_status::STATUS_REQ_INITIALIZED;

                                //Store the translation request
                                m_jobs_list.push_back(data);
                                m_ids_to_jobs_map[job_id] = data;
                            } else {
                                break;
                            }
                        }

                        //Send the translation jobs
                        for (jobs_list_iter_type it = m_jobs_list.begin(); (it != m_jobs_list.end()) && !m_is_stopping; ++it) {
                            //Get the pointer to the translation job data
                            trans_job_ptr data = *it;
                            try {
                                //Send the translation job request
                                m_client.send(data->m_request);
                                //Mark the job sending as good in the administration
                                data->m_status = trans_job_status::STATUS_REQ_SENT_GOOD;
                            } catch (Exception e) {
                                //Log the error message
                                LOG_ERROR << "Error when sending a translation request "
                                        << data->m_request->get_job_id() << ": "
                                        << e.get_message() << END_LOG;
                                //Mark the job sending as failed in the administration
                                data->m_status = trans_job_status::STATUS_REQ_SENT_FAIL;
                            }
                        }

                        //The translation jobs have been sent!
                        notify_jobs_sent();
                    }

                    /**
                     * Allows to compute the number of sentences to send with the next request
                     * @return the number of sentences to send with the next request
                     */
                    uint64_t get_num_of_sentences() {
                        if (m_params.m_min_sent != m_params.m_max_sent) {
                            return m_params.m_min_sent + rand() % (m_params.m_max_sent - m_params.m_min_sent) + 1;
                        } else {
                            return m_params.m_min_sent;
                        }
                    }

                private:
                    //Stores the static instance of the id manager
                    static id_manager<job_id_type> m_id_mgr;

                    //Stores a reference to the translation client parameters
                    const client_config & m_params;

                    //Stores the translation client
                    translation_client m_client;

                    //Stores the source text
                    CStyleFileReader m_source_file;

                    //Stores the list of the translation job objects in the
                    //same order as they were created from the input file
                    jobs_list_type m_jobs_list;

                    //Stores the mapping from the job id to the job data objects
                    jobs_map_type m_ids_to_jobs_map;

                    //Stores the synchronization mutex for notifying that all the translation jobs were sent
                    mutex m_jobs_sent_lock;
                    //The conditional variable for tracking that all the translation jobs were sent
                    condition_variable m_jobs_sent_cond;

                    //Stores the synchronization mutex for notifying that all the translation jobs got responces
                    mutex m_jobs_done_lock;
                    //The conditional variable for tracking that all the translation jobs got responces
                    condition_variable m_jobs_done_cond;

                    //Stores the translation request sending thread
                    thread * m_sending_thread_ptr;

                    //Stores the boolean that is used to notify that we need to stop
                    atomic<bool> m_is_stopping;
                    //Stores a flag indicating that all the translation jobs are sent
                    atomic<bool> m_is_all_jobs_sent;
                    //Stores a flag indicating that all the translation jobs are received
                    atomic<bool> m_is_all_jobs_done;

                    //Store the finished jobs count
                    atomic<uint32_t> m_num_done_jobs;
                };

                id_manager<job_id_type> trans_manager::m_id_mgr(job_id::MINIMUM_JOB_ID);
            }
        }
    }
}

#endif /* TRANS_MANAGER_HPP */

