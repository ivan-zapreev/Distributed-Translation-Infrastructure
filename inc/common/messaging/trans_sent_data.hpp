/* 
 * File:   trans_sent_data.hpp
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
 * Created on June 23, 2016, 3:35 PM
 */

#ifndef TRANS_SENT_DATA_HPP
#define TRANS_SENT_DATA_HPP

#include <string>

//Disable the assertions in the JSON code
#define NDEBUG true
#include <json.hpp>
#undef NDEBUG

#include <common/messaging/outgoing_msg.hpp>

using namespace std;

using json = nlohmann::json;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace common {
                namespace messaging {

                    /**
                     * This class stores the translation data for a translated sentence.
                     * It wraps around a JSON object, but it does not own it.
                     */
                    class trans_sent_data : public response_msg {
                    public:
                        //The target data field name
                        static const string TRANS_TEXT_FIELD_NAME;
                        //The target data field name
                        static const string STACK_LOAD_FIELD_NAME;

                        //Typedef the loads array data structure for storing the stack load percent values
                        typedef vector<int64_t> loads_array;

                        /**
                         * The basic constructor. This class is just a wrapper for a
                         * JSON object, but it does not own it.
                         * @param data_obj the reference to the encapsulated JSON object
                         */
                        trans_sent_data(json::object_t & data_obj)
                        : response_msg(), m_data_obj(data_obj) {
                            //Nothing to be done here
                        }

                        /**
                         * The basic destructor
                         */
                        virtual ~trans_sent_data() {
                            //Nothing to be done here
                        }

                        /**
                         * Allows to replace a stored reference to a JSON object with a new reference.
                         * @param data_obj the reference to a new JSON object.
                         */
                        inline void set_sent_data((json::object_t & data_obj)) {
                            m_data_obj = data_obj;
                        }

                    protected:
                        //Stores the reference to the encapsulated JSON object
                        json::object_t & m_data_obj;
                    };

                }
            }
        }
    }
}

#endif /* TRANS_SENT_DATA_HPP */

