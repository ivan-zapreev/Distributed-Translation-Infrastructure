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

                    /**
                     * The basic constructor for the adapter class
                     */
                    translation_server_adapter() : m_params(NULL), m_client(NULL) {
                    }

                    /**
                     * The basic destructor for the adapter class
                     */
                    virtual ~translation_server_adapter() {
                        //Remove the previous connection client if any
                        remove_connection_client();
                    }

                    /**
                     * Allows to configure the adapter with the translation server parameters
                     * @param params the translation server parameters
                     */
                    inline void configure(const trans_server_params & params) {
                        //Store the reference to the parameters
                        m_params = &params;

                        //Remove the previous connection client if any
                        remove_connection_client();

                        //Create a new translation client
                        m_client = new translation_client(m_params->m_address, m_params->m_port,
                                bind(&translation_server_adapter::set_job_response, this, _1),
                                bind(&translation_server_adapter::notify_conn_closed, this));
                    }

                    /**
                     * The main method to start the translation server adapter
                     */
                    inline void start() {
                        //Check the sanity, that the order is correct
                        ASSERT_SANITY_THROW((m_client == NULL), "The translation server adapter is NULL ");

                        //Try to connect to the server
                        if (! m_client->connect()) {
                            LOG_INFO << "Could not connect to the '" << m_params->m_name << "' translation server." << END_LOG;
                            //ToDo: Make a delayed attempt to re-connect
                        }
                    }

                    /**
                     * Allows to stop the translation server adapter
                     */
                    inline void stop() {
                        LOG_INFO << "Disconnecting the client ... " << END_LOG;
                        //Disconnect from the server
                        m_client->disconnect();
                        LOG_INFO << "The client is disconnected" << END_LOG;
                        
                        //ToDo: Cancel any delayed attempt to re-connect to the server
                    }

                protected:

                    /**
                     * Allows to process the server job request response
                     * @param trans_job_resp a pointer to the translation job response data, not NULL
                     */
                    void set_job_response(trans_job_resp_in * trans_job_resp) {
                        LOG_DEBUG << "The client got a translation job response!" << END_LOG;

                        //ToDo: Implement
                    }

                    /**
                     * This function will be called if the connection is closed during the translation process
                     */
                    void notify_conn_closed() {
                        LOG_WARNING << "The server has closed the connection!" << END_LOG;

                        //ToDo: Implement
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

                };

            }
        }
    }
}

#endif /* TRANSLATION_SERVER_ADAPTER_HPP */

