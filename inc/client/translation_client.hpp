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
#define	TRANSLATION_CLIENT_HPP

#include <cstdlib>
#include <string>

#define ASIO_STANDALONE

#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/common/thread.hpp>

using namespace std;

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::bind;

namespace uva {
    namespace smt {
        namespace decodig {
            namespace client {

                /**
                 * This class is responsible for sending the translation
                 * job request to the server and receiving the result.
                 */
                class translation_client {
                public:
                    typedef websocketpp::client<websocketpp::config::asio_client> client;
                    typedef websocketpp::lib::lock_guard<websocketpp::lib::mutex> scoped_lock;

                    translation_client(const string & host, const uint16_t port) : m_open(false), m_done(false) {
                        //Initialize the URI to connect to
                        m_uri = string("ws://") + host + string(":") + to_string(port);

                        // Set up access channels to only log interesting things
                        m_client.clear_access_channels(websocketpp::log::alevel::all);
                        m_client.set_access_channels(websocketpp::log::alevel::connect);
                        m_client.set_access_channels(websocketpp::log::alevel::disconnect);
                        m_client.set_access_channels(websocketpp::log::alevel::app);

                        // Initialize the Asio transport policy
                        m_client.init_asio();

                        // Bind the handlers we are using
                        m_client.set_open_handler(bind(&translation_client::on_open, this, _1));
                        m_client.set_close_handler(bind(&translation_client::on_close, this, _1));
                        m_client.set_fail_handler(bind(&translation_client::on_fail, this, _1));
                    }

                    ~translation_client() {
                        if (m_open && !m_done) {
                            //Close the connection to the server
                            m_client.close(m_hdl, websocketpp::close::status::normal, "The needed translations are finished.");
                        }
                    }

                    /**
                     * This method will block until the connection is complete
                     * @param uri the uri to connect to
                     */
                    void connect() {
                        // Create a new connection to the given URI
                        websocketpp::lib::error_code ec;
                        client::connection_ptr con = m_client.get_connection(m_uri, ec);
                        if (ec) {
                            m_client.get_alog().write(websocketpp::log::alevel::app,
                                    "Get Connection (" + m_uri + ") Error: " + ec.message());
                            return;
                        }

                        // Grab a handle for this connection so we can talk to it in a thread
                        // safe manor after the event loop starts.
                        m_hdl = con->get_handle();

                        // Queue the connection. No DNS queries or network connections will be
                        // made until the io_service event loop is run.
                        m_client.connect(con);

                        // Create a thread to run the ASIO io_service event loop
                        websocketpp::lib::thread asio_thread(&client::run, &m_client);

                        // Create a thread to run the telemetry loop
                        websocketpp::lib::thread telemetry_thread(&translation_client::telemetry_loop, this);

                        asio_thread.join();
                        telemetry_thread.join();
                    }

                    /**
                     * The open handler will signal that we are ready to start sending telemetry
                     * @param the connection handler
                     */
                    void on_open(websocketpp::connection_hdl hdl) {
                        m_client.get_alog().write(websocketpp::log::alevel::app,
                                "Connection opened, starting telemetry!");

                        scoped_lock guard(m_lock);
                        m_open = true;
                    }

                    /**
                     * The close handler will signal that we should stop sending telemetry
                     * @param the connection handler
                     */
                    void on_close(websocketpp::connection_hdl hdl) {
                        m_client.get_alog().write(websocketpp::log::alevel::app,
                                "Connection closed, stopping telemetry!");

                        scoped_lock guard(m_lock);
                        m_done = true;
                    }

                    /**
                     * The fail handler will signal that we should stop sending telemetry
                     * @param the connection handler
                     */
                    void on_fail(websocketpp::connection_hdl hdl) {
                        m_client.get_alog().write(websocketpp::log::alevel::app,
                                "Connection failed, stopping translation!");

                        scoped_lock guard(m_lock);
                        m_done = true;
                    }

                    void telemetry_loop() {
                        uint64_t count = 0;
                        std::stringstream val;
                        websocketpp::lib::error_code ec;

                        while (1) {
                            bool wait = false;

                            {
                                scoped_lock guard(m_lock);
                                // If the connection has been closed, stop generating telemetry
                                if (m_done) {
                                    break;
                                }

                                // If the connection hasn't been opened yet wait a bit and retry
                                if (!m_open) {
                                    wait = true;
                                }
                            }

                            if (wait) {
                                sleep(1);
                                continue;
                            }

                            val.str("");
                            val << "count is " << count++;

                            m_client.get_alog().write(websocketpp::log::alevel::app, val.str());
                            m_client.send(m_hdl, val.str(), websocketpp::frame::opcode::text, ec);

                            // The most likely error that we will get is that the connection is
                            // not in the right state. Usually this means we tried to send a
                            // message to a connection that was closed or in the process of
                            // closing. While many errors here can be easily recovered from,
                            // in this simple example, we'll stop the telemetry loop.
                            if (ec) {
                                m_client.get_alog().write(websocketpp::log::alevel::app,
                                        "Send Error: " + ec.message());
                                break;
                            }

                            sleep(1);
                        }
                    }
                private:
                    client m_client;
                    websocketpp::connection_hdl m_hdl;
                    websocketpp::lib::mutex m_lock;
                    bool m_open;
                    bool m_done;
                    string m_uri;
                };
            }
        }
    }
}

#endif	/* TRANSLATION_CLIENT_HPP */

