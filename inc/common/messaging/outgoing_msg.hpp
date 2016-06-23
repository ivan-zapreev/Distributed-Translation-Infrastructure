/* 
 * File:   outgoing_msg.hpp
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

#ifndef OUTGOING_MSG_HPP
#define OUTGOING_MSG_HPP

#include "common/messaging/msg_base.hpp"

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace common {
                namespace messaging {

                    /**
                     * This class represents a JSON message begin sent between the client and the server.
                     */
                    class outgoing_msg : private msg_base {
                    public:

                        /**
                         * The basic constructor
                         * @param type the message type
                         */
                        outgoing_msg(msg_type type) : msg_base() {
                            //Initialize the outgoing message with the protocol version and type data
                            m_json[PROT_VER_FIELD_NAME] = PROTOCOL_VERSION;
                            m_json[MSG_TYPE_FIELD_NAME] = type;
                        }

                        /**
                         * The basic destructor
                         */
                        virtual ~outgoing_msg() {
                            //Nothing to be done here
                        }

                        /**
                         * Allows to set the given field to the given value
                         * @param field_name the field name
                         * @param value the value to be set
                         */
                        template<typename value_type>
                        void set_value(const string & field_name, const value_type &value) {
                            m_json[field_name] = value;
                        }

                        /**
                         * Allows to serialize the outgoing message into a string
                         * @return the string representation of the message
                         */
                        inline string serialize() const {
                            return m_json.size();
                        }

                        /**
                         * Allows to set a field to a given value in the given json object
                         * @param object the json object to set the field value in
                         * @param field_name the field name
                         * @param value the value
                         */
                        template<typename value_type>
                        static inline void set_value(json::object_t & object, const string & field_name, const value_type & value) {
                            object[field_name] = value;
                        }
                        
                    protected:

                        /**
                         * Allows to get a reference to the internally stored json object
                         * @return the reference to the internally stored json object.
                         */
                        inline json & get_json() {
                            return m_json;
                        }
                    };

                }
            }
        }
    }
}


#endif /* OUTGOING_MSG_HPP */

