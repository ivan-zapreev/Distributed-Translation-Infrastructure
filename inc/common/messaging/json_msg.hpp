/* 
 * File:   json_msg.hpp
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
 * Created on June 20, 2016, 3:00 PM
 */

#ifndef JSON_MSG_HPP
#define JSON_MSG_HPP

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
                     * This is a generic message that has at least two attributes, the protocol version and
                     * the message type. Other attributes are possible but are not handled by this class itself.
                     */
                    class json_msg {
                    public:

                        //Declare the friend classes
                        friend class trans_job_request;
                        friend class supp_lang_response;
                        
                        //Stores the version of the message protocol
                        static constexpr uint32_t PROTOCOL_VERSION = 1;
                        //Stores the protocol version attribute name
                        static const string PROT_VER_NAME;
                        //Stores the message type attribute name
                        static const string MSG_TYPE_NAME;

                        /**
                         * The basic constructor, is to be used for the received messages
                         */
                        json_msg() : m_json_obj() {
                        }

                        /**
                         * The basic constructor that is to be used for the messages to be sent.
                         * An instance must receive the message type identifier 
                         * @param type the message type identifier
                         */
                        json_msg(msg_type type) : m_json_obj() {
                            m_json_obj[PROT_VER_NAME] = PROTOCOL_VERSION;
                            m_json_obj[MSG_TYPE_NAME] = type;
                        }

                        /**
                         * Allows to return the message type
                         * @return the message type
                         */
                        inline msg_type get_type() const {
                            //This is a primitive way to cast to the enumeration type 
                            //from an integer. Later we could introduce a fancier way 
                            //with all sorts of checks but this shall do it for now.
                            return static_cast<msg_type> (get_value<int32_t>(MSG_TYPE_NAME));
                        }

                        /**
                         * Allows to de-serialize the JSON string into the json object
                         * @param data the JSON string to be parsed
                         */
                        inline void de_serialize(const string & data) {
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
                         * Allows to serialize the message into a string.
                         * @return the string representation of the JSON message
                         */
                        inline string serialize() const {
                            return m_json_obj.dump();
                        }

                        /**
                         * Allows to get the data from the JSON object and cast it to the desired result type
                         * @param result_type the result type to be returned
                         * @param field_name the name of the JSON field
                         * @return the value of the required type of the given field 
                         */
                        template<typename result_type>
                        inline result_type get_value(const string & field_name) const {
                            LOG_DEBUG << "Extracting JSON field: " << field_name << END_LOG;

                            //Retrieve the value from the JSON object
                            auto entry = m_json_obj.find(field_name);

                            //Assert sanity, the property must be present
                            ASSERT_CONDITION_THROW((entry == m_json_obj.end()),
                                    string("The JSON field '") + field_name + string("' is not present!"));

                            return entry->get<result_type>();
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
                            const uint32_t prot_ver = get_value<int32_t>(PROT_VER_NAME);

                            LOG_DEBUG << "The request protocol version: " << to_string(prot_ver)
                                    << ", local protocol version: " << to_string(PROTOCOL_VERSION) << END_LOG;

                            ASSERT_CONDITION_THROW((prot_ver > PROTOCOL_VERSION),
                                    string("Client/Server protocol version miss-match got: ") +
                                    to_string(prot_ver) + string(" expected <= : ") +
                                    to_string(PROTOCOL_VERSION));
                        }

                    private:
                        //Stores the json object instance
                        json m_json_obj;
                    };

                    //Typedef the JSON message pointer
                    typedef json_msg * json_msg_ptr;

                }
            }
        }
    }
}

#endif /* JSON_MSG_HPP */

