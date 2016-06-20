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
                     * This class represents the supported languages request message
                     */
                    class supp_lang_request{
                    public:
                        //The begin of the supported languages request message
                        static const string SUPP_LANG_REQUEST_PREFIX;

                        /**
                         * Allows to detect whether the given payload corresponds to the translation job request 
                         * @param payload the payload that stores the serialized message
                         * @return true if this is a translation job request, otherwise false
                         */
                        static inline bool is_request(const string & payload) {
                            return (payload.compare(0, SUPP_LANG_REQUEST_PREFIX.length(), SUPP_LANG_REQUEST_PREFIX) == 0);
                        }

                        /**
                         * Allows to serialize the supported languages request into a string
                         * @return the string representation of the supported languages request
                         */
                        const string & serialize() const {
                            return SUPP_LANG_REQUEST_PREFIX;
                        }
                    };
                    
                }
            }
        }
    }
}

#endif /* SUPP_LANG_REQUEST_HPP */

