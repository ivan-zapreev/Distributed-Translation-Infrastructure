/* 
 * File:   tm_phrase_id.hpp
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
 * Created on February 9, 2016, 5:35 PM
 */

#ifndef TM_PHRASE_ID_HPP
#define TM_PHRASE_ID_HPP

#include <string>

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"
#include "common/utils/hashing_utils.hpp"

using namespace std;

using namespace uva::utils::exceptions;
using namespace uva::utils::logging;
using namespace uva::utils::hashing;

namespace uva {
    namespace smt {
        namespace translation {
            namespace server {
                namespace tm {
                    namespace models {

                        //Declare the phrase unique identifier type
                        typedef uint64_t phrase_uid;

                        //Define the undefined phrase id value
                        static constexpr uint64_t UNDEFINED_PHRASE_ID = 0;
                        //Contains the minimum valid phrase id value
                        static constexpr uint64_t MIN_VALID_PHRASE_ID = UNDEFINED_PHRASE_ID + 1;

                        /**
                         * Allows to get the phrase uid for the given phrase.
                         * The current implementation uses the hash function to compute the uid.
                         * Before the computation of the phrase id the phrase string is trimmed.
                         * @param the phrase to get the uid for
                         * @return the uid of the phrase
                         */
                        static inline phrase_uid get_phrase_uid(const string phrase) {
                            //Compute the string hash
                            phrase_uid uid = compute_hash(phrase);
                            //If the value is somehow undefined then increase it to the minimum
                            if (uid == UNDEFINED_PHRASE_ID) {
                                uid = MIN_VALID_PHRASE_ID;
                            }
                            return uid;
                        }
                    }
                }
            }
        }
    }
}

#endif /* TM_PHRASE_ID_HPP */

