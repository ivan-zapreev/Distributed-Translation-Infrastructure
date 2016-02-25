/* 
 * File:   m_gram_query.cpp
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
 * Created on February 24, 2016, 4:05 PM
 */

#include "server/lm/models/m_gram_query.hpp"

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace lm {

                    /**
                     * Allows to serialize the m-gram query to the output stream as a string
                     * @param stream the reference to the stream to output into
                     * @param query the query object to output
                     * @return the reference to the stream
                     */
                    ostream& operator<<(ostream& stream, const m_gram_query & query) {
                        //Get the current level for logging
                        const phrase_length & curr_level =query.get_curr_level();

                        //Do the output and return
                        return stream << SSTR(curr_level) << "-gram [" << query.m_curr_begin_word_idx << ","
                                << query.m_curr_end_word_idx << "] with word ids " << query.m_gram;
                    };
                }
            }
        }
    }
}