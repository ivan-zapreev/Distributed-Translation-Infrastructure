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

#define ASIO_STANDALONE
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include "common/utils/Exceptions.hpp"
#include "common/utils/logging/Logger.hpp"
#include "common/messaging/trans_job_reply.hpp"
#include "common/messaging/trans_job_request.hpp"
#include "session_object.hpp"
#include "session_manager.hpp"

using namespace std;
using namespace uva::utils::logging;
using namespace uva::utils::exceptions;
using namespace uva::smt::decoding::common::messaging;

using websocketpp::connection_hdl;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;
using websocketpp::log::alevel;
using websocketpp::frame::opcode::value;
using websocketpp::frame::opcode::text;
using websocketpp::lib::error_code;

namespace uva {
    namespace smt {
        namespace decoding {
            namespace server {

                /**
                 * This is the translation server class implementing the functionality of
                 * receiving the client connections and doing translation jobs for them.
                 */
                class translation_server {
                public:
                    typedef websocketpp::server<websocketpp::config::asio> server;

                    translation_server(const uint16_t port) {
                        // Set up access channels to only log interesting things
                        m_server.clear_access_channels(alevel::all);
                        m_server.set_access_channels(alevel::app);

                        // Initialize the Asio transport policy
                        m_server.init_asio();

                        //ToDo: Add the handlers for the connection events
                        m_server.set_open_handler(bind(&translation_server::on_open, this, _1));
                        m_server.set_close_handler(bind(&translation_server::on_close, this, _1));
                        m_server.set_fail_handler(bind(&translation_server::on_fail, this, _1));

                        // Bind the handlers we are using
                        m_server.set_message_handler(bind(&translation_server::on_message, this, _1, _2));

                        // Set the port that the server will listen to
                        m_server.listen(port);
                    }

                    /**
                     * Creates a new session object for the new connection/client
                     * @param hdl
                     */
                    void on_open(connection_hdl hdl) {
                        //Create a new session object for the handler
                        string err_msg;
                        session_object_ptr session_ptr = m_manager.create_session(hdl, err_msg);
                        if (session_ptr == NULL) {
                            m_server.close(hdl, websocketpp::close::status::internal_endpoint_error, err_msg);
                        }
                    }

                    /**
                     * Removes the session object and also stops the processed translation job requests
                     * @param the connection handler
                     */
                    void on_close(connection_hdl hdl) {
                        //Destroy the session 
                        m_manager.destroy_session(hdl);
                    }

                    /**
                     * ToDo: Figure out if this handles is needed, for now just do logging
                     * @param the connection handler
                     */
                    void on_fail(connection_hdl hdl) {
                        m_server.get_alog().write(alevel::app, "Connection failed!");
                    }

                    void run() {
                        m_server.start_accept();
                        m_server.run();
                    }

                    void on_message(websocketpp::connection_hdl hdl, server::message_ptr msg) {
                        //Declare the translation job request pointer
                        trans_job_request_ptr request_ptr = NULL;
                        try {
                            //Extract the translation job request
                            request_ptr = new trans_job_request(msg->get_payload());

                            //Schedule a translation job
                            session_object_ptr session_ptr = m_manager.get_session(hdl);
                            session_ptr->add_job_request(request_ptr);
                        } catch (Exception & ex) {
                            //Locally report error
                            LOG_ERROR << ex.get_message() << END_LOG;

                            //Create the reply message, with or without job id
                            const job_id_type job_id = (request_ptr == NULL) ? trans_job_request::UNDEFINED_JOB_ID : request_ptr->get_job_id();
                            trans_job_reply reply(job_id, job_result_code::RESULT_ERROR, ex.get_message());
                            const string reply_str = reply.serialize();
                            
                            //Declare the error code
                            error_code ec;
                            //Send/schedule the translation job reply
                            m_server.send(hdl, reply_str, text, ec);
                            //Locally report sending error
                            if (ec) {
                                LOG_ERROR << "Failed sending error '" << reply_str << "' reply: " << ec.message() << END_LOG;
                            }
                        }
                    }

                private:
                    //Stores the server object
                    server m_server;
                    //Stores the session manager object
                    session_manager m_manager;
                };
            }
        }
    }
}

#endif /* TRANSLATION_SERVER_HPP */

