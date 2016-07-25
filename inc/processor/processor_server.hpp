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

#include "common/messaging/websocket_server.hpp"

#include "processor/processor_parameters.hpp"
#include "processor/processor_manager.hpp"

using namespace std;
using namespace uva::smt::bpbd::common::messaging;

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
                 */
                class processor_server : public websocket_server {
                public:

                    /**
                     * The basic constructor
                     * @param params the balancer parameters
                     */
                    processor_server(const processor_parameters & params)
                    : websocket_server(params.m_server_port),
                    m_manager(params.m_num_threads) {
                        //Provide the manager with the functional for sending
                        //the translation response and getting the adapters
                        m_manager.set_response_sender(bind(&processor_server::send_response, this, _1, _2));
                    }

                    /**
                     * Allows to report the runtime information about the server.
                     */
                    inline void report_run_time_info() {
                        //Report the translation manager' info
                        m_manager.report_run_time_info();
                    }
                    
                    /**
                     * Allows to set a new number of pool threads
                     * @param num_threads the new number of threads
                     */
                    inline void set_num_threads(const int32_t num_threads) {
                        m_manager.set_num_threads(num_threads);
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
                    virtual void language_request(websocketpp::connection_hdl hdl, supp_lang_req_in * msg) override {
                        //Destroy the message
                        delete msg;
                        
                        //ToDo: Throw a non-supported exception
                    }

                    /**
                     * @see websocket_server
                     */
                    virtual void translation_request(websocketpp::connection_hdl hdl, trans_job_req_in * msg) override {
                        //Destroy the message
                        delete msg;
                        
                        //ToDo: Throw a non-supported exception
                    }

                private:
                    //Stores the translation manager
                    processor_manager m_manager;

                };

            }
        }
    }
}


#endif /* PROCESSOR_SERVER_HPP */

