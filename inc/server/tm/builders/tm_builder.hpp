/* 
 * File:   tm_builder.hpp
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
 * Created on May 4, 2016, 12:07 PM
 */

#ifndef TM_BUILDER_HPP
#define TM_BUILDER_HPP

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace tm {
                    namespace builders {

                        //Stores the translation model delimiter character for parsing one line
                        static const char TM_DELIMITER = '|';
                        //Stores the translation model delimiter character cardinality
                        static const size_t TM_DELIMITER_CDTY = 3;
                    }
                }
            }
        }
    }
}

#endif /* TM_BUILDER_HPP */

