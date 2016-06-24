/* 
 * File:   trans_sent_data_out.hpp
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
 * Created on June 23, 2016, 3:37 PM
 */

#ifndef TRANS_SENT_DATA_OUT_HPP
#define TRANS_SENT_DATA_OUT_HPP

#include "common/messaging/trans_sent_data.hpp"
#include "common/messaging/outgoing_msg.hpp"

#include "server/messaging/trans_sent_data_out.hpp"

using namespace uva::smt::bpbd::common::messaging;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace messaging {

                    /**
                     * This class represents the translated sentence data object to be sent out to the client.
                     */
                    class trans_sent_data_out : public trans_sent_data {
                    public:

                        /**
                         * The basic constructor. This class is just a wrapper for a
                         * JSON object, but it does not own it.
                         * @param data_obj the reference to the encapsulated JSON object
                         */
                        trans_sent_data_out(json::object_t & data_obj) : trans_sent_data(data_obj) {
                            m_data_obj[TRANS_TEXT_FIELD_NAME] = "";
                            m_data_obj[STACK_LOAD_FIELD_NAME] = {};
                        }

                        /**
                         * Allows to set the message status: code and message
                         * @param code the status code
                         * @param msg the status message
                         */
                        inline void set_status(const status_code & code, const string & msg) {
                            m_data_obj[STAT_CODE_FIELD_NAME] = code;
                            m_data_obj[STAT_MSG_FIELD_NAME] = msg;
                        }

                        /**
                         * Allows to get the reference to the translation text, to be filled in.
                         * @return the reference to the translation text string
                         */
                        inline const string & get_trans_text() {
                            return m_data_obj[TRANS_TEXT_FIELD_NAME];
                        }

                        /**
                         * Allows to get the reference to the stack loads array
                         * @return the reference to the stack loads array
                         */
                        inline const loads_array & get_stack_load() {
                            return m_data_obj[STACK_LOAD_FIELD_NAME];
                        }
                    };

                }
            }
        }
    }
}

#endif /* TRANS_SENT_DATA_OUT_HPP */

