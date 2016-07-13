/* 
 * File:   translation_server_adapter.hpp
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
 * Created on July 7, 2016, 12:11 PM
 */

#ifndef TRANSLATION_SERVER_ADAPTER_HPP
#define TRANSLATION_SERVER_ADAPTER_HPP

#include <future>

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"

#include "balancer/balancer_parameters.hpp"

#include "client/messaging/trans_job_resp_in.hpp"
#include "client/translation_client.hpp"

using namespace std;

using namespace uva::utils::logging;
using namespace uva::utils::exceptions;

using namespace uva::smt::bpbd::client;
using namespace uva::smt::bpbd::client::messaging;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace balancer {

                /**
                 * This is the translation server adapter class:
                 * Responsibilities:
                 *      Connects to a translation server
                 *      Send supported languages requests
                 *      Receive supported languages responses
                 *      Notifies the server adviser about the supported languages
                 *      Notifies the server adviser about the connection status
                 *      Re-connects to the disconnected server
                 *      Send translation requests
                 *      Receive translation responses
                 */
                class translation_server_adapter {
                public:
                    //Define the function type for the function used to notify about the disconnected server
                    typedef function<void(const translation_server_adapter &) > closed_conn_notifier_type;

                    /**
                     * The basic constructor for the adapter class
                     */
                    translation_server_adapter()
                    : m_params(NULL), m_client(NULL), m_is_enabled(false),
                    m_lock_con(), m_notify_conn_closed_func() {
                    }

                    /**
                     * The basic destructor for the adapter class
                     */
                    virtual ~translation_server_adapter() {
                        //Stop the server if it is not stopped yet
                        disable();

                        //Remove the previous connection client if any
                        remove_connection_client();
                    }

                    /**
                     * Allows to configure the adapter with the translation server parameters
                     * @param params the translation server parameters
                     * @param notify_conn_closed_func the function to notify about the closed server connection
                     */
                    inline void configure(const trans_server_params & params, closed_conn_notifier_type notify_conn_closed_func) {
                        recursive_guard guard(m_lock_con);

                        //Check that the adapter is not enabled!
                        ASSERT_CONDITION_THROW(m_is_enabled,
                                string("Trying to re-configure an enabled adapter for: ") + m_params->m_name);

                        //Store the reference to the parameters
                        m_params = &params;

                        //Store the function needed to notify about the closed connection
                        m_notify_conn_closed_func = notify_conn_closed_func;

                        //Remove the previous connection client if any
                        remove_connection_client();
                    }

                    /**
                     * The main method to enable the translation server adapter
                     */
                    inline void enable() {
                        recursive_guard guard(m_lock_con);
                        LOG_DEBUG << "Enabling the server adapter for: " << m_params->m_name << END_LOG;

                        //Set the flag to true indicating that we are in the process of working
                        m_is_enabled = true;

                        //Create a new translation client
                        m_client = new translation_client(m_params->m_address, m_params->m_port,
                                bind(&translation_server_adapter::set_job_response, this, _1),
                                bind(&translation_server_adapter::notify_conn_closed, this));

                        LOG_DEBUG << "Is '" << m_params->m_name << "' connected: "
                                << to_string(m_client->is_connected()) << END_LOG;

                        //If we are not connected to the server
                        if (!m_client->is_connected()) {
                            //Attempt to connect, if we fail then schedule a re-connect
                            if (!m_client->connect()) {
                                LOG_DEBUG << "Could not connect to: '" << m_params->m_name << "'" << END_LOG;
                            }
                        }

                        LOG_DEBUG << "Finished enabling the server adapter for: " << m_params->m_name << END_LOG;
                    }

                    /**
                     * Allows to disable the translation server adapter
                     */
                    inline void disable() {
                        recursive_guard guard(m_lock_con);

                        LOG_DEBUG << "Disabling the server adapter for " << m_params->m_name << END_LOG;

                        //Set the flag to false indicating that we are in the process of stopping
                        m_is_enabled = false;

                        //Disconnect from the server
                        remove_connection_client();

                        LOG_DEBUG << "Finished disabling the server adapter for " << m_params->m_name << END_LOG;
                    }

                    /**
                     * Allows to check whether the adaptor is enabled or disabled
                     * @return true if the adaptor is enabled, otherwise false
                     */
                    inline bool is_enabled() {
                        recursive_guard guard(m_lock_con);

                        return m_is_enabled;
                    }

                    /**
                     * Allows to check whether the adaptor's client is connected to the server
                     * @return true if the adaptor is connected, otherwise false
                     */
                    inline bool is_disconnected() {
                        recursive_guard guard(m_lock_con);

                        return (m_client == NULL) || (!m_client->is_connected());
                    }

                protected:

                    /**
                     * Allows to process the server job request response
                     * @param trans_job_resp a pointer to the translation job response data, not NULL
                     */
                    void set_job_response(trans_job_resp_in * trans_job_resp) {
                        LOG_DEBUG << "The server '" << m_params->m_name << "' client got a translation job response!" << END_LOG;

                        //ToDo: Implement
                    }

                    /**
                     * This function will be called if the connection is closed during the translation process
                     */
                    void notify_conn_closed() {
                        LOG_DEBUG << "The server '" << m_params->m_name << "' has closed the connection!" << END_LOG;

                        //Notify the translation servers manager
                        m_notify_conn_closed_func(*this);

                        //ToDo: Implement, the currently awaiting responses are to be canceled
                    }

                    /**
                     * Allows to remove and destroy the connection client.
                     * First the connection is closed. If client is not
                     * present then nothing is done.
                     */
                    inline void remove_connection_client() {
                        if (m_client != NULL) {
                            m_client->disconnect();
                            delete m_client;
                            m_client = NULL;
                        }
                    }

                private:
                    //Stores the pointer to the translation server parameters
                    const trans_server_params * m_params;
                    //Stores the pointer to the translation client
                    translation_client * m_client;
                    //Stores the boolean flag indicating whether the adapter is enabled
                    atomic<bool> m_is_enabled;
                    //Stores the synchronization mutex for connection
                    recursive_mutex m_lock_con;
                    //Stores the function needed to notify that the connection was closed
                    closed_conn_notifier_type m_notify_conn_closed_func;
                };

            }
        }
    }
}

#endif /* TRANSLATION_SERVER_ADAPTER_HPP */

