/* 
 * File:   translation_server.hpp
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
 * Created on January 14, 2016, 2:39 PM
 */

#ifndef TRANSLATION_SERVER_HPP
#define TRANSLATION_SERVER_HPP

#include <iostream>
#include <functional>

#define ASIO_STANDALONE
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"
#include "common/messaging/trans_job_response.hpp"
#include "common/messaging/trans_job_request.hpp"
#include "common/messaging/trans_job_code.hpp"
#include "trans_manager.hpp"

using namespace std;
using namespace std::placeholders;

using namespace websocketpp;
using namespace websocketpp::frame;
using namespace websocketpp::lib;
using namespace websocketpp::log;

using namespace uva::utils::logging;
using namespace uva::utils::exceptions;
using namespace uva::smt::bpbd::common::messaging;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {

                /**
                 * This is the translation server class implementing the functionality of
                 * receiving the client connections and doing translation jobs for them.
                 */
                class translation_server {
                public:
                    typedef websocketpp::server<websocketpp::config::asio> server;

                    /**
                     * The basic constructor
                     * @param port the port to listen to
                     * @param num_threads the number of translation threads to run
                     */
                    translation_server(const uint16_t port, const size_t num_threads)
                    : m_manager(num_threads) {
                        //Set up access channels to only log interesting things
                        m_server.clear_access_channels(log::alevel::all);
                        m_server.set_access_channels(log::alevel::app);

                        //Initialize the Asio transport policy
                        m_server.init_asio();

                        //Add the handlers for the connection events
                        m_server.set_open_handler(bind(&translation_server::on_open, this, _1));
                        m_server.set_close_handler(bind(&translation_server::on_close, this, _1));
                        m_server.set_fail_handler(bind(&translation_server::on_fail, this, _1));

                        //Bind the handlers we are using
                        m_server.set_message_handler(bind(&translation_server::on_message, this, _1, _2));

                        //Set the reply sending function to the translation manager
                        m_manager.set_response_sender(bind(&translation_server::send_response, this, _1, _2));

                        //Set the port that the server will listen to
                        m_server.listen(port);
                    }

                    /**
                     * Allows to set the new number of worker threads.
                     * This operation should be safe as the new threads
                     * are just added to the list and the deleted ones
                     * are let to finish their translation task execution. 
                     * @param num_threads the new number of worker threads
                     */
                    void set_num_threads(const size_t num_threads) {
                        m_manager.set_num_threads(num_threads);
                    }

                    /**
                     * Allows to report the runtime information about the server.
                     */
                    void report_run_time_info() {
                        m_manager.report_run_time_info();
                    }

                    /**
                     * Allows to run the server
                     */
                    void run() {
                        m_server.start_accept();
                        m_server.run();
                    }

                    /**
                     * Allows to stop the translation server
                     */
                    void stop() {
                        //NOTE: Somehow stopping to listen to the new connections result in errors
                        //It seems to be an implementation flaw of the library, from what I see in
                        //the code. The work-around now is that @ stopping the errors are disabled.
                        m_server.clear_error_channels(log::elevel::all);

                        LOG_DEBUG << "Removing the on_close handler." << END_LOG;
                        //Remove the on_close handler
                        m_server.set_close_handler(NULL);

                        LOG_DEBUG << "Stop listening to the new connections." << END_LOG;
                        //Stop listening to the (new) connections
                        m_server.stop_listening();

                        LOG_USAGE << "Stopping the session manager." << END_LOG;
                        //Stop the session manager, this should cancel all the unfinished translation tasks
                        m_manager.stop();

                        LOG_USAGE << "Stopping the WEBSOCKET server." << END_LOG;
                        //Stop the server
                        m_server.stop();
                    }

                protected:

                    /**
                     * Allows to send the translation job response to the client associated with the given connection handler.
                     * @param hdl the connection handler to identify the connection
                     * @param response the translation response object to be used
                     */
                    void send_response(connection_hdl hdl, trans_job_response & response) {
                        LOG_DEBUG << "Sending the job response: " << &response << END_LOG;

                        //Get the response string
                        const string reply_str = response.serialize();
                        //Declare the error code
                        lib::error_code ec;

                        //Send/schedule the translation job reply
                        m_server.send(hdl, reply_str, opcode::text, ec);

                        //Locally report sending error
                        if (ec) {
                            LOG_ERROR << "Failed sending error '" << reply_str << "' reply: " << ec.message() << END_LOG;
                        }

                        LOG_DEBUG << "The job response: " << &response << " is sent!" << END_LOG;
                    }

                    /**
                     * Creates a new session object for the new connection/client
                     * @param hdl
                     */
                    void on_open(connection_hdl hdl) {
                        LOG_DEBUG << "Opening connection!" << END_LOG;
                        try {
                            //Create a new session object for the handler
                            m_manager.open_session(hdl);
                        } catch (uva_exception & ex) {
                            //Locally report error
                            string err_msg = ex.get_message();
                            LOG_ERROR << err_msg << END_LOG;
                            m_server.close(hdl, websocketpp::close::status::internal_endpoint_error, err_msg);
                        }
                    }

                    /**
                     * Removes the session object and also stops the processed translation job requests
                     * @param hdl the connection handler
                     */
                    void on_close(connection_hdl hdl) {
                        LOG_DEBUG << "Closing connection!" << END_LOG;

                        //Destroy the session 
                        m_manager.close_session(hdl);
                    }

                    /**
                     * Is called in case of a websocket error, for now does nothing but logs the error
                     * @param hdl the connection handler
                     */
                    void on_fail(connection_hdl hdl) {
                        LOG_DEBUG << "Connection failed!" << END_LOG;
                    }

                    /**
                     * Is called when the message is received by the server
                     * @param hdl the connection handler
                     * @param msg the received message
                     */
                    void on_message(websocketpp::connection_hdl hdl, server::message_ptr msg) {
                        LOG_DEBUG << "Received a message!" << END_LOG;

                        //Create translation job request, will be deleted by the translation job
                        trans_job_request_ptr request_ptr = new trans_job_request();

                        //Declare the job id for the case of needed error reporting
                        job_id_type job_id_val = job_id::UNDEFINED_JOB_ID;
                        try {
                            //De-serialize the job request
                            request_ptr->de_serialize(msg->get_payload());

                            //Store the job id in case of an error
                            job_id_val = request_ptr->get_job_id();

                            //Schedule a translation job
                            m_manager.translate(hdl, request_ptr);
                        } catch (uva_exception & ex) {
                            //Locally report error
                            LOG_ERROR << ex.get_message() << END_LOG;

                            //Create the reply message, with or without job id
                            trans_job_response response(job_id_val, trans_job_code::RESULT_ERROR, ex.get_message());

                            //Send the response
                            send_response(hdl, response);

                            //Delete the request object if the pointer is not NULL
                            if (request_ptr != NULL) {
                                delete request_ptr;
                            }
                        }
                    }

                private:
                    //Stores the server object
                    server m_server;
                    //Stores the session manager object
                    trans_manager m_manager;
                };
            }
        }
    }
}

#endif /* TRANSLATION_SERVER_HPP */

