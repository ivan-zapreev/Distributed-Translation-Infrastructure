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

#ifndef LM_CONSTS_HPP
#define LM_CONSTS_HPP

#include <inttypes.h>
#include <string>

#include "server/server_configs.hpp"

#include "common/utils/containers/dynamic_memory_arrays.hpp"

using namespace std;

using namespace uva::utils::containers;

namespace uva {
    namespace smt {

        //As the m-gram level is a template parameter, some template instances will violate 
        //the array index constraint. These templates will not be used @ runtime but we
        //need to disable these warnings in order to be able to build the code.
#pragma GCC diagnostic ignored "-Warray-bounds"

        namespace bpbd {
            namespace server {
                namespace lm {

                    namespace dictionary {

                        namespace __AWordIndex {
                            //The memory factor for any existing word index ....
                            static constexpr float MEMORY_FACTOR = 2.6;
                        }

                        namespace __optimizing_word_index {
                            //This is the number of buckets factor for the optimizing word index. The 
                            //number of buckets will be proportional the number of words * this value
                            static constexpr double BUCKETS_FACTOR = 2.0;
                        }
                    }

                    //This namespace stores types and constants for the m-gram and context identifiers.
                    //ToDo: Go trough all the LM and change from TShordId and
                    //      TLongId to phrase_uid, word_uid and basic types!
                    namespace identifiers {
                        //This is the small id type to be used for e.g. word ids
                        typedef uint32_t TShortId;
                        //This is the long id type to be used for e.g. long context ids 
                        typedef uint64_t TLongId;
                    }

                    namespace __C2DHybridTrie {
                        //The unordered map memory factor for the M-Grams in C2DMapArrayTrie
                        static constexpr float UM_M_GRAM_MEMORY_FACTOR = 2.1;
                        //The unordered map memory factor for the N-Grams in C2DMapArrayTrie
                        static constexpr float UM_N_GRAM_MEMORY_FACTOR = 2.0;
                        //Stores the word index type to be used in this trie, the COUNTING
                        //index does not seem to give any performance improvements. The optimizing
                        //word index gives about 10% performance improvement!
                        //static constexpr word_index_types WORD_INDEX_TYPE = OPTIMIZING_BASIC_WORD_INDEX;
                        //With the bitmap hashing we get some 5% performance improvement
                        static constexpr uint8_t BITMAP_HASH_CACHE_BUCKETS_FACTOR = 10;
                    }

                    namespace __C2DMapTrie {
                        //The unordered map memory factor for the M-Grams in CtxMultiHashMapTrie
                        static constexpr float UM_M_GRAM_MEMORY_FACTOR = 2.0;
                        //The unordered map memory factor for the N-Grams in CtxMultiHashMapTrie
                        static constexpr float UM_N_GRAM_MEMORY_FACTOR = 2.5;
                        //Stores the word index type to be used in this trie, the COUNTING
                        //index does not seem to give any performance improvements. The optimizing
                        //word index gives about 10% performance improvement!
                        //static constexpr word_index_types WORD_INDEX_TYPE = OPTIMIZING_BASIC_WORD_INDEX;
                        //With the bitmap hash caching on we are not faster with this trie
                        static constexpr uint8_t BITMAP_HASH_CACHE_BUCKETS_FACTOR = 0;
                    }

                    namespace __G2DMapTrie {
                        //This is the factor that is used to define an average number
                        //of words per buckets in G2DHashMapTrie. The number of buckets
                        //will be proportional to the number of words * this value
                        static constexpr double BUCKETS_FACTOR = 3.0;
                        //Stores the word index type to be used in this trie, COUNTING
                        //index is a must to save memory for gram ids! The optimizing
                        //word index gives about 10% performance improvement!
                        //static constexpr word_index_types WORD_INDEX_TYPE = OPTIMIZING_COUNTING_WORD_INDEX;
                        //With the bitmap hashing we get some 5% performance improvement
                        static constexpr uint8_t BITMAP_HASH_CACHE_BUCKETS_FACTOR = 5;
                    }

                    namespace __H2DMapTrie {
                        //This is the factor that is used to define an average number of words
                        //per buckets in H2DHashMapTrie. The number of buckets
                        //will be proportional to the number of words * this value
                        static constexpr double BUCKETS_FACTOR = 2.0;
                        //Stores the word index type to be used in this trie, COUNTING
                        //index is a must to save memory for gram ids! The optimizing
                        //word index gives about 10% performance improvement!
                        //static constexpr word_index_types WORD_INDEX_TYPE = HASHING_WORD_INDEX;
                        //With the bitmap hash caching on we are not faster with this trie
                        static constexpr uint8_t BITMAP_HASH_CACHE_BUCKETS_FACTOR = 0;
                    }

                    namespace __W2CArrayTrie {
                        //In case set to true will pre-allocate memory per word for storing contexts
                        //This can speed up the filling in of the trie but at the same time it can
                        //have a drastic effect on RSS - the maximum RSS can grow significantly
                        static constexpr bool PRE_ALLOCATE_MEMORY = false;
                        //Stores the percent of the memory that will be allocated per word data 
                        //storage in one Trie level relative to the estimated number of needed data
                        static constexpr float INIT_MEM_ALLOC_PRCT = 0.5;
                        //Stores the memory increment factor, the number we will multiply by the computed increment
                        static constexpr float MEM_INC_FACTOR = 1;
                        //Stores the minimum capacity increase in number of elements, must be >= 1!!!
                        static constexpr size_t MIN_MEM_INC_NUM = 1;
                        //This constant stores true or false. If the value is true then the log2
                        //based memory increase strategy is used, otherwise it is log10 base.
                        //For log10 the percentage of memory increase drops slower than for log2
                        //with the growth of the #number of already allocated elements
                        static constexpr MemIncTypesEnum MEM_INC_TYPE = MemIncTypesEnum::LOG_2;
                        //Stores the word index type to be used in this trie, the  COUNTING
                        //index gives about 5% faster faster querying. The optimizing
                        //word index gives about 10% performance improvement!
                        //static constexpr word_index_types WORD_INDEX_TYPE = OPTIMIZING_COUNTING_WORD_INDEX;
                        //With the bitmap hashing we get some 5% performance improvement
                        static constexpr uint8_t BITMAP_HASH_CACHE_BUCKETS_FACTOR = 5;
                    }

                    namespace __C2WArrayTrie {
                        //Stores the word index type to be used in this trie, the COUNTING
                        //index gives about 5% faster faster querying. The optimizing
                        //word index gives about 10% performance improvement!
                        //static constexpr word_index_types WORD_INDEX_TYPE = OPTIMIZING_COUNTING_WORD_INDEX;
                        //With the bitmap hashing we get some 5% performance improvement
                        static constexpr uint8_t BITMAP_HASH_CACHE_BUCKETS_FACTOR = 5;
                    }

                    namespace __W2CHybridTrie {
                        //The unordered map memory factor for the unordered maps in CtxToPBMapStorage
                        static constexpr float UM_CTX_TO_PB_MAP_STORE_MEMORY_FACTOR = 5.0;

                        //Stores the word index type to be used in this trie, the COUNTING
                        //index gives about 5% faster faster querying. The optimizing
                        //word index gives about 10% performance improvement!
                        //static constexpr word_index_types WORD_INDEX_TYPE = OPTIMIZING_COUNTING_WORD_INDEX;
                        //With the bitmap hashing we get some 5% performance improvement
                        static constexpr uint8_t BITMAP_HASH_CACHE_BUCKETS_FACTOR = 10;
                    }

                    //This namespace stores m-gram levels and related constants
                    namespace m_grams {
                        //Various M-gram levels
                        const static phrase_length M_GRAM_LEVEL_UNDEF = 0u; //MUST BE ZERO
                        const static phrase_length M_GRAM_LEVEL_1 = 1u;
                        const static phrase_length M_GRAM_LEVEL_2 = 2u;
                        const static phrase_length M_GRAM_LEVEL_3 = 3u;
                        const static phrase_length M_GRAM_LEVEL_4 = 4u;
                        const static phrase_length M_GRAM_LEVEL_5 = 5u;
                        const static phrase_length M_GRAM_LEVEL_6 = 6u;
                        const static phrase_length M_GRAM_LEVEL_7 = 7u;
                        const static phrase_length M_GRAM_LEVEL_8 = 8u;

                        //Stores the maximum sopported level
                        static constexpr phrase_length MAX_SUPP_GRAM_LEVEL = M_GRAM_LEVEL_6;

                        //This structure stores the current level value mapping from the [begin,end] value pair
                        static constexpr phrase_length CURR_LEVEL_MAP[MAX_SUPP_GRAM_LEVEL][MAX_SUPP_GRAM_LEVEL] = {
                            {M_GRAM_LEVEL_1, M_GRAM_LEVEL_2, M_GRAM_LEVEL_3, M_GRAM_LEVEL_4, M_GRAM_LEVEL_5, M_GRAM_LEVEL_6},
                            {M_GRAM_LEVEL_UNDEF, M_GRAM_LEVEL_1, M_GRAM_LEVEL_2, M_GRAM_LEVEL_3, M_GRAM_LEVEL_4, M_GRAM_LEVEL_5},
                            {M_GRAM_LEVEL_UNDEF, M_GRAM_LEVEL_UNDEF, M_GRAM_LEVEL_1, M_GRAM_LEVEL_2, M_GRAM_LEVEL_3, M_GRAM_LEVEL_4},
                            {M_GRAM_LEVEL_UNDEF, M_GRAM_LEVEL_UNDEF, M_GRAM_LEVEL_UNDEF, M_GRAM_LEVEL_1, M_GRAM_LEVEL_2, M_GRAM_LEVEL_3},
                            {M_GRAM_LEVEL_UNDEF, M_GRAM_LEVEL_UNDEF, M_GRAM_LEVEL_UNDEF, M_GRAM_LEVEL_UNDEF, M_GRAM_LEVEL_1, M_GRAM_LEVEL_2},
                            {M_GRAM_LEVEL_UNDEF, M_GRAM_LEVEL_UNDEF, M_GRAM_LEVEL_UNDEF, M_GRAM_LEVEL_UNDEF, M_GRAM_LEVEL_UNDEF, M_GRAM_LEVEL_1},
                        };

                        //This structure stores the current level minus 1 value mapping from the [begin,end] value pair
                        static constexpr phrase_length CURR_LEVEL_MIN_1_MAP[MAX_SUPP_GRAM_LEVEL][MAX_SUPP_GRAM_LEVEL] = {
                            {0, 1, 2, 3, 4, 5},
                            {0, 0, 1, 2, 3, 4},
                            {0, 0, 0, 1, 2, 3},
                            {0, 0, 0, 0, 1, 2},
                            {0, 0, 0, 0, 0, 1},
                            {0, 0, 0, 0, 0, 0},
                        };

                        //This structure stores the current level minus 2 value mapping from the [begin,end] value pair
                        static constexpr phrase_length CURR_LEVEL_MIN_2_MAP[MAX_SUPP_GRAM_LEVEL][MAX_SUPP_GRAM_LEVEL] = {
                            {0, 0, 1, 2, 3, 4},
                            {0, 0, 0, 1, 2, 3},
                            {0, 0, 0, 0, 1, 2},
                            {0, 0, 0, 0, 0, 1},
                            {0, 0, 0, 0, 0, 0},
                            {0, 0, 0, 0, 0, 0},
                        };
                    }
                }
            }
        }
    }
}

#endif /* GLOBALS_HPP */

