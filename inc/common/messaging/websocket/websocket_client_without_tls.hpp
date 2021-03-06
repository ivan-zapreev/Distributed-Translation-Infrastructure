/* 
 * File:   websocket_client_without_tls.hpp
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
 * Created on May 26, 2018, 10:32 PM
 */

#ifndef WEBSOCKET_CLIENT_WITHOUT_TLS_HPP
#define WEBSOCKET_CLIENT_WITHOUT_TLS_HPP

#include "common/messaging/websocket/websocket_client_base.hpp"

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace common {
                namespace messaging {
                    namespace websocket {

                        //The generic class without TLS is nothing more but the base template with specialization
                        typedef websocket_client_base<websocketpp::config::asio_client> websocket_client_no_tls;

                    }
                }
            }
        }
    }
}

#endif /* WEBSOCKET_CLIENT_WITHOUT_TLS_HPP */

