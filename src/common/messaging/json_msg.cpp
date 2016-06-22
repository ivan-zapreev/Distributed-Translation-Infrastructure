/* 
 * File:   json_msg.cpp
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

#include "common/messaging/json_msg.hpp"

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace common {
                namespace messaging {

                        constexpr uint32_t json_msg::PROTOCOL_VERSION;
                        
                        const string json_msg::PROT_VER_FIELD_NAME = "prot_ver";
                        const string json_msg::MSG_TYPE_FIELD_NAME = "msg_type";
                        const string json_msg::STAT_CODE_FIELD_NAME = "stat_code";
                        const string json_msg::STAT_MSG_FIELD_NAME = "stat_msg";
                }
            }
        }
    }
}