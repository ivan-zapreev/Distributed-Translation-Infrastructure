/* 
 * File:   translator_adapter.hpp
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

#ifndef TRANSLATOR_ADAPTER_HPP
#define TRANSLATOR_ADAPTER_HPP

#include <future>

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"
#include "common/utils/id_manager.hpp"
#include "common/messaging/incoming_msg.hpp"

#include "client/messaging/trans_job_resp_in.hpp"
#include "client/messaging/supp_lang_resp_in.hpp"
#include "client/messaging/supp_lang_req_out.hpp"
#include "client/translation_client.hpp"

#include "balancer/balancer_consts.hpp"
#include "balancer/balancer_parameters.hpp"

using namespace std;

using namespace uva::utils;
using namespace uva::utils::logging;
using namespace uva::utils::exceptions;

using namespace uva::smt::bpbd::common::messaging;
using namespace uva::smt::bpbd::client;
using namespace uva::smt::bpbd::client::messaging;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace balancer {
                //Define the functional to set the translation response
                typedef function<void(trans_job_resp_in *) > trans_resp_notifier;
                //Define the functional to notify about the translator adapter disconnect
                typedef function<void(const trans_server_uid & uid) > adapter_disc_notifier;

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
                class translator_adapter {
                public:
                    //Define ready connection notifier function
                    typedef function<void(translator_adapter *, supp_lang_resp_in *) > ready_conn_notifier_type;
                    //Define the function type for the function used to notify about the disconnected server
                    typedef function<void(translator_adapter *) > closed_conn_notifier_type;

                    /**
                     * The basic constructor for the adapter class
                     */
                    translator_adapter()
                    : m_uid(m_ids_manager.get_next_id()), m_params(NULL),
                    m_trans_resp_func(NULL), m_adapter_disc_func(NULL),
                    m_client(NULL), m_is_enabled(false), m_is_connected(false),
                    m_is_connecting(false), m_lock_con(), m_notify_conn_closed_func() {
                    }

                    /**
                     * The basic destructor for the adapter class
                     */
                    virtual ~translator_adapter() {
                        //Stop the server if it is not stopped yet
                        disable();

                        //Remove the previous connection client if any
                        remove_connection_client();
                    }

                    /**
                     * Allows to configure the adapter with the translation server parameters
                     * @param params the translation server parameters
                     * @param trans_resp_func the functional to notify about the translation response
                     * @param adapter_disc_func the functional to notify about the adapter disconnect
                     * @param notify_conn_ready_func the functional to notify about the adapter ready
                     * @param notify_conn_closed_func the function to notify about the closed server connection
                     */
                    inline void configure(
                            const trans_server_params & params,
                            trans_resp_notifier trans_resp_func,
                            adapter_disc_notifier adapter_disc_func,
                            ready_conn_notifier_type notify_conn_ready_func,
                            closed_conn_notifier_type notify_conn_closed_func) {
                        recursive_guard guard(m_lock_con);

                        //Check that the adapter is not enabled!
                        ASSERT_CONDITION_THROW(m_is_enabled,
                                string("Trying to re-configure an enabled adapter for: ") + m_params->m_name);

                        //Store the reference to the parameters
                        m_params = &params;

                        //Store the functional to notify about the translation response
                        m_trans_resp_func = trans_resp_func;
                        //Store the functional to notify about the adapter disconnect
                        m_adapter_disc_func = adapter_disc_func;

                        //Store the functions needed to notify about the connection
                        m_notify_conn_ready_func = notify_conn_ready_func;
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

                        //Create a new client and connect to the server
                        create_connection_client_connect();

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
                     * Allows to reconnect the adapter if it is enabled and disconnected.
                     */
                    inline void reconnect() {
                        recursive_guard guard(m_lock_con);

                        LOG_DEBUG << "Re-connecting the server adapter for: " << m_params->m_name << END_LOG;

                        //Check if the adapter needs re-connection
                        if (this->is_enabled() && this->is_disconnected()) {
                            //Disconnect from the server and remove the client
                            remove_connection_client();
                            //Create a new connection client;
                            create_connection_client_connect();
                        }

                        LOG_DEBUG << "Finished re-connecting the server adapter for " << m_params->m_name << END_LOG;
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

                    /**
                     * Allows to check whether the adaptor's client is connecting to the server
                     * @return true if the adaptor is connecting, otherwise false
                     */
                    inline bool is_connecting() {
                        recursive_guard guard(m_lock_con);

                        return m_is_connecting;
                    }

                    /**
                     * Reports the run-time information
                     */
                    inline void report_run_time_info() {
                        recursive_guard guard(m_lock_con);

                        //Get the connection status
                        string status = "DISABLED";
                        if (this->is_enabled()) {
                            if (this->is_connecting()) {
                                status = "CONNECTING";
                            } else {
                                if (this->is_disconnected()) {
                                    status = "AWAITING RE-CONNECT";
                                } else {
                                    status = "CONNECTED";
                                }
                            }
                        }

                        LOG_USAGE << "\t" << m_params->m_name << "(uid:" << to_string(m_uid) << ") -> " << status << END_LOG;
                    }

                    /**
                     * Allows to get the name of the server represented by this adapter
                     * @return the name of the server
                     */
                    inline const string & get_name() const {
                        return m_params->m_name;
                    }

                    /**
                     * Allows to get the unique identifier of the translation server adapter
                     * @return the name of the server
                     */
                    inline const trans_server_uid & get_uid() const {
                        return m_uid;
                    }

                    /**
                     * Allows to get the load weight of the adapter
                     * @return the load weight of the adapter
                     */
                    inline uint32_t get_weight() const {
                        return m_params->m_load_weight;
                    }

                protected:

                    /**
                     * Allows to process the server message
                     * @param json_msg a pointer to the json incoming message, not NULL
                     */
                    inline void set_server_message(incoming_msg * json_msg) {
                        recursive_guard guard(m_lock_con);

                        LOG_DEBUG << "The server '" << m_params->m_name << "' client got "
                                << "a server message, type: " << json_msg->get_msg_type() << END_LOG;

                        //Check on the message type
                        switch (json_msg->get_msg_type()) {
                            case msg_type::MESSAGE_TRANS_JOB_RESP:
                            {
                                //Create a new job response message
                                trans_job_resp_in * job_resp_msg = new trans_job_resp_in(json_msg);
                                try {
                                    //Set the newly received job response
                                    m_trans_resp_func(job_resp_msg);
                                } catch (std::exception & ex) {
                                    LOG_ERROR << ex.what() << END_LOG;
                                    //Delete the message as it was not set
                                    delete job_resp_msg;
                                }
                            }
                                break;
                            case msg_type::MESSAGE_SUPP_LANG_RESP:
                            {
                                //Create a new languages response message
                                supp_lang_resp_in * lang_resp_msg = new supp_lang_resp_in(json_msg);
                                try {
                                    //Set the newly received job response
                                    m_notify_conn_ready_func(this, lang_resp_msg);
                                } catch (std::exception & ex) {
                                    LOG_ERROR << ex.what() << END_LOG;
                                    //Delete the message as it was not set
                                    delete lang_resp_msg;
                                }
                            }
                                break;
                            default:
                                THROW_EXCEPTION(string("Unexpected incoming message type: ") +
                                        to_string(json_msg->get_msg_type()));
                        }
                    }

                    /**
                     * This function will be called if the connection is opened
                     */
                    void notify_conn_opened() {
                        recursive_guard guard(m_lock_con);

                        LOG_DEBUG << "The server '" << m_params->m_name << "' connection is open!" << END_LOG;

                        //Request the supported languages, if the server is enabled
                        if (m_is_enabled && m_client->is_connected()) {
                            supp_lang_req_out req;
                            m_client->send(&req);
                        }

                        //Once everything is processed the connection is truly open
                        m_is_connected = true;

                        //Set the flag indicating that we stopped connecting
                        m_is_connecting = false;
                    }

                    /**
                     * This function will be called if the connection is closed during the translation process
                     */
                    void notify_conn_closed() {
                        recursive_guard guard(m_lock_con);

                        LOG_DEBUG << "The server '" << m_params->m_name << "' has closed the connection!" << END_LOG;

                        //Check if the connection was open, as it can be the first time
                        //we tried to connect or a failed re-connection attempt.
                        if (m_is_connected) {
                            //Notify the translation servers manager that the server adapter is disconnected
                            m_notify_conn_closed_func(this);

                            //Notify the translation manager that there was a translation server connection lost
                            m_adapter_disc_func(m_uid);

                            //Once everything is processed the connection is truly closed
                            m_is_connected = false;
                        }

                        //Set the flag indicating that we stopped connecting
                        m_is_connecting = false;
                    }

                    /**
                     * Allows to remove and destroy the connection client.
                     * First the connection is closed. If client is not
                     * present then nothing is done.
                     */
                    inline void remove_connection_client() {
                        recursive_guard guard(m_lock_con);

                        if (m_client != NULL) {
                            m_client->disconnect();
                            delete m_client;
                            m_client = NULL;
                        }
                    }

                    /**
                     * Allows to create a new connection client if there is none.
                     * The method is not synchronized. The precondition is that
                     * the adapter is configured. 
                     */
                    inline void create_connection_client() {
                        m_client = new translation_client(m_params->m_address, m_params->m_port,
                                bind(&translator_adapter::set_server_message, this, _1),
                                bind(&translator_adapter::notify_conn_closed, this),
                                bind(&translator_adapter::notify_conn_opened, this));
                    }

                    /**
                     * Allows to create a new connection client if there is none and request a connect.
                     * The connection will be done in a non-blocking way. The method is not synchronized.
                     * The precondition is that the adapter is configured. 
                     */
                    inline void create_connection_client_connect() {
                        //Create a new translation client
                        create_connection_client();

                        //Set the flag indicating that we started connecting
                        m_is_connecting = true;

                        //Attempt to connect
                        m_client->connect_nb();
                    }

                private:
                    //Stores the adapter uid manager for issuing the adapter ids
                    static id_manager<trans_server_uid> m_ids_manager;
                    //Stores the unique identifier for the given translation adapter
                    const trans_server_uid m_uid;
                    //Stores the pointer to the translation server parameters
                    const trans_server_params * m_params;
                    //Stores the functional to notify about the translation response
                    trans_resp_notifier m_trans_resp_func;
                    //Stores the functional to notify about the adapter disconnect
                    adapter_disc_notifier m_adapter_disc_func;

                    //Stores the pointer to the translation client
                    translation_client * m_client;
                    //Stores the boolean flag indicating whether the adapter is enabled
                    a_bool_flag m_is_enabled;
                    //Stores the boolean flag indicating whether the adapter is connected 
                    a_bool_flag m_is_connected;
                    //Stores the boolean flag indicating whether the adapter is connecting 
                    a_bool_flag m_is_connecting;
                    //Stores the synchronization mutex for connection
                    recursive_mutex m_lock_con;
                    //Stores the function needed to notify about ready connection
                    ready_conn_notifier_type m_notify_conn_ready_func;
                    //Stores the function needed to notify about closed connection
                    closed_conn_notifier_type m_notify_conn_closed_func;
                };

            }
        }
    }
}

#endif /* TRANSLATION_SERVER_ADAPTER_HPP */

