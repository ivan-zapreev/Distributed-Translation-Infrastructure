/* 
 * File:   Configuration.hpp
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
 * Created on September 18, 2015, 8:54 AM
 */

#ifndef CONFIGURATION_HPP
#define	CONFIGURATION_HPP

#include <inttypes.h>
#include <string>

namespace uva {
    namespace smt {
        namespace logging {

            /**
             * This enumeration stores all the available logging levels.
             */
            enum DebugLevelsEnum {
                ERROR = 0, WARNING = ERROR + 1, USAGE = WARNING + 1, RESULT = USAGE + 1,
                INFO = RESULT + 1, INFO1 = INFO + 1, INFO2 = INFO1 + 1, INFO3 = INFO2 + 1,
                DEBUG = INFO3 + 1, DEBUG1 = DEBUG + 1, DEBUG2 = DEBUG1 + 1, DEBUG3 = DEBUG2 + 1,
                DEBUG4 = DEBUG3 + 1, size = DEBUG4 + 1
            };

            //Defines the maximum logging level
            static const DebugLevelsEnum LOGER_MAX_LEVEL = INFO3;

            //Defines the log level from which the detailed timing info is available
            static const DebugLevelsEnum PROGRESS_ACTIVE_LEVEL = INFO2;

            //Enables all sorts of internal sanity checks,
            //e.g. sets the collision detection on and off.
            const bool DO_SANITY_CHECKS = false;
        }

        //The following type definitions are important for storing the Tries information
        namespace tries {

            //The considered maximum length of the N-gram 
            const static uint8_t M_GRAM_LEVEL_MAX = 5u;

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
                    UNDEFINED_WORD_INDEX = 0,
                    BASIC_WORD_INDEX = UNDEFINED_WORD_INDEX + 1,
                    COUNTING_WORD_INDEX = BASIC_WORD_INDEX + 1,
                    OPTIMIZING_BASIC_WORD_INDEX = COUNTING_WORD_INDEX + 1,
                    OPTIMIZING_COUNTING_WORD_INDEX = OPTIMIZING_BASIC_WORD_INDEX + 1,
                    HASHING_WORD_INDEX = OPTIMIZING_COUNTING_WORD_INDEX + 1,
                    size_word_index = HASHING_WORD_INDEX + 1
                };

                namespace __HashMapWordIndex {
                    //The unordered map memory factor for the Word index in AHashMapTrie
                    static const float MEMORY_FACTOR = 2.6;
                }

                namespace __OptimizingWordIndex {
                    //This is the number of buckets factor for the optimizing word index
                    //The number of buckets will be the number of words * this value
                    static const float BUCKETS_FACTOR = 10.0;
                }
            }
            using namespace dictionary;

            namespace __BitmapHashCache {
                //The default number of buckets allocated for hash is equal to the
                //number of M-grams in the Trie level. This is absolutely not enough
                //As then the cache will be fully used and there will be only performance
                //losses. To make the cache work we need much more buckets so that the
                //queried M-grams have a very low chance to fall into the buckets with
                //the Trie grams. So the bigger this number the better, yet the
                //memory constraints. ALthough they are not crucial as we use bitmaps.
                //NOTE: The experiments with C2WA showed a 5% performance improvement
                //In the range of values 15-50. Yet, looking at memory consumption the 
                //optimum value was chosen to be 20 as with 15 it starts deteriorating 
                static const float BUCKET_MULTIPLIER_FACTOR = 20;
            }

            //Stores the possible Trie types

            enum TrieTypesEnum {
                UNDEFINED_TRIE = 0,
                C2DH_TRIE = UNDEFINED_TRIE + 1,
                C2DM_TRIE = C2DH_TRIE + 1,
                G2DM_TRIE = C2DM_TRIE + 1,
                W2CA_TRIE = G2DM_TRIE + 1,
                C2WA_TRIE = W2CA_TRIE + 1,
                W2CH_TRIE = C2WA_TRIE + 1,
                H2DM_TRIE = W2CH_TRIE + 1,
                size_trie = H2DM_TRIE + 1
            };

            namespace __C2DHybridTrie {
                //The unordered map memory factor for the M-Grams in C2DMapArrayTrie
                static const float UM_M_GRAM_MEMORY_FACTOR = 2.1;
                //The unordered map memory factor for the N-Grams in C2DMapArrayTrie
                static const float UM_N_GRAM_MEMORY_FACTOR = 2.0;
                //Stores the word index type to be used in this trie, the COUNTING
                //index does not seem to give any performance improvements. The optimizing
                //word index gives about 10% performance improvement!
                static const WordIndexTypesEnum WORD_INDEX_TYPE = OPTIMIZING_BASIC_WORD_INDEX;
                //This flag is to enable/disable the bitmap cache hashing in this Trie
                static const bool DO_BITMAP_HASH_CACHE = true;
            }

            namespace __C2DMapTrie {
                //The unordered map memory factor for the M-Grams in CtxMultiHashMapTrie
                static const float UM_M_GRAM_MEMORY_FACTOR = 2.0;
                //The unordered map memory factor for the N-Grams in CtxMultiHashMapTrie
                static const float UM_N_GRAM_MEMORY_FACTOR = 2.5;
                //Stores the word index type to be used in this trie, the COUNTING
                //index does not seem to give any performance improvements. The optimizing
                //word index gives about 10% performance improvement!
                static const WordIndexTypesEnum WORD_INDEX_TYPE = OPTIMIZING_BASIC_WORD_INDEX;
                //This flag is to enable/disable the bitmap cache hashing in this Trie
                //The experiments show that with 20*bitmap cache this is about 5% faster
                static const bool DO_BITMAP_HASH_CACHE = true;
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
                static const float WORDS_PER_BUCKET_FACTOR = 0.1;
                //Stores the word index type to be used in this trie, COUNTING
                //index is a must to save memory for gram ids! The optimizing
                //word index gives about 10% performance improvement!
                static const WordIndexTypesEnum WORD_INDEX_TYPE = OPTIMIZING_COUNTING_WORD_INDEX;
                //This flag is to enable/disable the bitmap cache hashing in this Trie
                //The experiments show that with 20*bitmap cache this is about 5% faster
                static const bool DO_BITMAP_HASH_CACHE = true;
            }

            namespace __H2DMapTrie {
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
                static const float WORDS_PER_BUCKET_FACTOR = 10;
                //Stores the word index type to be used in this trie, COUNTING
                //index is a must to save memory for gram ids! The optimizing
                //word index gives about 10% performance improvement!
                static const WordIndexTypesEnum WORD_INDEX_TYPE = HASHING_WORD_INDEX;
                //This flag is to enable/disable the bitmap cache hashing in this Trie
                //The experiments show that with 20*bitmap cache this is about 5% faster
                static const bool DO_BITMAP_HASH_CACHE = true;
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
                //index gives about 5% faster faster querying. The optimizing
                //word index gives about 10% performance improvement!
                static const WordIndexTypesEnum WORD_INDEX_TYPE = OPTIMIZING_COUNTING_WORD_INDEX;
                //This flag is to enable/disable the bitmap cache hashing in this Trie
                //The experiments show that with 20*bitmap cache this is about 5% faster
                static const bool DO_BITMAP_HASH_CACHE = true;
            }

            namespace __C2WArrayTrie {
                //Stores the word index type to be used in this trie, the COUNTING
                //index gives about 5% faster faster querying. The optimizing
                //word index gives about 10% performance improvement!
                static const WordIndexTypesEnum WORD_INDEX_TYPE = OPTIMIZING_COUNTING_WORD_INDEX;
                //This flag is to enable/disable the bitmap cache hashing in this Trie
                //The experiments show that with 20*bitmap cache this is about 5% faster
                static const bool DO_BITMAP_HASH_CACHE = true;
            }

            namespace __W2CHybridTrie {
                //The unordered map memory factor for the unordered maps in CtxToPBMapStorage
                static const float UM_CTX_TO_PB_MAP_STORE_MEMORY_FACTOR = 5.0;

                //Stores the word index type to be used in this trie, the COUNTING
                //index gives about 5% faster faster querying. The optimizing
                //word index gives about 10% performance improvement!
                static const WordIndexTypesEnum WORD_INDEX_TYPE = OPTIMIZING_COUNTING_WORD_INDEX;
                //This flag is to enable/disable the bitmap cache hashing in this Trie
                static const bool DO_BITMAP_HASH_CACHE = true;
            }
        }
    }
}


#endif	/* CONFIGURATION_HPP */

