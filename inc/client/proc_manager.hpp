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

#include "common/utils/id_manager.hpp"
#include "common/utils/string_utils.hpp"
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
                    typedef function<proc_req_out * (const job_id_type) > request_creator;

                    /**
                     * This is the basic constructor needed to 
                     * @param params the translation client parameters
                     * @param name the name we instantiate this client for, is used for logging.
                     * @param input the input stream to read the source text from
                     * @param input_lang the reference to a string to get the input language
                     * @param output the output stream to write the target text into
                     * @param output_lang the reference to a string to store the output language
                     * @param req_crt_func the reference to the request creator function
                    ;
                     */
                    proc_manager(const client_parameters & params, const string name,
                            stringstream & input, const string & input_lang,
                            stringstream & output, string & output_lang, const request_creator &req_crt_func)
                    : client_manager<MSG_TYPE, proc_resp_in>(params.m_trans_uri, name),
                    m_params(params), m_input(input), m_input_lang(input_lang),
                    m_output(output), m_output_lang(output_lang), m_req_crt_func(req_crt_func),
                    m_act_num_req(0), m_exp_num_resp(0), m_act_num_resp(0) {
                    }

                    /**
                     * The basic destructor class
                     */
                    virtual ~proc_manager() {
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
                            //Issue a new job id
                            m_job_id = m_id_mgr.get_next_id();

                            //Store the reference to the client
                            m_client = &client;

                            //Process the text in chunks
                            process_utf8_chunks<MESSAGE_MAX_WCHARS_LEN>(m_input,
                                    bind(&proc_manager::send_utf8_chunk_msg, this, _1, _2, _3));
                        }
                    }

                    /**
                     * @see client_manager
                     */
                    virtual void set_job_response(proc_resp_in * job_resp_msg) override {
                        //ToDo: Implement
                        THROW_NOT_IMPLEMENTED();
                    }

                    /**
                     * @see client_manager
                     */
                    virtual void process_results() override {
                        //ToDo: Implement
                        THROW_NOT_IMPLEMENTED();
                    }

                private:
                    //Stores the static instance of the id manager
                    static id_manager<job_id_type> m_id_mgr;

                    //Stores the reference to the client parameters
                    const client_parameters & m_params;

                    //Stores the reference to the input stream storing the source text
                    stringstream & m_input;
                    //Stores the reference to a string to get the input language
                    const string & m_input_lang;

                    //Stores the reference to the output stream for the target text
                    stringstream & m_output;
                    //Stores the reference to a string to store the output language
                    string & m_output_lang;

                    //Stores the request creator function
                    const request_creator m_req_crt_func;

                    //Store the actual number of sent requests
                    size_t m_act_num_req;
                    //Store the expected number of responses
                    size_t m_exp_num_resp;
                    //Store the actual number of responses
                    atomic<uint32_t> m_act_num_resp;

                    //Stores the issued job id to be used. Is initialized before sending the job.
                    job_id_type m_job_id;
                    //Stores the pointer to the client. Is initialized before sending the job.
                    generic_client * m_client;

                    /**
                     * Allows to send a chunk of utf8 characters to the server.
                     * Does nothing if the client is stopping.
                     * @param buffer the buffer storing the characters
                     * @param num_chunks the total number of chunks to send 
                     * @param chunk_idx the current chunk index starting with 0.
                     */
                    inline void send_utf8_chunk_msg(wchar_t * buffer, const size_t num_chunks, const size_t chunk_idx) {
                        if (!client_manager<MSG_TYPE, proc_resp_in>::is_stopping()) {
                            //Get the error response
                            proc_req_out * req = m_req_crt_func(m_job_id);

                            //Set the language
                            req->set_language(m_input_lang);

                            //Set text the text piece index and the number of text pieces
                            wstring ws(buffer);
                            req->set_chunk(string(ws.begin(), ws.end()), chunk_idx, num_chunks);

                            //Send the translation job request
                            m_client->send(req);

                            //Delete the response
                            delete req;
                        }
                    }
                };

                template<msg_type MSG_TYPE>
                id_manager<job_id_type> proc_manager<MSG_TYPE>::m_id_mgr(job_id::MINIMUM_JOB_ID);
            }
        }
    }
}

#endif /* PROC_MANAGER_HPP */

