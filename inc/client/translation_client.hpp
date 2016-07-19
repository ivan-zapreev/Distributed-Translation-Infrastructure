/* 
 * File:   translation_client.hpp
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
 * Created on January 14, 2016, 2:24 PM
 */

#ifndef TRANSLATION_CLIENT_HPP
#define TRANSLATION_CLIENT_HPP

#include <cstdlib>
#include <string>
#include <unordered_map>

#define ASIO_STANDALONE
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

#include "common/utils/threads/threads.hpp"
#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"

#include "common/messaging/incoming_msg.hpp"
#include "client/messaging/trans_job_req_out.hpp"
#include "client/messaging/trans_job_resp_in.hpp"

using namespace std;
using namespace std::placeholders;

using namespace uva::utils::logging;
using namespace uva::utils::exceptions;
using namespace uva::utils::threads;

using namespace uva::smt::bpbd::client::messaging;

using websocketpp::log::alevel;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace client {

                /**
                 * This class is responsible for sending the translation
                 * job request to the server and receiving the result.
                 */
                class translation_client {
                public:
                    typedef websocketpp::client<websocketpp::config::asio_client> client;

                    //Define the function type for the function used to set the translation job result
                    typedef function<void(incoming_msg * json_msg) > server_message_setter;

                    //Define the function type for the function used to notify about the connection status
                    typedef function<void() > conn_status_notifier;

                    translation_client(const string & host, const uint16_t port,
                            server_message_setter set_server_msg_func,
                            conn_status_notifier notify_conn_close,
                            conn_status_notifier notify_conn_open)
                    : m_started(false), m_stopped(false), m_opened(false), m_closed(false),
                    m_set_server_msg_func(set_server_msg_func),
                    m_notify_conn_close(notify_conn_close),
                    m_notify_conn_open(notify_conn_open) {
                        //Assert that the notifiers and setter are defined
                        ASSERT_SANITY_THROW(!m_set_server_msg_func, "The server message setter is NULL!");

                        //Initialize the URI to connect to
                        m_uri = string("ws://") + host + string(":") + to_string(port);

                        //Set up access channels to only log interesting things
                        m_client.clear_access_channels(alevel::all);
                        m_client.set_access_channels(alevel::none);
                        m_client.clear_error_channels(alevel::all);
                        m_client.set_error_channels(alevel::none);

                        // Initialize the Asio transport policy
                        m_client.init_asio();

                        // Bind the handlers we are using
                        m_client.set_open_handler(bind(&translation_client::on_open, this, _1));
                        m_client.set_close_handler(bind(&translation_client::on_close, this, _1));
                        m_client.set_fail_handler(bind(&translation_client::on_fail, this, _1));
                        m_client.set_message_handler(bind(&translation_client::on_message, this, _1, _2));
                    }

                    /**
                     * The basic destructor that also stops the client
                     */
                    ~translation_client() {
                        //Stope the client
                        disconnect();
                    }

                    /**
                     * This method allows to open the connection in a non-blocking way
                     * @param uri the uri to connect to
                     * @return true if the connection has been established
                     * 
                     */
                    inline void connect_nb() {
                        // Create a new connection to the given URI
                        websocketpp::lib::error_code ec;
                        client::connection_ptr con = m_client.get_connection(m_uri, ec);
                        ASSERT_CONDITION_THROW(ec, string("Get Connection (") + m_uri + string(") Error: ") + ec.message());

                        // Grab a handle for this connection so we can talk to it in a thread
                        // safe manor after the event loop starts.
                        m_hdl = con->get_handle();

                        // Queue the connection. No DNS queries or network connections will be
                        // made until the io_service event loop is run.
                        m_client.connect(con);

                        // Create a thread to run the ASIO io_service event loop
                        m_asio_thread = thread(&client::run, &m_client);

                        //Set the client as started
                        m_started = true;

                    }

                    /**
                     * This method will block until the connection is complete
                     * @param uri the uri to connect to
                     * @return true if the connection has been established
                     * 
                     */
                    inline bool connect() {
                        //Request a non-clocking connection open
                        connect_nb();

                        //Wait until the connection is actually open
                        return wait_connect();
                    }

                    /**
                     * Allows to close the connection and stop the io service thread
                     */
                    inline void disconnect() {
                        LOG_DEBUG << "Stopping the client: m_open = " << to_string(m_opened) << ", m_done = " << to_string(m_closed) << END_LOG;

                        if (m_opened && !m_closed) {
                            LOG_INFO << "Closing the server connection..." << END_LOG;

                            //Close the connection to the server
                            m_client.close(m_hdl, websocketpp::close::status::normal, "The needed translations are finished.");

                            //Set the done flag to true as we  are now done
                            m_closed = true;
                        }

                        //Check if the client is stopped
                        if (m_started && !m_stopped) {

                            LOG_DEBUG << "Stopping the IO service thread..." << END_LOG;

                            //Stop the io service thread
                            m_client.stop();

                            //Wait until the service is not stopped
                            while (!m_client.stopped());

                            //Wait for the thread to exit.
                            m_asio_thread.join();

                            //Set the stopped flag to true
                            m_stopped = true;
                        }
                    }

                    /**
                     * Attempts to send an outgoing message to the server
                     * @param message an outgoing message string
                     */
                    inline void send(const string & msg_str) {
                        //Declare the error code
                        websocketpp::lib::error_code ec;

                        LOG_DEBUG << "Serialized translation request: \n" << msg_str << END_LOG;

                        //Try to send the translation job request
                        m_client.send(m_hdl, msg_str, websocketpp::frame::opcode::text, ec);

                        // The most likely error that we will get is that the connection is
                        // not in the right state. Usually this means we tried to send a
                        // message to a connection that was closed or in the process of
                        // closing.
                        ASSERT_CONDITION_THROW(ec, string("Send Error: ") + ec.message());
                    }

                    /**
                     * Attempts to send an outgoing message to the server
                     * @param message an outgoing message
                     */
                    inline void send(outgoing_msg * message) {
                        //Serialize the message into string and send
                        send(message->serialize());
                    }

                    /**
                     * Allows to get the connection URI
                     * @return the connection URI
                     */
                    inline const string get_uri() const {
                        return m_uri;
                    }

                    /**
                     * Allows to check whether the client is connected to the server
                     * @return 
                     */
                    inline bool is_connected() const {
                        return m_opened && !m_closed;
                    }

                protected:

                    /**
                     * This method is used to receive the job translation messages
                     * @param hdl the connection handler
                     * @param msg the message
                     */
                    inline void on_message(websocketpp::connection_hdl hdl, client::message_ptr msg) {
                        //Create a new incoming message
                        incoming_msg * json_msg = new incoming_msg();

                        //Try parsing the incoming message
                        try {
                            //De-serialize the message from string
                            json_msg->de_serialize(msg->get_payload());

                            //Set the message to the client
                            m_set_server_msg_func(json_msg);
                        } catch (std::exception & ex) {
                            LOG_ERROR << ex.what() << END_LOG;
                            //Delete the message as it was not set
                            delete json_msg;
                        }
                    }

                    /**
                     * The open handler will signal that we are ready to start sending translation job(s)
                     * @param the connection handler
                     */
                    inline void on_open(websocketpp::connection_hdl hdl) {
                        LOG_DEBUG << "Connection opened!" << END_LOG;

                        //Do not lock the notification, as that might be blocking as well
                        {
                            scoped_guard guard(m_lock_con);
                            m_opened = true;
                        }

                        //Notify the client that the connection is opened, if the notifier is present!
                        if (m_notify_conn_open) {
                            m_notify_conn_open();
                        }
                    }

                    /**
                     * This function andles the closed connection
                     */
                    inline void handle_closed_connection() {
                        //Do not lock the notification, as that might be blocking as well
                        {
                            scoped_guard guard(m_lock_con);
                            m_closed = true;
                            m_opened = false;
                        }

                        //Notify the client that the connection is closed, if the notifier is present!
                        if (m_notify_conn_close) {
                            m_notify_conn_close();
                        }
                    }

                    /**
                     * The close handler will signal that we should stop sending translation job(s)
                     * @param the connection handler
                     */
                    inline void on_close(websocketpp::connection_hdl hdl) {
                        LOG_DEBUG << "Connection closed!" << END_LOG;

                        //Handle the closed connection
                        handle_closed_connection();
                    }

                    /**
                     * The fail handler will signal that we should stop sending translation job(s)
                     * @param the connection handler
                     */
                    inline void on_fail(websocketpp::connection_hdl hdl) {
                        LOG_DEBUG << "Connection failed!" << END_LOG;

                        //Handle the closed connection
                        handle_closed_connection();
                    }

                    /**
                     * Allows to wait until the connection to the server is established.
                     * @return true if the connection is successfully established
                     */
                    inline bool wait_connect() {
                        //Declare the variable to store the local connection status
                        bool is_connecting = false;

                        LOG_DEBUG << "Connection m_opened: " << to_string(m_opened)
                                << ", m_closed: " << to_string(m_closed) << END_LOG;

                        //Wait until the connection is established
                        while (true) {
                            //Check the connection status
                            {
                                scoped_guard guard(m_lock_con);
                                is_connecting = !m_opened && !m_closed;
                            }

                            //If we we are still connecting then sleep, otherwise move on
                            if (is_connecting) {
                                LOG_DEBUG2 << "Going to sleep, m_open = " << to_string(m_opened)
                                        << ", m_done = " << to_string(m_closed) << END_LOG;
                                sleep(1);
                                LOG_DEBUG2 << "Done sleeping!" << END_LOG
                            } else {
                                break;
                            }
                        }

                        //If the connection is open and is not done then we are nicely connected
                        const bool result = m_opened && !m_closed;
                        LOG_DEBUG << "Is connection open: " << to_string(result) << " (m_open = "
                                << to_string(m_opened) << ", m_done = " << to_string(m_closed) << ")" << END_LOG;

                        return result;
                    }

                private:
                    //Stores the client
                    client m_client;
                    //Stores the io thread
                    thread m_asio_thread;
                    //Stores the connection handler
                    websocketpp::connection_hdl m_hdl;

                    //Stores the synchronization mutex for connection
                    mutex m_lock_con;

                    //Stores the started and stopped flags for the client
                    a_bool_flag m_started;
                    a_bool_flag m_stopped;

                    //Stores the open and done flags for connection
                    a_bool_flag m_opened;
                    a_bool_flag m_closed;

                    //Stores the server message setting function
                    server_message_setter m_set_server_msg_func;
                    //Stores the connection close notifier
                    conn_status_notifier m_notify_conn_close;
                    //Stores the connection open notifier
                    conn_status_notifier m_notify_conn_open;

                    //Stores the server URI
                    string m_uri;

                    //Stores the mapping from the translation job request id to
                    //the resulting translation job result, if already received.
                    //The translation jobs without a reply are mapped to NULL
                    unordered_map<job_id_type, trans_job_resp_in *> m_jobs;
                };
            }
        }
    }
}

#endif /* TRANSLATION_CLIENT_HPP */

