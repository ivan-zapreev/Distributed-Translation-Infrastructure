/* 
 * File:   balancer_server.hpp
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
 * Created on July 7, 2016, 12:09 PM
 */

#ifndef BALANCER_SERVER_HPP
#define BALANCER_SERVER_HPP

#include <iostream>
#include <functional>

#define ASIO_STANDALONE
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"

#include "common/messaging/status_code.hpp"
#include "common/messaging/incoming_msg.hpp"

#include "server/messaging/supp_lang_req_in.hpp"
#include "server/messaging/trans_job_req_in.hpp"
#include "server/messaging/supp_lang_resp_out.hpp"
#include "server/messaging/trans_job_resp_out.hpp"

#include "balancer/balancer_parameters.hpp"
#include "balancer/adapters_manager.hpp"
#include "balancer/translation_manager.hpp"

using namespace std;
using namespace std::placeholders;

using namespace websocketpp;
using namespace websocketpp::frame;
using namespace websocketpp::lib;
using namespace websocketpp::log;

using namespace uva::utils::logging;
using namespace uva::utils::exceptions;

using namespace uva::smt::bpbd::common::messaging;

using namespace uva::smt::bpbd::server::messaging;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace balancer {

                /**
                 * This is the balancer server class, this class is a singleton:
                 * Responsibilities:
                 *      Receives the supported languages requests 
                 *      Sends the supported languages responses.
                 *      Receives the translation requests
                 *      Places the received requests into dispatching queue 
                 *      Sends the translation responses.
                 */
                class balancer_server {
                public:
                    typedef websocketpp::server<websocketpp::config::asio> server;

                    /**
                     * Allows to configure the balancer server
                     * @param params the parameters from which the server will be configured
                     */
                    static inline void configure(const balancer_parameters & params) {
                        m_params = &params;

                        //Add the handlers for the connection events
                        m_server.set_open_handler(translation_manager::open_session);
                        m_server.set_close_handler(translation_manager::close_session);
                        m_server.set_fail_handler(translation_manager::close_session);

                        //Bind the handlers we are using
                        m_server.set_message_handler(balancer_server::on_message);

                        //Configure the translation manager
                        translation_manager::configure(params, balancer_server::send_response);

                        //Set up access channels to only log interesting things
                        m_server.clear_access_channels(log::alevel::all);
                        m_server.set_access_channels(log::alevel::none);
                        m_server.clear_error_channels(log::alevel::all);
                        m_server.set_error_channels(log::alevel::none);

                        //Initialize the Asio transport policy
                        m_server.init_asio();

                        //Set the port that the server will listen to
                        m_server.listen(params.m_server_port);
                    }

                    /**
                     * The main method to run in the server thread
                     */
                    static inline void run() {
                        LOG_DEBUG << "Starting the balancer server ..." << END_LOG;
                        m_server.start_accept();
                        m_server.run();
                    }
                    
                    /**
                     * Allows to stop listening to the incoming messages
                     */
                    static inline void stop() {
                        LOG_USAGE << "Stopping the balancer server ..." << END_LOG;

                        LOG_DEBUG << "Removing the on_close handler." << END_LOG;
                        //Remove the on_close handler
                        m_server.set_close_handler(NULL);

                        LOG_DEBUG << "Stop listening to the new connections." << END_LOG;
                        //Stop listening to the (new) connections
                        m_server.stop_listening();

                        //Stop the translation manager
                        translation_manager::stop();
                        
                        LOG_USAGE << "Stopping the WEBSOCKET server." << END_LOG;
                        //Stop the server
                        m_server.stop();
                    }

                protected:

                    /**
                     * Allows to send the response to the client associated with the given connection handler.
                     * @param hdl the connection handler to identify the connection
                     * @param reply_str the response string
                     */
                    static inline void send_response(connection_hdl hdl, const string reply_str) {
                        LOG_DEBUG << "Sending the job response: ____" << reply_str << "____" << END_LOG;

                        //Declare the error code
                        lib::error_code ec;

                        //Send/schedule the translation job reply
                        m_server.send(hdl, reply_str, opcode::text, ec);

                        //Locally report sending error
                        if (ec) {
                            LOG_ERROR << "Failed sending error '" << reply_str << "' reply: " << ec.message() << END_LOG;
                        }

                        LOG_DEBUG << "The job response: ____" << reply_str << "____ is sent!" << END_LOG;
                    }

                    /**
                     * Is called when the message is received by the server
                     * @param hdl the connection handler
                     * @param raw_msg the received message
                     */
                    static inline void on_message(websocketpp::connection_hdl hdl, server::message_ptr raw_msg) {
                        LOG_DEBUG << "Received a message!" << END_LOG;

                        //Create an empty json message
                        incoming_msg * jmsg = new incoming_msg();

                        //De-serialize the message and then handle based on its type
                        try {
                            string raw_msg_str = raw_msg->get_payload();

                            LOG_DEBUG << "Received JSON msg: " << raw_msg_str << END_LOG;

                            //De-serialize the message
                            jmsg->de_serialize(raw_msg_str);

                            //Handle the request message based on its type
                            switch (jmsg->get_msg_type()) {
                                case msg_type::MESSAGE_TRANS_JOB_REQ:
                                    translation_manager::register_translation_request(hdl, new trans_job_req_in(jmsg));
                                    break;
                                case msg_type::MESSAGE_SUPP_LANG_REQ:
                                    language_request(hdl, jmsg);
                                    break;
                                default:
                                    THROW_EXCEPTION(string("Unsupported request type: ") + to_string(jmsg->get_msg_type()));
                            }
                        } catch (std::exception & e) {
                            //Send the error response, NOTE! This is not a JSON we are sending
                            //back, but just a string, as someone violated the protocol!
                            send_response(hdl, e.what());
                        }
                    }
                    
                    /**
                     * This method allows to handle the supported-languages request
                     * This function must not throw!
                     * @param hdl the connection handler
                     * @param msg a pointer to the JSON object storing the request data, not NULL.
                     */
                    static inline void language_request(websocketpp::connection_hdl hdl, const incoming_msg * msg) {
                        //Create the supported languages request message. This is done so that
                        //once the destructor is called the incoming message is destroyed.
                        supp_lang_req_in supp_lang_req(msg);

                        //Send the response supported languages response
                        send_response(hdl, adapters_manager::get_supported_lang_resp_data());
                    }
                    
                private:
                    //Stores the pointer to the balancer parameters
                    static const balancer_parameters * m_params;
                    //Stores the server object
                    static server m_server;

                    /**
                     * The private constructor to keep the class from being instantiated
                     */
                    balancer_server() {
                    }

                };

            }
        }
    }
}

#endif /* BALANCER_SERVER_HPP */

