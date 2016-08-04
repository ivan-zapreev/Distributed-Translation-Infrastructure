/* 
 * File:   proc_manager.hpp
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
 * Created on August 1, 2016, 12:10 PM
 */

#ifndef PROC_MANAGER_HPP
#define PROC_MANAGER_HPP

#include "common/utils/text/utf8_utils.hpp"
#include "common/utils/threads/threads.hpp"

#include "client/client_consts.hpp"
#include "client/client_parameters.hpp"
#include "client/generic_client.hpp"
#include "client/client_manager.hpp"

#include "client/messaging/proc_req_out.hpp"
#include "client/messaging/proc_resp_in.hpp"

using namespace std;
using namespace uva::utils;
using namespace uva::utils::text;
using namespace uva::utils::threads;

using namespace uva::smt::bpbd::client::messaging;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace client {

                /**
                 * This is the client side processor manager class. It's task
                 * is to send the pre/post- processor requests to the server
                 * and to receive them back.
                 */
                template<msg_type MSG_TYPE>
                class proc_manager : public client_manager<MSG_TYPE, proc_resp_in> {
                public:

                    //Declare the function that will be used to create a processor request
                    typedef function<proc_req_out * (const string) > request_creator;

                    /**
                     * This is the basic constructor needed to 
                     * @param proc_uri the uri of the processing server
                     * @param name the name we instantiate this client for, is used for logging.
                     * @param input the input stream to read the source text from
                     * @param output the output stream to write the target text into
                     * @param lang the reference to a string thaty stores the language
                     *        is to be updated with the language coming from the server
                     * @param job_token [in/out] the job token can be updated by the server
                     * @param req_crt_func the reference to the request creator function
                     */
                    proc_manager(const string & proc_uri, const string name,
                            stringstream & input, stringstream & output, string & lang,
                            string & job_token, const request_creator &req_crt_func)
                    : client_manager<MSG_TYPE, proc_resp_in>(proc_uri, name),
                    m_input(input), m_output(output), m_lang(lang), m_job_token(job_token),
                    m_req_crt_func(req_crt_func), m_responses(NULL), m_act_num_req(0),
                    m_exp_num_resp(0), m_act_num_resp(0) {
                    }

                    /**
                     * The basic destructor class
                     */
                    virtual ~proc_manager() {
                        //Delete the responses
                        delete_responses();
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
                    virtual void send_job_requests(generic_client & client) override {
                        if (!client_manager<MSG_TYPE, proc_resp_in>::is_stopping()) {
                            //Store the reference to the client
                            m_client = &client;

                            //Process the text in chunks
                            process_utf8_chunks<MESSAGE_MAX_CHAR_LEN>(m_input,
                                    bind(&proc_manager::send_utf8_chunk_msg, this, _1, _2, _3));

                            //Set the expected number of responses to be the
                            //actual number of requests. This value will change
                            //once the first response is received, and we get
                            //the number of chunks from the job response.
                            m_exp_num_resp = m_act_num_req;
                        }
                    }

                    /**
                     * @see client_manager
                     */
                    virtual void set_job_response(proc_resp_in * job_resp_msg) override {
                        //Synchronize with sending to prevent resources overlap
                        unique_guard guard(m_sr_lock);

                        if (job_resp_msg->get_status_code() == status_code::RESULT_OK) {
                            //If this is the first response then get the language, the number of
                            //expected responses and allocate an array to store the responses in. 
                            if (m_responses == NULL) {
                                //Update the language
                                m_lang = job_resp_msg->get_language();
                                //Update the number of chunks
                                m_exp_num_resp = job_resp_msg->get_num_chunks();
                                //Allocate the chunks array
                                m_responses = new proc_resp_in_ptr[m_exp_num_resp]();
                            }

                            //Get/update the job token
                            m_job_token = job_resp_msg->get_job_token();

                            //Get the chunk index
                            const size_t chunk_idx = job_resp_msg->get_chunk_idx();

                            //Check that the chunk idx is within the bounds

                            ASSERT_CONDITION_THROW((chunk_idx >= m_exp_num_resp),
                                    string("Processor response job token: ") + m_job_token + string(" has") +
                                    string(" chunk idx: ") + to_string(chunk_idx) + string(", the max expected") +
                                    string(" chunk idx is ") + to_string(m_exp_num_resp - 1) +
                                    string(", ignoring!"));

                            //Check that the chunk is not yet received
                            ASSERT_CONDITION_THROW((m_responses[chunk_idx] != NULL),
                                    string("Processor response job token: ") + m_job_token +
                                    string(" has chunk idx: ") + to_string(chunk_idx) +
                                    string(", already ") + string("received, ignoring!"));

                            //Store the response under the chunk index
                            m_responses[chunk_idx] = job_resp_msg;

                            //Increment the actual number of response
                            ++m_act_num_resp;

                            LOG_INFO1 << "The processor job response chunk " << m_act_num_resp
                                    << "/" << m_exp_num_resp << " is received." << END_LOG;
                        } else {
                            //To stop the process set the number of expected jobs to zero
                            m_exp_num_resp = 0;
                            //Throw an exception
                            THROW_EXCEPTION(job_resp_msg->get_status_msg());
                        }
                    }

                    /**
                     * @see client_manager
                     */
                    virtual void process_results() override {
                        //Synchronize with sending and receiving to prevent resources overlap
                        unique_guard guard(m_sr_lock);

                        //Check that the responses are set
                        ASSERT_CONDITION_THROW((m_responses == NULL),
                                "There are no responses to process!");

                        //Iterate through the responses and delete the non-null ones
                        for (size_t idx = 0; idx < m_exp_num_resp; ++idx) {
                            //Check that the chunk data is present!
                            ASSERT_CONDITION_THROW((m_responses[idx] == NULL),
                                    string("The response chunk: ") + to_string(idx) +
                                    string(" is missing, failed processor job!"));

                            //Put the chunk text into the output stream
                            m_output << m_responses[idx]->get_chunk();
                        }

                        //Delete responses
                        delete_responses();
                    }

                private:
                    //Stores the reference to the input stream storing the source text
                    stringstream & m_input;
                    //Stores the reference to the output stream for the target text
                    stringstream & m_output;

                    //Stores the reference to a string storing the language, is
                    //to be updated by the language coming from the server.
                    string & m_lang;

                    //Stores the reference to the job token string which is to be sent
                    //to the server with the processor job and the processor response
                    //might contain a new token to set.
                    string & m_job_token;

                    //Stores the request creator function
                    const request_creator m_req_crt_func;

                    //Stores the pointer to the array of responses
                    proc_resp_in_ptr * m_responses;

                    //Store the actual number of sent requests
                    size_t m_act_num_req;
                    //Store the expected number of responses
                    size_t m_exp_num_resp;
                    //Store the actual number of responses
                    atomic<uint32_t> m_act_num_resp;
                    //Stores the synchronization mutex for synchronizing sending and receiving code
                    mutex m_sr_lock;

                    //Stores the pointer to the client. Is initialized before sending the job.
                    generic_client * m_client;

                    /**
                     * Allows to delete responses
                     */
                    inline void delete_responses() {
                        if (m_responses != NULL) {
                            //Iterate through the responses and delete the non-null ones
                            for (size_t idx = 0; idx < m_exp_num_resp; ++idx) {
                                if (m_responses[idx] != NULL) {
                                    //Delete the request
                                    delete m_responses[idx];
                                }
                            }
                            //Delete the array
                            delete[] m_responses;
                            //Set the responses to NULL
                            m_responses = NULL;
                        }
                    }

                    /**
                     * Allows to send a chunk of utf8 characters to the server.
                     * Does nothing if the client is stopping.
                     * @param chunk the string storing the read chunk
                     * @param num_chunks the total number of chunks to send 
                     * @param chunk_idx the current chunk index starting with 0.
                     */
                    inline void send_utf8_chunk_msg(const string & chunk, const size_t num_chunks, const size_t chunk_idx) {
                        //Synchronize with receiving to prevent resources overlap
                        unique_guard guard(m_sr_lock);

                        if (!client_manager<MSG_TYPE, proc_resp_in>::is_stopping()) {
                            //Get the request message
                            proc_req_out * req = m_req_crt_func(m_job_token);

                            //Set the language
                            req->set_language(m_lang);

                            //Set text the text piece index and the number of text pieces
                            req->set_chunk(chunk, chunk_idx, num_chunks);

                            try {
                                //Send the translation job request
                                m_client->send(req);

                                //Increment the number of sent requests
                                ++m_act_num_req;

                                LOG_INFO1 << "The processor job request chunk " << (chunk_idx + 1)
                                        << "/" << num_chunks << " is sent." << END_LOG;

                            } catch (std::exception & e) {
                                //Log the error message
                                LOG_ERROR << "Failed sending the processor job chunk " << (chunk_idx + 1)
                                        << "/" << num_chunks << ": " << e.what() << END_LOG;
                            }

                            //Delete the response
                            delete req;
                        }
                    }
                };

                /**
                 * Define the class for the pre-processor manager
                 */
                class pre_proc_manager : public proc_manager<msg_type::MESSAGE_PRE_PROC_JOB_RESP> {
                public:

                    /**
                     * This is the basic constructor needed to 
                     * @param params the translation client parameters
                     * @param input the input stream to read the source text from
                     * @param output the output stream to write the target text into
                     * @param lang the reference to a string thaty stores the language
                     *        is to be updated with the language coming from the server
                     * @param job_token [in/out] the job token can be updated by the server
                     */
                    pre_proc_manager(const client_parameters & params, stringstream & input,
                            stringstream & output, string & lang, string & job_token)
                    : proc_manager(params.m_pre_uri, "pre-processor", input,
                    output, lang, job_token, &proc_req_out::get_pre_proc_req) {
                    }
                };

                /**
                 * Define the class for the post-processor manager
                 */
                class post_proc_manager : public proc_manager<msg_type::MESSAGE_POST_PROC_JOB_RESP> {
                public:

                    /**
                     * This is the basic constructor needed to 
                     * @param params the translation client parameters
                     * @param input the input stream to read the source text from
                     * @param output the output stream to write the target text into
                     * @param lang the reference to a string thaty stores the language
                     *        is to be updated with the language coming from the server
                     * @param job_token [in/out] the job token can be updated by the server
                     */
                    post_proc_manager(const client_parameters & params, stringstream & input,
                            stringstream & output, string & lang, string & job_token)
                    : proc_manager(params.m_post_uri, "post-processor", input,
                    output, lang, job_token, &proc_req_out::get_post_proc_req) {
                    }
                };
            }
        }
    }
}

#endif /* PROC_MANAGER_HPP */

