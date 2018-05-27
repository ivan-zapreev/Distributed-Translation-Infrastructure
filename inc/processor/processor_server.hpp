/* 
 * File:   processor_server.hpp
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
 * Created on July 25, 2016, 11:42 AM
 */

#ifndef PROCESSOR_SERVER_HPP
#define PROCESSOR_SERVER_HPP

#include "common/messaging/websocket/websocket_server.hpp"
#include "common/utils/cmd/cmd_line_client.hpp"

#include "processor/processor_parameters.hpp"
#include "processor/processor_manager.hpp"

#include "processor/messaging/proc_req_in.hpp"

using namespace std;
using namespace uva::utils::cmd;
using namespace uva::smt::bpbd::common::messaging;
using namespace uva::smt::bpbd::processor::messaging;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace processor {

                /**
                 * This is the processor server class.
                 * Responsibilities:
                 *      Receives the text processor requests 
                 *      Places the received requests into dispatching queue 
                 *      Sends the processor text responses.
                 * 
                 * @param TLS_CLASS the TLS class which defines the server type and mode
                 */
                template<typename TLS_CLASS>
                class processor_server :
                public websocket_server<TLS_CLASS>,
                public cmd_line_client {
                public:

                    /**
                     * The basic constructor
                     * @param params the balancer parameters
                     */
                    processor_server(const processor_parameters & params)
                    : websocket_server<TLS_CLASS>(params),
                    m_manager(params) {
                        //Provide the manager with the functional for sending
                        //the translation response and getting the adapters
                        m_manager.set_response_sender(
                                bind(&processor_server::send_response, this, _1, _2));
                    }

                    /**
                     * @see cmd_line_client
                     */
                    virtual void report_run_time_info() override {
                        //Report the translation manager' info
                        m_manager.report_run_time_info();
                    }

                    /**
                     * @see cmd_line_client
                     */
                    virtual void set_num_threads(const int32_t num_threads) override {
                        m_manager.set_num_threads(num_threads);
                    }

                    /**
                     * @see cmd_line_client
                     */
                    virtual void request_stop() override {
                        this->stop();
                    }

                protected:

                    /**
                     * @see websocket_server
                     */
                    virtual void before_start_listening() override {
                        //Nothing to be done here
                    }

                    /**
                     * @see websocket_server
                     */
                    virtual void after_stop_listening() override {
                        //Stop the translation manager.
                        m_manager.stop();
                    }

                    /**
                     * @see websocket_server
                     */
                    virtual void open_session(websocketpp::connection_hdl hdl) override {
                        m_manager.open_session(hdl);
                    }

                    /**
                     * @see websocket_server
                     */
                    virtual void close_session(websocketpp::connection_hdl hdl) override {
                        m_manager.close_session(hdl);
                    }

                    /**
                     * @see websocket_server
                     */
                    virtual void pre_process_request(
                            websocketpp::connection_hdl hdl, proc_req_in * msg) override {
                        m_manager.pre_process(hdl, msg);
                    }

                    /**
                     * @see websocket_server
                     */
                    virtual void post_process_request(
                            websocketpp::connection_hdl hdl, proc_req_in * msg) override {
                        m_manager.post_process(hdl, msg);
                    }

                private:
                    //Stores the translation manager
                    processor_manager m_manager;

                };

#if IS_TLS_SUPPORT
                //Add the TLS servers as an option in case the TLS support is enabled
                typedef processor_server<server_hs_with_tls_old> processor_server_tls_old;
                typedef processor_server<server_hs_with_tls_int> processor_server_tls_int;
                typedef processor_server<server_hs_with_tls_mod> processor_server_tls_mod;
#endif
                //Add the no-TLS server for when TLS is not enabled or needed
                typedef processor_server<server_hs_without_tls> processor_server_no_tls;

            }
        }
    }
}


#endif /* PROCESSOR_SERVER_HPP */

