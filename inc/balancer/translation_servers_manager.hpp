/* 
 * File:   translation_servers_manager.hpp
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
 * Created on July 7, 2016, 12:09 PM
 */

#ifndef TRANSLATION_SERVERS_MANAGER_HPP
#define TRANSLATION_SERVERS_MANAGER_HPP

#include <map>

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"
#include "common/utils/threads/threads.hpp"

#include "balancer/balancer_parameters.hpp"
#include "balancer/translation_server_adapter.hpp"

using namespace std;

using namespace uva::utils::logging;
using namespace uva::utils::exceptions;
using namespace uva::utils::threads;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace balancer {

                /**
                 * This is the translation servers manager class:
                 * Responsibilities:
                 *      Keeps track of the online translation servers
                 *      Keeps track of languages supported by the servers
                 *      Keeps track of the known load on the servers
                 *      Advises translation server for a translation request
                 */
                class translation_servers_manager {
                public:

                    /**
                     * Allows to configure the balancer server
                     * @param params the parameters from which the server will be configured
                     */
                    static inline void configure(const balancer_parameters & params) {
                        LOG_INFO3 << "Configuring the translation servers' manager" << END_LOG; 
                        
                        //Store the pointer to the parameters
                        m_params = &params;
                        //Iterate through the list of translation server
                        //configs and create an adapter for each of them
                        for (auto iter = params.trans_servers.begin(); iter != params.trans_servers.end(); ++iter) {
                            m_server_adaptors[iter->first].configure(iter->second, translation_servers_manager::notify_disconnected);
                        }
                        
                        LOG_INFO2 << "The translation servers are configured" << END_LOG; 
                    }

                    /**
                     * The main method to enable the translation servers manager
                     */
                    static inline void enable() {
                        LOG_INFO2 << "Enabling the translation servers' manager" << END_LOG; 
                        
                        //First begin the reconnection thread
                        start_re_connection_thread();

                        //Enable the clients
                        for (auto iter = m_server_adaptors.begin(); iter != m_server_adaptors.end(); ++iter) {
                            iter->second.enable();
                        }
                        
                        LOG_INFO1 << "The translation servers are enabled" << END_LOG; 
                    }

                    /**
                     * Allows to disable the translation servers manager
                     */
                    static inline void disable() {
                        LOG_INFO2 << "Disabling the translation servers' manager" << END_LOG; 
                        
                        //First remove the reconnection thread
                        finish_re_connection_thread();

                        //Disable the clients
                        for (auto iter = m_server_adaptors.begin(); iter != m_server_adaptors.end(); ++iter) {
                            iter->second.disable();
                        }
                        
                        LOG_INFO1 << "The translation servers are disabled" << END_LOG; 
                    }

                protected:

                    /**
                     * Allows to notify that there were disconnected servers
                     * so that we could immediately take care of them by trying
                     * to reconnect.
                     * @param adapter the referene to the translation server adapter that got disconnected
                     */
                    static inline void notify_disconnected(const translation_server_adapter & adapter) {
                        //NOTE: Do not really need to wake up the reconnection thread as it
                        //has a time out and if notified it will keep reconnecting too often
                        //m_re_connect_condition.notify_all();

                        //ToDo: Cancel the jobs associated with the given translation server
                    }

                    /**
                     * Is run within a separate thread which allows to periodically
                     * try to re-connect disconnected the servers.
                     */
                    static inline void re_connect_servers() {
                        //Get the time to wait 
                        std::chrono::milliseconds time_to_wait = std::chrono::milliseconds{m_params->m_recon_time_out};

                        //Run the re-connection loop
                        while (m_is_reconnect_run) {
                            unique_guard guard(m_re_connect_mutex);

                            //Make the thread sleep, use the real server specific parameter in milliseconds
                            m_re_connect_condition.wait_for(guard, time_to_wait);

                            //If we were not waken up to stop, then check on the disconnected servers
                            if (m_is_reconnect_run) {
                                //Iterate through all the connectors and check for their activity
                                for (auto iter = m_server_adaptors.begin(); iter != m_server_adaptors.end(); ++iter) {
                                    if (iter->second.is_enabled() && iter->second.is_disconnected()) {
                                        iter->second.disable();
                                        iter->second.enable();
                                    }
                                }
                            }
                        }
                    }

                    /**
                     * Allows to start the re-connection thread.
                     */
                    static inline void start_re_connection_thread() {
                        //Check if the re-connection thread is present, if not then add it
                        if (m_re_connect == NULL) {
                            //Create a thread that will take care of re-connecting
                            m_re_connect = new thread(translation_servers_manager::re_connect_servers);
                        }
                    }

                    /**
                     * Allows to finish the re-connection thread.
                     */
                    static inline void finish_re_connection_thread() {
                        if (m_re_connect != NULL) {
                            //Make sure that the re-connection thread terminates
                            m_is_reconnect_run = false;

                            //Wake up the thread
                            m_re_connect_condition.notify_all();

                            //Check if the re-connection thread is joinable
                            if (m_re_connect->joinable()) {
                                LOG_INFO << "Waiting for the re-connection thread to stop " << END_LOG;
                                m_re_connect->join();
                            }

                            //Delete the thread
                            delete m_re_connect;
                            m_re_connect = NULL;
                        }
                    }

                private:
                    //Stores the pointer to the parameters structure 
                    static const balancer_parameters * m_params;
                    //Stores the mapping from the server names to the server adaptors
                    static map<string, translation_server_adapter> m_server_adaptors;
                    //Stores the pointer to the re-connection thread
                    static thread * m_re_connect;
                    //Stores the synchronization primitive instances
                    static mutex m_re_connect_mutex;
                    static condition_variable m_re_connect_condition;
                    //Stores the flag that indicates for how long the reconnection thread needs to run
                    static atomic<bool> m_is_reconnect_run;

                    //Stores the mapping from the source/target language pairs to the adaptor sets

                    /**
                     * The private constructor to keep the class from being instantiated
                     */
                    translation_servers_manager() {
                    }
                };
            }
        }
    }
}
#endif /* TRANSLATION_SERVERS_MANAGER_HPP */

