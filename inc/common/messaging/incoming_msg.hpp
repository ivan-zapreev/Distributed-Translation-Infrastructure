/* 
 * File:   incoming_msg.hpp
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
 * Created on June 23, 2016, 3:34 PM
 */

#ifndef INCOMING_MSG_HPP
#define INCOMING_MSG_HPP

#include "common/messaging/msg_base.hpp"

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace common {
                namespace messaging {

                    /**
                     * This class represents a JSON message begin sent between the client and the server.
                     */
                    class incoming_msg : public msg_base {
                    public:

                        /**
                         * The basic constructor
                         */
                        client_msg() : msg_base() {
                            //Nothing to be done here
                        }

                        /**
                         * The basic destructor
                         */
                        virtual ~client_msg() {
                            //Nothing to be done here
                        }

                        /**
                         * 
                         * @param data
                         */
                        void de_serialize(const string & data) {
                            //De-serialize the data and catch any exception, convert it into our type
                            try {
                                stringstream stream_data(data);
                                stream_data >> m_json_obj;
                            } catch (std::exception & ex) {
                                LOG_ERROR << "An exception when parsing JSON string: " << ex.what() << END_LOG;
                                THROW_EXCEPTION(ex.what());
                            }

                            //Verify that the version number is good
                            verify_protocol_version();
                        }

                        /**
                         * 
                         * @return 
                         */
                        msg_type get_type() const {
                            //This is a primitive way to cast to the enumeration type 
                            //from an integer. Later we could introduce a fancier way 
                            //with all sorts of checks but this shall do it for now.
                            return static_cast<msg_type> (get_value<int32_t>(MSG_TYPE_FIELD_NAME));
                        }

                        /**
                         * Allows to get the data from the JSON object and
                         * cast it to the desired result type.
                         * @param value_type the result type to be returned
                         * @param object the json object to get the field value from
                         * @param field_name the name of the JSON field
                         * @return the value of the required type of the given field 
                         */
                        template<typename value_type>
                        static inline const value_type & get_value(const json::object_t & object, const string & field_name) {
                            LOG_DEBUG << "Extracting JSON field: " << field_name << END_LOG;

                            //Retrieve the value from the JSON object
                            auto entry = object.find(field_name);

                            //Assert sanity, the property must be present
                            ASSERT_CONDITION_THROW((entry == object.end()),
                                    string("The JSON field '") + field_name + string("' is not present!"));

                            return *entry;
                        }

                        /**
                         * Allows to get the data from the JSON object and
                         * cast it to the desired result type.
                         * @param value_type the result type to be returned
                         * @param field_name the name of the JSON field
                         * @return the value of the required type of the given field 
                         */
                        template<typename value_type>
                        inline const value_type & get_value(const string & field_name) const {
                            return get_value(m_json, field_name);
                        }

                        /**
                         * Allows to get a reference to the internally stored json object
                         * @return the reference to the internally stored json object.
                         */
                        inline const json & get_json() const {
                            return m_json;
                        }

                    protected:

                        /**
                         * Allows to check if the protocol version is fine.
                         * The protocol version of the received message is
                         * to be less or equal than the constant. 
                         * @return true if the received message version is less or equal to that of the version constant. 
                         */
                        void verify_protocol_version() const {
                            //Get the protocol version and check it
                            const uint32_t prot_ver = get_value<int32_t>(PROT_VER_FIELD_NAME);

                            LOG_DEBUG << "The request protocol version: " << to_string(prot_ver)
                                    << ", local protocol version: " << to_string(PROTOCOL_VERSION) << END_LOG;

                            ASSERT_CONDITION_THROW((prot_ver > PROTOCOL_VERSION),
                                    string("Client/Server protocol version miss-match got: ") +
                                    to_string(prot_ver) + string(" expected <= : ") +
                                    to_string(PROTOCOL_VERSION));
                        }
                    };

                }
            }
        }
    }
}


#endif /* INCOMING_MSG_HPP */

