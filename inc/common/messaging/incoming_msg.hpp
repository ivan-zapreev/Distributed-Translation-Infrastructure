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

#include "rapidjson/document.h"

#include "common/messaging/msg_base.hpp"

using namespace rapidjson;

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
                        incoming_msg() : msg_base(), m_json() {
                            //Nothing to be done here
                        }

                        /**
                         * The basic destructor
                         */
                        virtual ~incoming_msg() {
                            //Nothing to be done here
                        }

                        /**
                         * Allows to de-serialize the json request
                         * @param data the json string to be parsed
                         */
                        inline void de_serialize(const string & data) {
                            LOG_DEBUG << "Incoming JSON message: " << data << END_LOG;

                            //De-serialize the data and catch any exception, convert it into our type
                            try {
                                //Parse the json request
                                m_json.Parse(data.c_str());

                                //Check if there was an error while parsing
                                ASSERT_CONDITION_THROW(m_json.HasParseError(),
                                        string("JSON parse error: ") + to_string(m_json.GetParseError()));
                            } catch (std::exception & ex) {
                                LOG_ERROR << "An exception when parsing JSON string: " << ex.what() << END_LOG;
                                THROW_EXCEPTION(ex.what());
                            }

                            //Verify that the version number is good
                            verify_protocol_version();
                        }

                        /**
                         * Allows to get the message type
                         * @return the message type
                         */
                        inline msg_type get_msg_type() const {
                            //This is a primitive way to cast to the enumeration type 
                            //from an integer. Later we could introduce a fancier way 
                            //with all sorts of checks but this shall do it for now.
                            return static_cast<msg_type> (m_json[MSG_TYPE_FIELD_NAME].GetInt());
                        }

                        /**
                         * Allows to get a reference to a constant json document
                         * @return the reference to the constant json document.
                         */
                        inline const Document & get_json() const {
                            return m_json;
                        }

                        /**
                         * Allows to get a reference to a constant json document
                         * @return the reference to the constant json document.
                         */
                        inline Document & get_json() {
                            return m_json;
                        }

                        /**
                         * @see msg_base
                         */
                        virtual string serialize() const override {
                            return m_json.GetString();
                        }

                    protected:
                        //Stores the json document representing the message
                        Document m_json;

                        /**
                         * Allows to check if the protocol version is fine.
                         * The protocol version of the received message is
                         * to be less or equal than the constant. 
                         * @return true if the received message version is less or equal to that of the version constant. 
                         */
                        void verify_protocol_version() const {
                            //Get the protocol version and check it
                            const uint32_t prot_ver = m_json[PROT_VER_FIELD_NAME].GetUint();

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

