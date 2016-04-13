/* 
 * File:   de_configs.hpp
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
 * Created on February 11, 2016, 5:51 PM
 */

#ifndef DE_CONFIGS_HPP
#define DE_CONFIGS_HPP

#include <inttypes.h>
#include <cstdint>
#include <string>

#include "server/server_configs.hpp"

using namespace std;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace decoder {
                    //Stores the maximum considered number of words per sentence
                    static const size_t MAX_WORDS_PER_SENTENCE = 256;
                }
            }
        }
    }
}

#endif /* DE_CONFIGS_HPP */

