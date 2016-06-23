/* 
 * File:   msg_base.hpp
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
 * Created on June 23, 2016, 3:33 PM
 */

#ifndef MSG_BASE_HPP
#define MSG_BASE_HPP

#include <string>
#include <exception>
#include <stdint.h>

//Disable the assertions in the JSON code
#define NDEBUG true
#include <json.hpp>
#undef NDEBUG

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"

using namespace std;

using namespace uva::utils::logging;
using namespace uva::utils::exceptions;

using json = nlohmann::json;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace common {
                namespace messaging {

                    /**
                     * This enumeration stores the available message type values
                     */
                    enum msg_type {
                        //The message type is undefined
                        MESSAGE_UNDEFINED = 0,
                        //The supported languages request message
                        MESSAGE_SUPP_LANG_REQ = 1,
                        //The supported languages response message
                        MESSAGE_SUPP_LANG_RESP = 2,
                        //The translation job request message
                        MESSAGE_TRANS_JOB_REQ = 3,
                        //The translation job response message
                        MESSAGE_TRANS_JOB_RESP = 4
                    };

                    /**
                     * This class represents a JSON message begin sent between the client and the server.
                     * Responsibilities:
                     * - Is a base class for all messages
                     * - Stores the all-messages field names and the json object 
                     */
                    class msg_base {
                    public:
                        //Stores the version of the message protocol
                        static constexpr uint32_t PROTOCOL_VERSION = 1;
                        //Stores the protocol version attribute name
                        static const string PROT_VER_FIELD_NAME;
                        //Stores the message type attribute name
                        static const string MSG_TYPE_FIELD_NAME;

                        /**
                         * The basic constructor
                         */
                        msg_base() : m_json() {
                            //Nothing to be done here
                        }

                        /**
                         * The basic destructor
                         */
                        virtual ~msg_base() {
                            //Nothing to be done here
                        }

                    protected:
                        //Stores the json structure representing the message
                        json m_json;
                    };

                }
            }
        }
    }
}

#endif /* MSG_BASE_HPP */

