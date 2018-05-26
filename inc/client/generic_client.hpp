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

#ifndef GENERIC_CLIENT_HPP
#define GENERIC_CLIENT_HPP

#define ASIO_STANDALONE
#include <websocketpp/client.hpp>

//Define an easy check macro for TLS support
#ifndef IS_TLS_SUPPORT
#if defined(WITH_TLS) && WITH_TLS
#define IS_TLS_SUPPORT true
#else
#define IS_TLS_SUPPORT false
#endif
#endif

#include "common/messaging/msg_base.hpp"
#include "common/messaging/incoming_msg.hpp"

using namespace uva::smt::bpbd::common::messaging;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace client {

                /**
                 * This class represents the generic client interface which has
                 * virtual functions to be overridden by the children classes. 
                 * It also contains several useful type definitions.
                 */
                class generic_client {
                protected:
                    
                    /**
                     * The basic constructor
                     */
                    generic_client() {
                        //Nothing to be done
                    }
                    
                public:

                    //Define the function type for the function used to set the translation job result
                    typedef function<void(incoming_msg * json_msg) > new_msg_notifier;

                    //Define the function type for the function used to notify about the connection status
                    typedef function<void() > conn_status_notifier;

                    /**
                     * The basic destructor that also stops the client
                     */
                    virtual ~generic_client() {
                        //Nothing to be done here
                    }

                    /**
                     * This method allows to open the connection in a non-blocking way
                     * @return true if the connection has been established
                     * 
                     */
                    virtual void connect_nb() = 0;

                    /**
                     * This method will block until the connection is complete
                     * @return true if the connection has been established
                     * 
                     */
                    virtual bool connect() = 0;

                    /**
                     * Allows to close the connection and stop the io service thread
                     */
                    virtual void disconnect() = 0;

                    /**
                     * Attempts to send an outgoing message to the server
                     * @param message a message to be sent, not NULL
                     */
                    virtual void send(const msg_base * message) = 0;

                    /**
                     * Allows to get the connection URI
                     * @return the connection URI
                     */
                    virtual const string get_uri() const = 0;

                    /**
                     * Allows to check whether the client is connected to the server
                     * @return 
                     */
                    virtual bool is_connected() const = 0;
                    
                };
            }
        }
    }
}

#endif /* GENERIC_CLIENT_HPP */

