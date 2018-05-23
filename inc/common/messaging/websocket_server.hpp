/* 
 * File:   websocket_server.hpp
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
 * Created on July 15, 2016, 10:29 AM
 */

#ifndef WEBSOCKET_SERVER_HPP
#define WEBSOCKET_SERVER_HPP

#include <iostream>
#include <functional>

#define ASIO_STANDALONE
#if defined(WITH_TLS) && WITH_TLS
#include <websocketpp/config/asio.hpp>
#else
#include <websocketpp/config/asio_no_tls.hpp>
#endif
#include <websocketpp/server.hpp>

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"

#include "common/messaging/status_code.hpp"
#include "common/messaging/incoming_msg.hpp"

#include "server/messaging/supp_lang_req_in.hpp"
#include "server/messaging/trans_job_req_in.hpp"
#include "server/messaging/supp_lang_resp_out.hpp"
#include "server/messaging/trans_job_resp_out.hpp"

#include "processor/messaging/proc_req_in.hpp"

using namespace std;
using namespace std::placeholders;

using namespace websocketpp;
using namespace websocketpp::frame;
using namespace websocketpp::lib;
using namespace websocketpp::log;

using namespace uva::utils::logging;
using namespace uva::utils::exceptions;

using namespace uva::smt::bpbd::server::messaging;
using namespace uva::smt::bpbd::processor::messaging;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace common {
                namespace messaging {

                    /**
                     * Represents the base class for the web socket server.
                     * Contains the common needed functionality
                     */
                    class websocket_server {
                    public:
#if defined(WITH_TLS) && WITH_TLS
                        typedef websocketpp::server<websocketpp::config::asio_tsl> server;
#else
                        typedef websocketpp::server<websocketpp::config::asio> server;
#endif
                        /**
                         * The basic constructor
                         * @param server_port the server port to listen to
                         */
                        websocket_server(const uint16_t server_port) {
                            //Set up access channels to only log interesting things
                            m_server.clear_access_channels(log::alevel::all);
                            m_server.set_access_channels(log::alevel::none);
                            m_server.clear_error_channels(log::alevel::all);
                            m_server.set_error_channels(log::alevel::none);

                            //Initialize the Asio transport policy
                            m_server.init_asio();

                            //Add the handlers for the connection events
                            m_server.set_open_handler(bind(&websocket_server::open_session, this, _1));
                            m_server.set_close_handler(bind(&websocket_server::close_session, this, _1));
                            m_server.set_fail_handler(bind(&websocket_server::close_session, this, _1));

                            //Bind the handlers we are using
                            m_server.set_message_handler(bind(&websocket_server::on_message, this, _1, _2));

                            //Set the port that the server will listen to
                            m_server.listen(server_port);
                        }

                        /**
                         * Allows to run the server
                         */
                        void run() {
                            //Do something before the server starts listening.
                            before_start_listening();

                            LOG_DEBUG << "Starting the websockets server." << END_LOG;
                            //Start accepting the messages
                            m_server.start_accept();
                            m_server.run();
                        }

                        /**
                         * Allows to stop the translation server
                         */
                        void stop() {
                            LOG_DEBUG << "Removing the on_close handler." << END_LOG;
                            //Remove the on_close handler
                            m_server.set_close_handler(NULL);

                            LOG_DEBUG << "Stop listening to the new connections." << END_LOG;
                            //Stop listening to the (new) connections
                            m_server.stop_listening();

                            //Send the remaining responses after the server stopped listening.
                            after_stop_listening();

                            LOG_USAGE << "Stopping the WEBSOCKET server " << END_LOG;
                            //Stop the server
                            m_server.stop();

                            //Wait until the server is stopped
                            while (!m_server.stopped()) {
                                //Sleep for a second
                                sleep(1);
                            }
                            LOG_INFO << "The WEBSOCKET server is stopped!" << END_LOG;
                        }

                    protected:

                        /**
                         * This method is to be called when the server is being started.
                         * It will be called before the server starts listening to the
                         * incoming messages.
                         */
                        virtual void before_start_listening() {
                            //An empty implementation so that it is not compulsory to implement it
                        }

                        /**
                         * This method is to be called when the server is being stopped.
                         * It will be called after the server stopped listening to the
                         * incoming messages but it is still able to send the remaining
                         * reply messages.
                         */
                        virtual void after_stop_listening() {
                            //An empty implementation so that it is not compulsory to implement it
                        }

                        /**
                         * Allows to send the response to the client associated with the given connection handler.
                         * @param hdl the connection handler to identify the connection
                         * @param reply_str the response string
                         */
                        inline void send_response(connection_hdl hdl, const string reply_str) {
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
                         * Allows to create and register a new session object, synchronized.
                         * If for some reason a new session can not be opened, an exception is thrown.
                         * @param hdl [in] the connection handler to identify the session object.
                         */
                        virtual void open_session(websocketpp::connection_hdl hdl) = 0;

                        /**
                         * Allows to erase the session object from the map and return the stored object, synchronized.
                         * Returns NULL if there was no session object associated with the given handler.
                         * @param hdl the connection handler to identify the session object.
                         * @return the session object to be removed, is to be deallocated by the caller.
                         */
                        virtual void close_session(websocketpp::connection_hdl hdl) = 0;

                        /**
                         * Is called when the message is received by the server
                         * @param hdl the connection handler
                         * @param raw_msg the received message
                         */
                        virtual void on_message(websocketpp::connection_hdl hdl, server::message_ptr raw_msg) {
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
                                        translation_request(hdl, new trans_job_req_in(jmsg));
                                        break;
                                    case msg_type::MESSAGE_SUPP_LANG_REQ:
                                        language_request(hdl, new supp_lang_req_in(jmsg));
                                        break;
                                    case msg_type::MESSAGE_PRE_PROC_JOB_REQ:
                                        pre_process_request(hdl, new proc_req_in(jmsg));
                                        break;
                                    case msg_type::MESSAGE_POST_PROC_JOB_REQ:
                                        post_process_request(hdl, new proc_req_in(jmsg));
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
                         * Allows to process a new supported languages request. It is the
                         * responsibility of the function implementation to destroy
                         * the message object. This method is to be overridden. The
                         * default implementation just throws an unsupported exception.
                         * @param hdl the session handler
                         * @param msg the received message
                         */
                        virtual void language_request(websocketpp::connection_hdl hdl, supp_lang_req_in * msg) {
                            //Destroy the message
                            delete msg;
                            //Throw a non-supported exception
                            THROW_NOT_SUPPORTED();
                        }

                        /**
                         * Allows to process a new translation job request. It is the
                         * responsibility of the function implementation to destroy
                         * the message object. This method is to be overridden. The
                         * default implementation just throws an unsupported exception.
                         * @param hdl the session handler
                         * @param msg the received message
                         */
                        virtual void translation_request(websocketpp::connection_hdl hdl, trans_job_req_in * msg) {
                            //Destroy the message
                            delete msg;
                            //Throw a non-supported exception
                            THROW_NOT_SUPPORTED();
                        }

                        /**
                         * Allows to process a new supported languages request. It is the
                         * responsibility of the function implementation to destroy
                         * the message object. This method is to be overridden. The
                         * default implementation just throws an unsupported exception.
                         * @param hdl the session handler
                         * @param msg the received message
                         */
                        virtual void pre_process_request(websocketpp::connection_hdl hdl, proc_req_in * msg) {
                            //Destroy the message
                            delete msg;
                            //Throw a non-supported exception
                            THROW_NOT_SUPPORTED();
                        }


                        /**
                         * Allows to process a new supported languages request. It is the
                         * responsibility of the function implementation to destroy
                         * the message object. This method is to be overridden. The
                         * default implementation just throws an unsupported exception.
                         * @param hdl the session handler
                         * @param msg the received message
                         */
                        virtual void post_process_request(websocketpp::connection_hdl hdl, proc_req_in * msg) {
                            //Destroy the message
                            delete msg;
                            //Throw a non-supported exception
                            THROW_NOT_SUPPORTED();
                        }

                    private:
                        //Stores the server object
                        server m_server;
                    };
                }
            }
        }
    }
}
#endif /* WEBSOCKET_SERVER_HPP */

