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

#include "common/messaging/status_code.hpp"
#include "common/messaging/incoming_msg.hpp"

#include "server/trans_manager.hpp"
#include "server/server_parameters.hpp"

#include "server/messaging/supp_lang_req_in.hpp"
#include "server/messaging/trans_job_req_in.hpp"
#include "server/messaging/supp_lang_resp_out.hpp"
#include "server/messaging/trans_job_resp_out.hpp"

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
                        supp_lang_resp_out supp_lang_resp;
                        //Add the supported languages
                        supp_lang_resp.start_supp_lang_obj();
                        supp_lang_resp.start_source_lang_arr(params.m_source_lang);
                        supp_lang_resp.add_target_lang(params.m_target_lang);
                        supp_lang_resp.end_source_lang_arr();
                        supp_lang_resp.end_supp_lang_obj();
                        
                        m_supp_lang_resp = supp_lang_resp.serialize();

                        //Set up access channels to only log interesting things
                        m_server.clear_access_channels(log::alevel::all);
                        m_server.set_access_channels(log::alevel::none);
                        m_server.clear_error_channels(log::alevel::all);
                        m_server.set_error_channels(log::alevel::none);

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
                     * Creates a new session object for the new connection/client
                     * @param hdl
                     */
                    void on_open(connection_hdl hdl) {
                        LOG_DEBUG << "Opening connection!" << END_LOG;
                        try {
                            //Create a new session object for the handler
                            m_manager.open_session(hdl);
                        } catch (std::exception & ex) {
                            //Locally report error
                            string err_msg = ex.what();
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
                     * @param raw_msg the received message
                     */
                    void on_message(websocketpp::connection_hdl hdl, server::message_ptr raw_msg) {
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
                                    translation_job(hdl, jmsg);
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
                    inline void language_request(websocketpp::connection_hdl hdl, const incoming_msg * msg) {
                        //Create the supported languages request message
                        supp_lang_req_in supp_lang_req(msg);

                        //Send the response supported languages response
                        send_response(hdl, m_supp_lang_resp);
                    }

                    /**
                     * This method allows to handle the translation job request
                     * This function must not throw!
                     * @param hdl the connection handler
                     * @param msg a pointer to the JSON object storing the request data, not NULL.
                     */
                    inline void translation_job(websocketpp::connection_hdl hdl, const incoming_msg * msg) {
                        //Create translation job request, will be deleted by the translation job
                        trans_job_req_in trans_req(msg);

                        //Declare the job id for the case of needed error reporting
                        job_id_type job_id_val = job_id::UNDEFINED_JOB_ID;
                        try {
                            //Check the source and target languages
                            check_source_target_languages(trans_req.get_source_lang(), trans_req.get_target_lang());

                            //Store the job id in case of an error
                            job_id_val = trans_req.get_job_id();

                            //Schedule a translation job
                            m_manager.translate(hdl, trans_req);
                        } catch (std::exception & ex) {
                            //Get the error message
                            string error_msg = ex.what();

                            //Locally report error
                            LOG_ERROR << "job " << job_id_val << ": " << error_msg << END_LOG;

                            //Create the reply message, with or without job id
                            trans_job_resp_out response(job_id_val, status_code::RESULT_ERROR, error_msg);

                            //Send the response
                            send_response(hdl, response.serialize());
                        }
                    }

                protected:

                    /**
                     * Allows to check that the source and target languages are proper,
                     * as the ones that this server instance supports.
                     * @param source_lang the source language string, from the job request.
                     * @param target_lang the target language string, from the job request.
                     * @throws uva_exception in case the source or target language do not match.
                     */
                    void check_source_target_languages(string source_lang, string target_lang) {
                        //Check for an exact match
                        if (m_params.m_source_lang != source_lang) {
                            //If no exact match then lowercase
                            (void) to_lower(source_lang);
                            //Check for a match again
                            if (m_params.m_source_lang_lower != source_lang) {
                                //If there is still no match then throw 
                                THROW_EXCEPTION(string("Unsupported source language: '") + source_lang +
                                        string("' the server only supports: '") + m_params.m_source_lang +
                                        string("'"));
                            }
                        }
                        //Check for an exact match
                        if (m_params.m_target_lang != target_lang) {
                            //If no exact match then lowercase
                            (void) to_lower(target_lang);
                            //Check for a match again
                            if (m_params.m_target_lang_lower != target_lang) {
                                //If there is still no match then throw 
                                THROW_EXCEPTION(string("Unsupported target language: '") + target_lang +
                                        string("' the server only supports: '") + m_params.m_target_lang +
                                        string("'"));
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

