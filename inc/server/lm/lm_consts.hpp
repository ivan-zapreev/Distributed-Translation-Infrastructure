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

using namespace std;

namespace uva {
    namespace smt {

        //As the m-gram level is a template parameter, some template instances will violate 
        //the array index constraint. These templates will not be used @ runtime but we
        //need to disable these warnings in order to be able to build the code.
#pragma GCC diagnostic ignored "-Warray-bounds"

        namespace bpbd {
            namespace server {
                namespace lm {

                    //This namespace stores types and constants for the m-gram and context identifiers.
                    namespace identifiers {
                        //This is the small id type to be used for e.g. word ids
                        typedef uint32_t TShortId;
                        //This is the long id type to be used for e.g. long context ids 
                        typedef uint64_t TLongId;
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

