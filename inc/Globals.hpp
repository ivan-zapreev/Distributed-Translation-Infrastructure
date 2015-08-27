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

using namespace std;

namespace uva {
    namespace smt {

        //This is the pattern used for file path separation
        const string PATH_SEPARATION_SYMBOLS = "/\\";
        //This is a delimiter used in the test files
        const char TOKEN_DELIMITER_CHAR = ' ';
        //The expected number of program arguments
        const uint32_t EXPECTED_NUMBER_OF_ARGUMENTS = 3;
        //The expected number of user arguments
        const uint32_t EXPECTED_USER_NUMBER_OF_ARGUMENTS = (EXPECTED_NUMBER_OF_ARGUMENTS - 1);
        //The number of bytes in one Mb
        const uint32_t BYTES_ONE_MB = 1024u;

        namespace logging {

            //This enumeration stores all the available logging levels.

            enum DebugLevelsEnum {
                USAGE = 0, ERROR = USAGE + 1, WARNING = ERROR + 1, RESULT = WARNING + 1,
                INFO = RESULT + 1, INFO1 = INFO + 1, INFO2 = INFO1 + 1, INFO3 = INFO2 + 1,
                DEBUG = INFO3 + 1, DEBUG1 = DEBUG + 1, DEBUG2 = DEBUG1 + 1, DEBUG3 = DEBUG2 + 1,
                DEBUG4 = DEBUG3 + 1
            };

            //Defines the maximum logging level
            static const DebugLevelsEnum LOGER_MAX_LEVEL = DEBUG3;

            //Defines the log level from which the detailed timing info is available
            static const DebugLevelsEnum PROGRESS_ACTIVE_LEVEL = INFO2;
        }

        //The following type definitions are important for storing the Tries information
        namespace tries {
            //This typedef if used in the tries in order to specify the type of the N-gram level NcontextHash
            typedef uint16_t TModelLevel;

            //Define the undefined N-gram level
            const TModelLevel UNDEF_NGRAM_LEVEL = 0u;
            //Define the minimum possible N-gram level
            const TModelLevel MIN_NGRAM_LEVEL = 1u;
            //The considered maximum length of the N-gram
            const TModelLevel MAX_NGRAM_LEVEL = 5u;

            //Enables all sorts of internal sanity checks,
            //e.g. sets the collision detection on and off.
            const bool DO_SANITY_CHECKS = true;

            namespace __HashMapWordIndex {
                //The unordered map memory factor for the Word index in AHashMapTrie
                const float UM_WORD_INDEX_MEMORY_FACTOR = 2.6;
            }

            namespace __CtxMultiHashMapTrie {
                //The unordered map memory factor for the One-Grams in ContextMultiHashMapTrie
                const float UM_O_GRAM_MEMORY_FACTOR = 2.6;
                //The unordered map memory factor for the M-Grams in ContextMultiHashMapTrie
                const float UM_M_GRAM_MEMORY_FACTOR = 2.0;
                //The unordered map memory factor for the N-Grams in ContextMultiHashMapTrie
                const float UM_N_GRAM_MEMORY_FACTOR = 2.5;
            }

            namespace __CtxToPBUMapStorageFactory {
                //The unordered map memory factor for the unordered maps in CtxToPBMapStorage
                const float UM_CTX_TO_PB_MAP_STORE_MEMORY_FACTOR = 5.0;
            }

            namespace __W2COrderedArrayTrie {
                //Stores the procent of the memory that will be allocated per word data 
                //storage in one Trie level relative to the estimated number of needed data
                const float INIT_MEM_ALLOC_PRCT = 0.5;
                //Stores the maximum-initial memory increase for the case of more memory
                //needed for word data storage in one Trie level relative to the already
                //allocated amount of data
                const float MAX_MEM_INC_PRCT = 0.1;

                //Stores the minimum capacity increase in number of elements, must be >= 1!!!
                const size_t MIN_MEM_INC_NUM = 1;

                //Stores the possible memory increase types
                enum MemIncTypesEnum {
                    CONSTANT = 0, LINEAR = CONSTANT + 1, LOG_2 = LINEAR + 1, LOG_10 = LOG_2 + 1
                };

                //This constant stores true or false. If the value is true then the log2
                //based memory increase strategy is used, otherwise it is log10 base.
                //For log10 the percentage of memory increase drops slower than for log2
                //with the growth of the #number of already allocated elements
                const MemIncTypesEnum MEM_INC_TYPE = MemIncTypesEnum::CONSTANT;
            }
        }

        //The following type definitions are important for creating hashes
        namespace hashing {
            //This is the small id type to be used for e.g. word ids
            typedef uint32_t TShortId;
            //The maximum word index/id value
            const static TShortId MAX_SHORT_ID_VALUE = UINT32_MAX;
            //This is the long id type to be used for e.g. long context ids 
            typedef uint64_t TLongId;

            //Combine two short ids into one long id in a bit fashion
#define TShortId_TShortId_2_TLongId(first, second) (((TLongId) first) << 32) | second
        }
    }
}

#endif	/* GLOBALS_HPP */

