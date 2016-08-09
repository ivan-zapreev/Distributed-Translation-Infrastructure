/* 
 * File:   processor_consts.hpp
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
 * Created on August 1, 2016, 11:30 AM
 */

#ifndef PROCESSOR_CONSTS_HPP
#define PROCESSOR_CONSTS_HPP

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace processor {
                
                //The maximum length of the processor script output in bytes.
                static constexpr size_t MAX_PROCESSOR_OUTPUT_BYTES = 1024;
                
                //The maximum processor message text size is characters. This limits
                //the number of characters to send per processor request.
                static constexpr size_t MESSAGE_MAX_CHAR_LEN = 10*1024;
                
                //Stores the number of attempts the client must try to do something through console
                static constexpr size_t MAX_NUM_CONSOLE_ATTEMPTS = 10;
                
                //Stores the number of milliseconds the client shall wait until a re-do the console action
                static constexpr size_t CONSOLE_RE_TRY_TIME_OUT_MILLISEC = 20;

            }
        }
    }
}

#endif /* PROCESSOR_CONSTS_HPP */

