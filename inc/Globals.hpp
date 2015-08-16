/* 
 * File:   Globals.hpp
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
 * Created on April 20, 2015, 8:58 PM
 */

#ifndef GLOBALS_HPP
#define	GLOBALS_HPP

#include <inttypes.h>
#include <string>

#include "Logger.hpp"

namespace uva {
    namespace smt {

        //This is the pattern used for file path separation
        const string PATH_SEPARATION_SYMBOLS = "/\\";
        //This is a delimiter used in the text corpus and test files
        const char TOKEN_DELIMITER_CHAR = ' ';
        //The expected number of program arguments
        const uint EXPECTED_NUMBER_OF_ARGUMENTS = 3;
        //The expected number of user arguments
        const uint EXPECTED_USER_NUMBER_OF_ARGUMENTS = (EXPECTED_NUMBER_OF_ARGUMENTS - 1);
        //The number of bytes in one Mb
        const uint BYTES_ONE_MB = 1024u;

        //The following type definitions are important for storing the Tries information
        namespace tries {
            //This typedef if used in the tries in order to specify the type of the N-gram level NcontextHash
            typedef uint16_t TModelLevel;

            //The considered maximum length of the N-gram
            const TModelLevel MAX_NGRAM_LEVEL = 5u;
            //Defined the minimum possible N-gram level
            const TModelLevel MIN_NGRAM_LEVEL = 1u;
            
            namespace __AHashMapTrie {
                //The unordered map memory factor for the Word index in AHashMapTrie
                const float UNORDERED_MAP_WORD_INDEX_MEMORY_FACTOR = 2.6;
            }
            
            namespace __ContextMultiHashMapTrie {
                //The unordered map memory factor for the One-Grams in ContextMultiHashMapTrie
                const float UNORDERED_MAP_O_GRAM_MEMORY_FACTOR = 2.6;
                //The unordered map memory factor for the M-Grams in ContextMultiHashMapTrie
                const float UNORDERED_MAP_M_GRAM_MEMORY_FACTOR = 2.0;
                //The unordered map memory factor for the N-Grams in ContextMultiHashMapTrie
                const float UNORDERED_MAP_N_GRAM_MEMORY_FACTOR = 2.5;
            }
        }

        //The following type definitions are important for creating hashes
        namespace hashing {
            //This is the smallest size which I've tested and it works for the hash without collisions
            typedef uint32_t TWordHashSize;
            //This is the hash reference size which should be twice as long as the TWordHashSize
            typedef uint64_t TReferenceHashSize;
        }
    }
}

#endif	/* GLOBALS_HPP */

