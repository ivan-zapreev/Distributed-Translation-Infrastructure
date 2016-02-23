/* 
 * File:   server_constants.hpp
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
 * Created on February 23, 2016, 5:09 PM
 */

#ifndef SERVER_CONSTANTS_HPP
#define SERVER_CONSTANTS_HPP

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {

                //Stores the word hash for an undefined phrase, is 0
                //WARNING! MUST BE 0 as this is the value of a default initialized integer!
                static constexpr phrase_uid UNDEFINED_PHRASE_ID = 0;
                //Define the unknown phrase id value
                static constexpr phrase_uid UNKNOWN_PHRASE_ID = UNDEFINED_PHRASE_ID + 1;
                //Contains the minimum valid phrase id value
                static constexpr phrase_uid MIN_VALID_PHRASE_ID = UNKNOWN_PHRASE_ID + 1;
                //Contains the maximum valid phrase id value
                static constexpr phrase_uid MAX_VALID_PHRASE_ID = UINT64_MAX;

                //Stores the word hash for an undefined word, is 0
                //WARNING! MUST BE 0 as this is the value of a default initialized integer!
                static constexpr word_uid UNDEFINED_WORD_ID = 0;
                //Stores the word id for an unknown word, it must have value 1
                static constexpr word_uid UNKNOWN_WORD_ID = (UNDEFINED_WORD_ID + 1);
                //Stores the minimum known word id, it must have value 2
                static constexpr word_uid MIN_KNOWN_WORD_ID = (UNKNOWN_WORD_ID + 1);
                //The word indexes that start from 2, as 0 is given to UNDEFINED and 1 to UNKNOWN (<unk>)
                static constexpr word_uid EXTRA_NUMBER_OF_WORD_IDs = 2;
            }
        }
    }
}

#endif /* SERVER_CONSTANTS_HPP */

