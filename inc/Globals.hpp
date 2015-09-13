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

        // Check windows
#if _WIN32 || _WIN64
#if _WIN64
#define ENVIRONMENT64
#else
#define ENVIRONMENT32
#endif
#endif

        // Check GCC
#if __GNUC__
#if __x86_64__ || __ppc64__
#define ENVIRONMENT64
#else
#define ENVIRONMENT32
#endif
#endif

        //This is the pattern used for file path separation
        const string PATH_SEPARATION_SYMBOLS = "/\\";
        //This is a delimiter used in the test files
        const char TOKEN_DELIMITER_CHAR = ' ';
        //The expected number of program arguments
        const uint32_t EXPECTED_NUMBER_OF_ARGUMENTS = 4;
        //The expected number of user arguments
        const uint32_t EXPECTED_USER_NUMBER_OF_ARGUMENTS = (EXPECTED_NUMBER_OF_ARGUMENTS - 1);
        //The number of bytes in one Mb
        const uint32_t BYTES_ONE_MB = 1024u;

        namespace logging {

            /**
             * This enumeration stores all the available logging levels.
             */
            enum DebugLevelsEnum {
                USAGE = 0, ERROR = USAGE + 1, WARNING = ERROR + 1, RESULT = WARNING + 1,
                INFO = RESULT + 1, INFO1 = INFO + 1, INFO2 = INFO1 + 1, INFO3 = INFO2 + 1,
                DEBUG = INFO3 + 1, DEBUG1 = DEBUG + 1, DEBUG2 = DEBUG1 + 1, DEBUG3 = DEBUG2 + 1,
                DEBUG4 = DEBUG3 + 1, size = DEBUG4 + 1
            };

            //Defines the maximum logging level
            static const DebugLevelsEnum LOGER_MAX_LEVEL = INFO3;

            //Defines the log level from which the detailed timing info is available
            static const DebugLevelsEnum PROGRESS_ACTIVE_LEVEL = INFO2;
        }

        //The following type definitions are important for storing the Tries information
        namespace tries {

            //Enables all sorts of internal sanity checks,
            //e.g. sets the collision detection on and off.
            const bool DO_SANITY_CHECKS = false;

            //This typedef if used in the tries in order to specify the type of the N-gram level NcontextHash
            typedef uint8_t TModelLevel;

            //The type used for storing log probabilities and back-off values
            typedef float TLogProbBackOff;

            //The zero value for back-off weight
            const TLogProbBackOff ZERO_BACK_OFF_WEIGHT = 0.0f;
            //The zero value for probability weight
            const TLogProbBackOff ZERO_PROBABILITY_WEIGHT = 0.0f;
            //The zero like value for log probability weight
            const TLogProbBackOff ZERO_LOG_PROB_WEIGHT = -100.0f;

            //The base of the logarithm for stored probabilities/back-off weights
            const TLogProbBackOff LOG_PROB_WEIGHT_BASE = 10.0;
            //The value indicating an undefined probability/back-off weight
            const TLogProbBackOff UNDEF_LOG_PROB_WEIGHT = 100.0f;
            //The value of the minimal probability weight
            const TLogProbBackOff MIN_LOG_PROB_WEIGHT = -10.0f;
            //The value of the unknown word probability weight
            const TLogProbBackOff UNK_WORD_LOG_PROB_WEIGHT = ZERO_LOG_PROB_WEIGHT;

            //The considered maximum length of the N-gram 
            const static TModelLevel M_GRAM_LEVEL_MAX = 5u;

            namespace alloc {

                //Stores the possible memory increase types

                enum MemIncTypesEnum {
                    UNDEFINED = 0, CONSTANT = UNDEFINED + 1, LINEAR = CONSTANT + 1, LOG_2 = LINEAR + 1,
                    LOG_10 = LOG_2 + 1, size = LOG_10 + 1
                };
            }

            namespace dictionary {

                //Stores the possible Word index configurations

                enum WordIndexTypesEnum {
                    UNDEFINED = 0, BASIC_WORD_INDEX = UNDEFINED + 1,
                    COUNTING_WORD_INDEX = BASIC_WORD_INDEX + 1, size = COUNTING_WORD_INDEX + 1
                };
            }
            using namespace dictionary;

            namespace __HashMapWordIndex {
                //The unordered map memory factor for the Word index in AHashMapTrie
                static const float UM_WORD_INDEX_MEMORY_FACTOR = 2.6;
            }

            namespace __C2DHybridTrie {
                //The unordered map memory factor for the M-Grams in C2DMapArrayTrie
                static const float UM_M_GRAM_MEMORY_FACTOR = 2.1;
                //The unordered map memory factor for the N-Grams in C2DMapArrayTrie
                static const float UM_N_GRAM_MEMORY_FACTOR = 2.0;
                //Stores the word index type to be used in this trie, the COUNTING
                //index does not seem to give any performance improvements
                static const WordIndexTypesEnum WORD_INDEX_TYPE = BASIC_WORD_INDEX;
            }

            namespace __C2DMapTrie {
                //The unordered map memory factor for the M-Grams in CtxMultiHashMapTrie
                static const float UM_M_GRAM_MEMORY_FACTOR = 2.0;
                //The unordered map memory factor for the N-Grams in CtxMultiHashMapTrie
                static const float UM_N_GRAM_MEMORY_FACTOR = 2.5;
                //Stores the word index type to be used in this trie, the COUNTING
                //index does not seem to give any performance improvements
                static const WordIndexTypesEnum WORD_INDEX_TYPE = BASIC_WORD_INDEX;
            }

            namespace __G2DMapTrie {
                //Stores the memory increment factor, the number we will multiply by the computed increment
                static const float MEM_INC_FACTOR = 0.3;

                //Stores the minimum capacity increase in number of elements, must be >= 1!!!
                static const size_t MIN_MEM_INC_NUM = 1;

                //This constant stores true or false. If the value is true then the log2
                //based memory increase strategy is used, otherwise it is log10 base.
                //For log10 the percentage of memory increase drops slower than for log2
                //with the growth of the #number of already allocated elements
                static const alloc::MemIncTypesEnum MEM_INC_TYPE = alloc::MemIncTypesEnum::LOG_2;

                //This is the factor that is used to define an average number of words
                //per buckets in G2DHashMapTrie. I.e. the number of buckets per trie
                //level is defined as the number of M-grams in this level divided by
                //this factor value 
                static const float WORDS_PER_BUCKET_FACTOR = 1.0;

                //Stores the word index type to be used in this trie, COUNTING
                //index is a must to save memory for gram ids!
                static const WordIndexTypesEnum WORD_INDEX_TYPE = COUNTING_WORD_INDEX;
            }

            namespace __W2CArrayTrie {
                //In case set to true will pre-allocate memory per word for storing contexts
                //This can speed up the filling in of the trie but at the same time it can
                //have a drastic effect on RSS - the maximum RSS can grow significantly
                static const bool PRE_ALLOCATE_MEMORY = false;
                //Stores the percent of the memory that will be allocated per word data 
                //storage in one Trie level relative to the estimated number of needed data
                static const float INIT_MEM_ALLOC_PRCT = 0.5;

                //Stores the memory increment factor, the number we will multiply by the computed increment
                static const float MEM_INC_FACTOR = 0.3;

                //Stores the minimum capacity increase in number of elements, must be >= 1!!!
                static const size_t MIN_MEM_INC_NUM = 1;

                //This constant stores true or false. If the value is true then the log2
                //based memory increase strategy is used, otherwise it is log10 base.
                //For log10 the percentage of memory increase drops slower than for log2
                //with the growth of the #number of already allocated elements
                static const alloc::MemIncTypesEnum MEM_INC_TYPE = alloc::MemIncTypesEnum::LOG_2;

                //Stores the word index type to be used in this trie, the  COUNTING
                //index gives a bit faster querying.
                static const WordIndexTypesEnum WORD_INDEX_TYPE = COUNTING_WORD_INDEX;
            }

            namespace __C2WArrayTrie {

                //Stores the word index type to be used in this trie, the COUNTING
                //index gives a bit faster querying.
                static const WordIndexTypesEnum WORD_INDEX_TYPE = COUNTING_WORD_INDEX;
            }

            namespace __W2CHybridTrie {
                //The unordered map memory factor for the unordered maps in CtxToPBMapStorage
                static const float UM_CTX_TO_PB_MAP_STORE_MEMORY_FACTOR = 5.0;

                //Stores the word index type to be used in this trie, the COUNTING
                //index gives a bit faster querying.
                static const WordIndexTypesEnum WORD_INDEX_TYPE = COUNTING_WORD_INDEX;
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
            //The signed version of the long id, is used in binary searches and such
            typedef int64_t TSLongId;

            //Combine two short ids into one long id in a bit fashion
#define TShortId_TShortId_2_TLongId(first, second) ((((TLongId) first) << 32) | second)
        }
    }
}

#endif	/* GLOBALS_HPP */

