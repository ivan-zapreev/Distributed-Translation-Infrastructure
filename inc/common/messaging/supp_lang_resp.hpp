/* 
 * File:   supp_lang_resp.hpp
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
 * Created on June 23, 2016, 3:36 PM
 */

#ifndef SUPP_LANG_RESP_HPP
#define SUPP_LANG_RESP_HPP

#include <common/messaging/response_msg.hpp>

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace common {
                namespace messaging {

                    /**
                     * This is the base class for all the supported languages response messages
                     */
                    class supp_lang_resp : public response_msg {
                    public:
                        //Stores the languages field name for the JSON message
                        static const char * LANGUAGES_FIELD_NAME;

                        /**
                         * The basic constructor
                         */
                        supp_lang_resp() : response_msg() {
                            //Nothing to be done here
                        }

                        /**
                         * The basic destructor
                         */
                        virtual ~supp_lang_resp() {
                            //Nothing to be done here
                        }
                    };

                }
            }
        }
    }
}

#endif /* SUPP_LANG_RESP_HPP */

