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

#ifndef TRANS_MANAGER_HPP
#define TRANS_MANAGER_HPP

#include <string>
#include <vector>
#include <fstream>
#include <cstdlib>
#include <iostream>
#include <unordered_map>

#include "common/utils/id_manager.hpp"
#include "common/utils/text/string_utils.hpp"
#include "common/utils/threads/threads.hpp"

#include "common/messaging/status_code.hpp"
#include "common/messaging/job_id.hpp"

#include "client/client_consts.hpp"
#include "client/client_parameters.hpp"
#include "client/client_manager.hpp"
#include "client/trans_job.hpp"
#include "client/trans_job_status.hpp"

#include "client/messaging/trans_job_req_out.hpp"
#include "client/messaging/trans_job_resp_in.hpp"

using namespace std;
using namespace uva::utils;
using namespace uva::utils::text;
using namespace uva::utils::threads;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace client {

                /**
                 * This is the client side translation manager class. It's task
                 * is to get the source text from a file and then split it into
                 * a number of translation jobs that will be sent to the
                 * translation server. The finished translation jobs are collected
                 * and once all of them are finished the resulting text is written
                 * into the output file.
                 */
                class trans_manager : public client_manager<msg_type::MESSAGE_TRANS_JOB_RESP, trans_job_resp_in> {
                public:
                    //Stores the absolute allowed minimum of sentences to be sent by translation request.
                    //Note that if the translation text is smaller then this value is overruled.
                    static constexpr uint64_t MIN_SENTENCES_PER_REQUEST = 1;

                    //Define the type for the list of the translation data objects
                    typedef vector<trans_job_ptr> jobs_list_type;
                    //Define the type for the map from job id to job data
                    typedef unordered_map<job_id_type, trans_job_ptr> jobs_map_type;

                    /**
                     * This is the basic constructor needed to 
                     * @param params the translation client parameters
                     * @param input the input stream to read the source text from
                     * @param output the output stream to write the target text into
                     */
                    trans_manager(const client_parameters & params, stringstream & input, stringstream & output)
                    : client_manager(params.m_trans_uri, "translation"),
                    m_params(params), m_input(input), m_output(output),
                    m_act_num_req(0), m_exp_num_resp(0), m_act_num_resp(0) {
                        //Create the list of translation jobs
                        create_translation_jobs();
                    }

                    /**
                     * The basic destructor class
                     */
                    virtual ~trans_manager() {
                        //Clean the internal administration, just delete the translation jobs 
                        for (auto it = m_jobs_list.begin(); (it != m_jobs_list.end()); ++it) {
                            delete (*it);
                        }
                    }

                    /**
                     * Allows to start the translation process
                     */
                    inline void start() {
                        //Check that the minimum is not larger than the maximum
                        ASSERT_CONDITION_THROW((m_params.m_max_sent < m_params.m_min_sent),
                                string("The minimum number of sentences to be sent (") + to_string(m_params.m_min_sent)
                                + string(") is larger that the maximum (") + to_string(m_params.m_max_sent) + string(")!"));

                        //Check that the minimum is not too small
                        ASSERT_CONDITION_THROW((m_params.m_min_sent < MIN_SENTENCES_PER_REQUEST),
                                string("The minimum number of sentences to be sent (") + to_string(m_params.m_min_sent)
                                + string(") should be larger or equal than ") + to_string(MIN_SENTENCES_PER_REQUEST));

                        //Call the client manager's method
                        client_manager::start();
                    }

                protected:

                    /**
                     * @see client_manager
                     */
                    virtual size_t get_act_num_req() override {
                        return m_act_num_req;
                    }

                    /**
                     * @see client_manager
                     */
                    virtual size_t get_exp_num_resp() override {
                        return m_exp_num_resp;
                    }

                    /**
                     * @see client_manager
                     */
                    virtual void send_job_requests(websocket_client & client) override {
                        //Send the translation jobs
                        for (auto it = m_jobs_list.begin(); (it != m_jobs_list.end()) && !is_stopping(); ++it) {
                            //Get the pointer to the translation job data
                            trans_job_ptr data = *it;
                            try {
                                //Send the translation job request
                                client.send(data->m_request);
                                //Mark the job sending as good in the administration
                                data->m_status = trans_job_status::STATUS_REQ_SENT_GOOD;
                                //Increment the number of sent requests
                                ++m_act_num_req;
                            } catch (std::exception & e) {
                                //Log the error message
                                LOG_ERROR << "Error when sending a translation request "
                                        << data->m_request->get_job_id() << ": " << e.what() << END_LOG;
                                //Mark the job sending as failed in the administration
                                data->m_status = trans_job_status::STATUS_REQ_SENT_FAIL;
                            }

                            //The number of expected responses is equal to the number of actual requests
                            m_exp_num_resp = m_act_num_req;
                        }
                    }

                    /**
                     * @see client_manager
                     */
                    virtual void set_job_response(trans_job_resp_in * trans_job_resp) override {
                        //Get the job id to work with
                        const job_id_type job_id = trans_job_resp->get_job_id();

                        LOG_DEBUG << "Got the translation job response for job id: " << to_string(job_id) << END_LOG;

                        //Check if the job with the given id is known
                        if (m_ids_to_jobs_map.find(job_id) != m_ids_to_jobs_map.end()) {
                            //Register the job in the administration
                            m_ids_to_jobs_map[job_id]->m_response = trans_job_resp;

                            //Set the translation job status as received 
                            m_ids_to_jobs_map[job_id]->m_status = trans_job_status::STATUS_RES_RECEIVED;

                            //Increment the number of actual responses
                            ++m_act_num_resp;

                            LOG_INFO1 << "The job " << job_id << " is finished, "
                                    << m_act_num_resp << "/" << m_exp_num_resp
                                    << "." << END_LOG;
                        } else {
                            THROW_EXCEPTION(string("The received job response id ") +
                                    to_string(job_id) + string(" is not known!"));
                        }
                    }

                    /**
                     * @see client_manager
                     */
                    virtual void process_results() override {
                        //Get the names for the translation and info files
                        const string info_file_name = m_params.m_target_file + ".log";

                        //Open the report file and the info file
                        ofstream info_file(info_file_name);
                        //Declare the variable to store the line cursor
                        uint32_t line_cursor = 1;

                        LOG_INFO << "Storing logging into '" << info_file_name << "'" << END_LOG;

                        try {
                            ASSERT_CONDITION_THROW(!info_file.is_open(), string("Could not open: ") + info_file_name);

                            //Go through the translation job data and write it into the files
                            for (auto it = m_jobs_list.begin(); (it != m_jobs_list.end()); ++it) {
                                //Get the translation job data
                                const trans_job_ptr job = *it;

                                //Compute the first and last sentence numbers for the case the job is not completed
                                const uint32_t fis = line_cursor;
                                const uint32_t lis = fis + job->m_num_sentences - 1;
                                //Get the client-side job status
                                const trans_job_status & status = job->m_status;

                                //Log the client status
                                info_file << "----------------------------------------------------" << std::endl;
                                info_file << "Job id: " << to_string(job->m_request->get_job_id())
                                        << ", sentences [" << to_string(fis) << ":"
                                        << to_string(lis) << "], client status: '" << status << "'" << std::endl;

                                //If the status is that the response is received, log it
                                if (status == trans_job_status::STATUS_RES_RECEIVED) {
                                    process_job_result(fis, lis, job, m_output, info_file);
                                }

                                //Set the line cursor to the next line
                                line_cursor = lis + 1;
                            }
                        } catch (std::exception & e) {
                            LOG_ERROR << "Could not dump results: " << e.what() << END_LOG;
                        }

                        //Close the info file
                        info_file.close();
                    }

                private:
                    //Stores the static instance of the id manager
                    static id_manager<job_id_type> m_id_mgr;

                    //Stores the reference to the client parameters
                    const client_parameters & m_params;

                    //Stores the reference to the input stream storing the source text
                    stringstream & m_input;
                    //Stores the reference to the output stream for the target text
                    stringstream & m_output;

                    //Stores the list of the translation job objects in the
                    //same order as they were created from the input file
                    jobs_list_type m_jobs_list;

                    //Stores the mapping from the job id to the job data objects
                    jobs_map_type m_ids_to_jobs_map;

                    //Store the actual number of sent requests
                    size_t m_act_num_req;
                    //Store the expected number of responses
                    size_t m_exp_num_resp;
                    //Store the actual number of responses
                    atomic<uint32_t> m_act_num_resp;

                    /**
                     * Allows to compute the number of sentences to send with the next request
                     * @return the number of sentences to send with the next request
                     */
                    inline uint64_t get_num_of_sentences() {
                        if (m_params.m_min_sent != m_params.m_max_sent) {
                            return m_params.m_min_sent + rand() % (m_params.m_max_sent - m_params.m_min_sent) + 1;
                        } else {
                            return m_params.m_min_sent;
                        }
                    }

                    /**
                     * This function shall be run in a separate thread and create a number of translation jobs.
                     */
                    void create_translation_jobs() {
                        LOG_DEBUG << "Reading text from the source file ..." << END_LOG;
                        bool is_done = false;
                        while (!is_done) {
                            //Get the number of sentences to send in the next request
                            const uint64_t num_to_sent = get_num_of_sentences();
                            //Stores the number of read sentences
                            uint64_t num_read = 0;
                            //text to send for translation
                            vector<string> source_text;

                            LOG_DEBUG1 << "Planning to send  " << num_to_sent << " sentences with the next request" << END_LOG;
                            char buffer[LINE_MAX_BYTES_LEN];
                            while ((num_read < num_to_sent) && m_input.getline(buffer, sizeof (buffer))) {
                                //Obtain the read source sentence
                                string source_sent(buffer);

                                LOG_DEBUG2 << "Read line: '" << source_sent << "'" << END_LOG;

                                //Append the new line to the text to be sent
                                source_text.push_back(source_sent);

                                //Increment the number of read sentences
                                ++num_read;

                                LOG_DEBUG2 << "Read " << num_read << " input sentences!" << END_LOG;
                            }

                            LOG_DEBUG1 << "Read " << num_read << " sentences to be sent with the next request" << END_LOG;

                            //If there were lines read then do translation
                            if (num_read != 0) {
                                //Get the new job id
                                const job_id_type job_id = m_id_mgr.get_next_id();

                                //Create the new job data object
                                trans_job_ptr data = new trans_job();

                                //Create the translation job request 
                                data->m_request = new trans_job_req_out(job_id, m_params.m_priority,
                                        m_params.m_source_lang, source_text, m_params.m_target_lang,
                                        m_params.m_is_trans_info);
                                //Store the number of sentences in the translation request
                                data->m_num_sentences = num_read;
                                //Mark the job sending as good in the administration
                                data->m_status = trans_job_status::STATUS_REQ_INITIALIZED;

                                //Store the translation request
                                m_jobs_list.push_back(data);
                                m_ids_to_jobs_map[job_id] = data;
                            } else {
                                is_done = true;
                            }
                        }

                        LOG_DEBUG << "Finished reading text from the source file!" << END_LOG;
                    }

                    /**
                     * Allows to store the response data
                     * @param fis the first sentence number 
                     * @param resp the translation job response
                     * @param output the stream to write the translated text into
                     * @param info_file the file to write the translation info into
                     */
                    inline void process_task_result(uint32_t fis, trans_job_resp_in * resp,
                            stringstream & output, ofstream & info_file) {
                        //If the result is ok or partial then just put the text into the file
                        const trans_sent_data_in * sent_data = resp->next_send_data();

                        //Check if there is sentence data present
                        if (sent_data != NULL) {
                            while (sent_data != NULL) {
                                LOG_DEBUG3 << "Translation: " << sent_data->get_trans_text() << END_LOG;
                                //Dump the translated text
                                output << sent_data->get_trans_text() << std::endl;
                                //Get the sentence status code
                                const status_code code = sent_data->get_status_code();
                                //Dump the status code and message and the translation info such as stack loads
                                info_file << "--" << std::endl << "Sentence: " << to_string(fis)
                                        << " translation status: '" << code << "'";
                                //Log the message only if it is present.
                                if (!sent_data->get_status_msg().empty()) {
                                    info_file << ", message: " << sent_data->get_status_msg();
                                }
                                info_file << std::endl;
                                //Log the stack loads if present
                                if (sent_data->has_stack_load()) {
                                    info_file << "Multi-stack loads: [ ";
                                    const Value & loads = sent_data->get_stack_load();
                                    for (auto iter = loads.Begin(); iter != loads.End(); ++iter) {
                                        info_file << iter->GetUint() << "% ";
                                    }
                                    info_file << "]" << std::endl;
                                }
                                //Move to the next sentence if present
                                sent_data = resp->next_send_data();
                                //Increment the sentence number
                                ++fis;
                            }
                        } else {
                            //There is no sentence data present!
                            LOG_DEBUG << "Missing target sentences for job: " << resp->get_job_id() << END_LOG;
                        }
                    }

                    /**
                     * Allows to write the received translation job replies into the file
                     * @param fis the first sentence number 
                     * @param lis the last sentence number
                     * @param job the translation job data
                     * @param output the stream to write the translation result into
                     * @param info_file the file to write the translation info into
                     */
                    inline void process_job_result(const uint32_t fis, const uint32_t lis,
                            const trans_job_ptr job, stringstream & output, ofstream & info_file) {
                        //Get the response pointer
                        trans_job_resp_in * resp = job->m_response;

                        try {
                            //The job response is received but it can still be fully or partially canceled or be an error
                            const status_code code = resp->get_status_code();

                            //Log the error to the screen
                            if (code == status_code::RESULT_ERROR) {
                                LOG_ERROR << resp->get_status_msg() << END_LOG;
                            }

                            //Dump the server response info
                            info_file << "Server response status: '" << code << "', "
                                    << "message: " << resp->get_status_msg() << std::endl;

                            //Dump the sentences data
                            process_task_result(fis, resp, output, info_file);
                        } catch (std::exception & e) {
                            LOG_ERROR << "Could not dump data for sentences [" << to_string(fis)
                                    << ":" << to_string(lis) << "]: " << e.what() << END_LOG;
                        }
                    }
                };

                id_manager<job_id_type> trans_manager::m_id_mgr(job_id::MINIMUM_JOB_ID);
            }
        }
    }
}

#endif /* TRANS_MANAGER_HPP */

