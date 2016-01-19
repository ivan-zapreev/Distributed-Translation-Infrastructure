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
#define	TRANSLATION_SERVER_HPP

#include <iostream>

#define ASIO_STANDALONE

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include "common/utils/Exceptions.hpp"
#include "common/utils/logging/Logger.hpp"
#include "common/messaging/translation_job_reply.hpp"
#include "common/messaging/translation_job_request.hpp"

using namespace std;
using namespace uva::utils::logging;
using namespace uva::utils::exceptions;
using namespace uva::smt::decoding::common::messaging;

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

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
                        m_server.clear_access_channels(websocketpp::log::alevel::all);
                        m_server.set_access_channels(websocketpp::log::alevel::app);

                        // Initialize the Asio transport policy
                        m_server.init_asio();

                        //ToDo: Add the handlers for the connection events
                        //m_server.set_open_handler(bind(&translation_server::on_open, this, _1));
                        //m_server.set_close_handler(bind(&translation_server::on_close, this, _1));
                        //m_server.set_fail_handler(bind(&translation_server::on_fail, this, _1));
                        
                        // Bind the handlers we are using
                        m_server.set_message_handler(bind(&translation_server::on_message, this, _1, _2));

                        // Set the port that the server will listen to
                        m_server.listen(port);
                    }

                    void run() {
                        m_server.start_accept();
                        m_server.run();
                    }

                    void on_message(websocketpp::connection_hdl hdl, server::message_ptr msg) {
                        //Declare the error code
                        websocketpp::lib::error_code ec;
                        
                        //Extract the translation job request
                        translation_job_request request(msg->get_payload());
                        
                        //ToDo: schedule a delayed job reply sending in a separate thread
                        //ToDo: Make sure that things are synchronized for multiple job requests
                        //ToDo: Make sure that if the connection to the client is lost, then we cancel the translation job.
                        
                        //Send/schedule the translation job reply
                        m_server.send(m_hdl, "Got it!", websocketpp::frame::opcode::text, ec);
                        ASSERT_CONDITION_THROW(ec, string("Send Error: ") + ec.message());
                    }

                private:
                    server m_server;
                    websocketpp::connection_hdl m_hdl;
                };
            }
        }
    }
}

#endif	/* TRANSLATION_SERVER_HPP */

