/* 
 * File:   model_m_gram.cpp
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
 * Created on February 25, 2016, 9:19 AM
 */

#include "server/lm/mgrams/model_m_gram.hpp"
#include "common/utils/string_utils.hpp"

using namespace uva::utils::text;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace lm {
                    namespace m_grams {

                        /**
                         * Allows to serialize the m-gram to the output stream as a string
                         * @param stream the reference to the stream to output into
                         * @param gram the m-gram object to output
                         * @return the reference to the stream
                         */
                        ostream& operator<<(ostream& stream, const model_m_gram & gram) {
                            //Get the current level for logging
                            const phrase_length & curr_level = gram.get_num_words();

                            //Do the output and return
                            return stream << SSTR(curr_level) << "-gram [" << gram.get_first_word_idx() << ","
                                    << gram.get_last_word_idx() << "] with tokens " <<
                                    tokens_to_string<MODEL_M_GRAM_MAX_LEN>(gram.m_tokens,
                                    gram.get_first_word_idx(), gram.get_last_word_idx());
                        };
                    }
                }
            }
        }
    }
}