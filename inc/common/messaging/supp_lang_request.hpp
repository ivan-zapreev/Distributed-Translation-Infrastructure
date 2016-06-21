/* 
 * File:   supp_lang_request.hpp
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
 * Created on June 16, 2016, 2:26 PM
 */

#ifndef SUPP_LANG_REQUEST_HPP
#define SUPP_LANG_REQUEST_HPP

#include <string>

#include "common/messaging/json_msg.hpp"

using namespace std;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace common {
                namespace messaging {

                    /**
                     * This class represents the supported languages request message.
                     * There is no constructor for the received supported languages
                     * request at the moment as that possesses no special request
                     * attributes to be extracted. 
                     */
                    class supp_lang_request {
                    public:

                        /**
                         * The basic class constructor to be used for the
                         * supported language requests to be sent to the server.
                         */
                        supp_lang_request() : m_msg(msg_type::MESSAGE_SUPP_LANG_REQ) {
                        }

                        /**
                         * The basic destructor
                         */
                        virtual ~supp_lang_request() {
                            //Nothing to be done here
                        }

                        /**
                         * Allows to serialize the supported languages request into a string
                         * @return the string representation of the supported languages request
                         */
                        inline string serialize() const {
                            return m_msg.serialize();
                        }

                    private:
                        //Stores the the JSON message object
                        json_msg m_msg;
                    };

                }
            }
        }
    }
}

#endif /* SUPP_LANG_REQUEST_HPP */

