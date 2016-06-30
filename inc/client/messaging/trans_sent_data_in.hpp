/* 
 * File:   trans_sent_data_in.hpp
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

#ifndef TRANS_SENT_DATA_IN_HPP
#define TRANS_SENT_DATA_IN_HPP

#include <common/messaging/trans_sent_data.hpp>
#include <common/messaging/incoming_msg.hpp>
#include "common/messaging/status_code.hpp"

using namespace uva::smt::bpbd::common::messaging;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace client {
                namespace messaging {

                    /**
                     * This class represents a translated sentence data
                     * in an incoming translation job response message.
                     */
                    class trans_sent_data_in : public trans_sent_data {
                    public:

                        /**
                         * The basic constructor. This class is just a wrapper for a
                         * JSON object, but it does not own it.
                         * @param data_obj the reference to the encapsulated JSON object
                         */
                        trans_sent_data_in()
                        : trans_sent_data(), m_data_obj(NULL) {
                            //Nothing to be done here
                        }

                        /**
                         * The basic constructor
                         */
                        virtual ~trans_sent_data_in() {
                            //Nothing to be done here
                        }

                        /**
                         * Allows to get the translation task result code
                         * @return the translation task result code
                         */
                        inline status_code get_status_code() const {
                            //Create the status code class instance
                            return status_code(m_data_obj->operator [](STAT_CODE_FIELD_NAME).GetInt());
                        }

                        /**
                         * Allows to get the translation task status message
                         * @return the translation task status message
                         */
                        inline string get_status_msg() const {
                            return m_data_obj->operator [](STAT_MSG_FIELD_NAME).GetString();
                        }

                        /**
                         * Allows to get a reference to the translated text string.
                         * @return a reference to the translated text string.
                         */
                        inline string get_trans_text() const {
                            return m_data_obj->operator [](TRANS_TEXT_FIELD_NAME).GetString();
                        }

                        /**
                         * Allows to get a reference to an array of stack load numbers
                         * @return a reference to an array of stack load numbers
                         */
                        inline const Value & get_stack_load() const {
                            return m_data_obj->operator [](STACK_LOAD_FIELD_NAME);
                        }

                        /**
                         * Allows to check if the stack loads are present
                         * @return true if the stack loads are present, otherwise false
                         */
                        inline bool has_stack_load() const {
                            return m_data_obj->HasMember(STACK_LOAD_FIELD_NAME);
                        }

                        /**
                         * Allows to replace a stored reference to a JSON object with a new reference.
                         * @param data_obj the reference to a new JSON object
                         */
                        inline void set_sent_data(const Value & data_obj) {
                            m_data_obj = &data_obj;
                        }

                    private:
                        //Stores the pointer to the encapsulated JSON object
                        const Value * m_data_obj;
                    };

                }
            }
        }
    }
}

#endif /* TRANS_SENT_DATA_IN_HPP */

