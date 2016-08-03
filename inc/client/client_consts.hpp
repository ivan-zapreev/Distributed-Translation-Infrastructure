/* 
 * File:   client_consts.hpp
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
 * Created on August 1, 2016, 11:17 AM
 */

#ifndef CLIENT_CONSTS_HPP
#define CLIENT_CONSTS_HPP

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace client {
                
                //The maximum line length is bytes
                static constexpr size_t LINE_MAX_BYTES_LEN = 1024*1024;
                
                //The maximum processor message text size is characters. This limits
                //the number of characters to send per processor request.
                static constexpr size_t MESSAGE_MAX_CHAR_LEN = 10*1024;
                
            }
        }
    }
}

#endif /* CLIENT_CONSTS_HPP */

