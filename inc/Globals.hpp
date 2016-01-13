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

#include "Configuration.hpp"

using namespace std;

//Declare the maximum stack trace depth
#define MAX_STACK_TRACE_LEN 100

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

        //As the m-gram level is a template parameter, some template instances will violate 
        //the array index constraint. These templates will not be used @ runtime but we
        //need to disable these warnings in order to be able to build the code.
#pragma GCC diagnostic ignored "-Warray-bounds"

        //The following type definitions are important for storing the Tries information
        namespace tries {

            //This typedef if used in the tries in order to specify the type of the N-gram level NcontextHash
            typedef uint8_t TModelLevel;

            //The type used for storing log probabilities and back-off values
            typedef float TLogProbBackOff;

            //The zero value for back-off weight
            const TLogProbBackOff ZERO_BACK_OFF_WEIGHT = 0.0f;
            //The zero value for probability weight
            const TLogProbBackOff ZERO_PROB_WEIGHT = 0.0f;
            //The zero like value for log probability weight
            const TLogProbBackOff ZERO_LOG_PROB_WEIGHT = -100.0f;

            //The base of the logarithm for stored probabilities/back-off weights
            const TLogProbBackOff LOG_PROB_WEIGHT_BASE = 10.0;
            //The value indicating an undefined probability/back-off weight
            const TLogProbBackOff UNDEF_LOG_PROB_WEIGHT = 100.0f;
            //The default value of the unknown word probability weight
            const TLogProbBackOff UNK_WORD_LOG_PROB_WEIGHT = -10.0f;

            //The following type definitions are important for creating hashes
            namespace identifiers {
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

            namespace m_grams {
                //Various M-gram levels
                const static TModelLevel M_GRAM_LEVEL_UNDEF = 0u; //MUST BE ZERO
                const static TModelLevel M_GRAM_LEVEL_1 = 1u;
                const static TModelLevel M_GRAM_LEVEL_2 = 2u;
                const static TModelLevel M_GRAM_LEVEL_3 = 3u;
                const static TModelLevel M_GRAM_LEVEL_4 = 4u;
                const static TModelLevel M_GRAM_LEVEL_5 = 5u;
                const static TModelLevel M_GRAM_LEVEL_6 = 6u;
                const static TModelLevel M_GRAM_LEVEL_7 = 7u;
                const static TModelLevel M_GRAM_LEVEL_8 = 8u;


                //Stores the maximum sopported level
                static constexpr TModelLevel MAX_SUPP_GRAM_LEVEL = M_GRAM_LEVEL_6;

                //This structure stores the current level value mapping from the [begin,end] value pair
                static constexpr TModelLevel CURR_LEVEL_MAP[MAX_SUPP_GRAM_LEVEL][MAX_SUPP_GRAM_LEVEL] = {
                    {M_GRAM_LEVEL_1, M_GRAM_LEVEL_2, M_GRAM_LEVEL_3, M_GRAM_LEVEL_4, M_GRAM_LEVEL_5, M_GRAM_LEVEL_6},
                    {M_GRAM_LEVEL_UNDEF, M_GRAM_LEVEL_1, M_GRAM_LEVEL_2, M_GRAM_LEVEL_3, M_GRAM_LEVEL_4, M_GRAM_LEVEL_5},
                    {M_GRAM_LEVEL_UNDEF, M_GRAM_LEVEL_UNDEF, M_GRAM_LEVEL_1, M_GRAM_LEVEL_2, M_GRAM_LEVEL_3, M_GRAM_LEVEL_4},
                    {M_GRAM_LEVEL_UNDEF, M_GRAM_LEVEL_UNDEF, M_GRAM_LEVEL_UNDEF, M_GRAM_LEVEL_1, M_GRAM_LEVEL_2, M_GRAM_LEVEL_3},
                    {M_GRAM_LEVEL_UNDEF, M_GRAM_LEVEL_UNDEF, M_GRAM_LEVEL_UNDEF, M_GRAM_LEVEL_UNDEF, M_GRAM_LEVEL_1, M_GRAM_LEVEL_2},
                    {M_GRAM_LEVEL_UNDEF, M_GRAM_LEVEL_UNDEF, M_GRAM_LEVEL_UNDEF, M_GRAM_LEVEL_UNDEF, M_GRAM_LEVEL_UNDEF, M_GRAM_LEVEL_1},
                };

                //This structure stores the current level minus 1 value mapping from the [begin,end] value pair
                static constexpr TModelLevel CURR_LEVEL_MIN_1_MAP[MAX_SUPP_GRAM_LEVEL][MAX_SUPP_GRAM_LEVEL] = {
                    {0, 1, 2, 3, 4, 5},
                    {0, 0, 1, 2, 3, 4},
                    {0, 0, 0, 1, 2, 3},
                    {0, 0, 0, 0, 1, 2},
                    {0, 0, 0, 0, 0, 1},
                    {0, 0, 0, 0, 0, 0},
                };

                //This structure stores the current level minus 2 value mapping from the [begin,end] value pair
                static constexpr TModelLevel CURR_LEVEL_MIN_2_MAP[MAX_SUPP_GRAM_LEVEL][MAX_SUPP_GRAM_LEVEL] = {
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

#endif	/* GLOBALS_HPP */

