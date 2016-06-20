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
#include "common/messaging/supp_lang_request.hpp"
#include "common/messaging/supp_lang_response.hpp"
#include "common/messaging/trans_job_code.hpp"

#include "trans_manager.hpp"
#include "server_parameters.hpp"

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
                     * @param params the server parameters
                     */
                    translation_server(const server_parameters &params)
                    : m_manager(params.m_num_threads), m_params(params) {
                        //Initialize the supported languages and store the response for future use
                        supp_lang_response supp_lang_resp;
                        supp_lang_resp.add_supp_lang(params.m_source_lang, params.m_target_lang);
                        m_supp_lang_resp = supp_lang_resp.serialize();

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
                        m_server.listen(params.m_server_port);
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
                     * Allows to send the response to the client associated with the given connection handler.
                     * @param hdl the connection handler to identify the connection
                     * @param reply_str the response string
                     */
                    inline void send_str_response(connection_hdl hdl, const string reply_str) {
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
                     * Allows to send the translation job response to the client associated with the given connection handler.
                     * @param hdl the connection handler to identify the connection
                     * @param response the translation response object to be used
                     */
                    void send_response(connection_hdl hdl, trans_job_response & response) {
                        LOG_DEBUG << "Sending the job response: " << &response << END_LOG;

                        //Send the response 
                        send_str_response(hdl, response.serialize());

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
                     * This enumeration stores the request types
                     */
                    enum request_type_enum {
                        UNKNOWN_TYPE_REQUEST = 0,
                        TRANSLATION_JOB_REQUEST = 1,
                        SUPPORTED_LANGUAGES_REQUEST = 2
                    };

                    /**
                     * Is called when the message is received by the server
                     * @param hdl the connection handler
                     * @param msg the received message
                     */
                    void on_message(websocketpp::connection_hdl hdl, server::message_ptr msg) {
                        LOG_DEBUG << "Received a message!" << END_LOG;

                        //Obtain the message payload
                        const string payload = msg->get_payload();

                        //Act depending on the request type
                        switch (detect_msg_type(payload)) {
                            case request_type_enum::TRANSLATION_JOB_REQUEST:
                                translation_job(hdl, payload);
                                break;
                            case request_type_enum::SUPPORTED_LANGUAGES_REQUEST:
                                language_request(hdl, payload);
                                break;
                            default:
                                //Send the error response
                                send_str_response(hdl, "Unknown or unsupported request message!");
                        }
                    }

                    /**
                     * This method allows to detect the kind of request received by the translation server
                     * @param payload the serialized version of the request
                     * @return the detected request type
                     */
                    inline request_type_enum detect_msg_type(const string & payload) {
                        if (trans_job_request::is_request(payload)) {
                            return request_type_enum::TRANSLATION_JOB_REQUEST;
                        } else {
                            if (supp_lang_request::is_request(payload)) {
                                return request_type_enum::SUPPORTED_LANGUAGES_REQUEST;
                            } else {
                                return request_type_enum::UNKNOWN_TYPE_REQUEST;
                            }
                        }
                    }

                    /**
                     * This method allows to handle the supported-languages request
                     * @param hdl the connection handler
                     * @param payload the serialized supported languages request request
                     */
                    inline void language_request(websocketpp::connection_hdl hdl, const string & payload) {
                        //Send the response
                        send_str_response(hdl, m_supp_lang_resp);
                    }

                    /**
                     * This method allows to handle the translation job request
                     * @param hdl the connection handler
                     * @param payload the serialized translation job request
                     */
                    inline void translation_job(websocketpp::connection_hdl hdl, const string & payload) {
                        //Create translation job request, will be deleted by the translation job
                        trans_job_request_ptr request_ptr = new trans_job_request();

                        //Declare the job id for the case of needed error reporting
                        job_id_type job_id_val = job_id::UNDEFINED_JOB_ID;
                        try {
                            //De-serialize the job request
                            request_ptr->de_serialize(payload);

                            //Check that the source/target language pairs are proper
                            ASSERT_CONDITION_THROW(((m_params.m_source_lang != request_ptr->get_source_lang()) ||
                                    (m_params.m_target_lang != request_ptr->get_target_lang())),
                                    string("Wrong source-target language pair: ") + request_ptr->get_source_lang() +
                                    string("->") + request_ptr->get_target_lang() + string(", the server only supports: ") +
                                    m_params.m_source_lang + string("->") + m_params.m_target_lang);

                            //Store the job id in case of an error
                            job_id_val = request_ptr->get_job_id();

                            //Schedule a translation job
                            m_manager.translate(hdl, request_ptr);
                        } catch (uva_exception & ex) {
                            //Locally report error
                            LOG_ERROR << "job " << job_id_val << ": " << ex.get_message() << END_LOG;

                            //Create the reply message, with or without job id
                            trans_job_response response(job_id_val, trans_job_code::RESULT_ERROR, ex.get_message(), payload);

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
                    //Stores the reference to the server options
                    const server_parameters &m_params;
                    //Stores the serialized supported languages response string
                    string m_supp_lang_resp;
                };
            }
        }
    }
}

#endif /* TRANSLATION_SERVER_HPP */

