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

using namespace std;
using namespace uva::utils;
using namespace uva::utils::text;
using namespace uva::utils::threads;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace client {

                /**
                 * This is the client side processor manager class. It's task
                 * is to send the pre/post- processor requests to the server
                 * and to receive them back.
                 */
                template<msg_type MSG_TYPE, typename RESPONSE_TYPE>
                class proc_manager : public client_manager<MSG_TYPE, RESPONSE_TYPE> {
                public:

                    /**
                     * This is the basic constructor needed to 
                     * @param params the translation client parameters
                     * @param name the name we instantiate this client for, is used for logging.
                     * @param input the input stream to read the source text from
                     * @param output the output stream to write the target text into
                     */
                    proc_manager(const client_parameters & params, const string name, stringstream & input, stringstream & output)
                    : client_manager<MSG_TYPE, RESPONSE_TYPE>(params.m_trans_uri, name),
                    m_params(params), m_input(input), m_output(output),
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
                        //ToDo: Implement
                        THROW_NOT_IMPLEMENTED();
                    }

                    /**
                     * @see client_manager
                     */
                    virtual void set_job_response(RESPONSE_TYPE * job_resp_msg) override {
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
                    //Stores the reference to the client parameters
                    const client_parameters & m_params;

                    //Stores the reference to the input stream storing the source text
                    stringstream & m_input;
                    //Stores the reference to the output stream for the target text
                    stringstream & m_output;

                    //Store the actual number of sent requests
                    size_t m_act_num_req;
                    //Store the expected number of responses
                    size_t m_exp_num_resp;
                    //Store the actual number of responses
                    atomic<uint32_t> m_act_num_resp;

                };
            }
        }
    }
}

#endif /* PROC_MANAGER_HPP */

