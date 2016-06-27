/* 
 * File:   supp_lang_resp_in.hpp
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
 * Created on June 23, 2016, 3:38 PM
 */

#ifndef SUPP_LANG_RESP_IN_HPP
#define SUPP_LANG_RESP_IN_HPP

#include <string>
#include <vector>
#include <map>

#include "common/messaging/incoming_msg.hpp"
#include "common/messaging/supp_lang_resp.hpp"

using namespace std;
using namespace uva::smt::bpbd::common::messaging;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace client {
                namespace messaging {

                    /**
                     * This class represents an outgoing request for supported
                     * languages, created received by the client side.
                     */
                    class supp_lang_resp_in : public supp_lang_resp {
                    public:
                        //Typedef the data structure storing the supported languages
                        typedef map<string, vector<string>> supp_lang_map;

                        /**
                         * The basic constructor
                         * @param inc_msg the pointer to the incoming message, NOT NULL
                         */
                        supp_lang_resp_in(incoming_msg * inc_msg)
                        : supp_lang_resp(), m_inc_msg(inc_msg) {
                            //Nothing to be done here
                        }

                        /**
                         * The basic destructor
                         */
                        virtual ~supp_lang_resp_in() {
                            delete m_inc_msg;
                        }

                        /**
                         * Allows to retrieve the JSON object storing the supported languages
                         * @return the object storing the JSON supported languages mapping
                         */
                        inline const json & get_languages() const {
                            return m_inc_msg->get_value(LANGUAGES_FIELD_NAME);
                        }

                    private:
                        //Stores the pointer to the incoming message storing
                        //the response data, this pointer must not be NULL
                        incoming_msg * m_inc_msg;
                    };

                }
            }
        }
    }
}

#endif /* SUPP_LANG_RESP_IN_HPP */

