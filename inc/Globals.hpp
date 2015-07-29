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

#include "Logger.hpp"

namespace uva {
    namespace smt {

//This is the pattern used for file path separation
#define PATH_SEPARATION_SYMBOLS "/\\"
//This is a delimiter used in the text corpus and test files
#define TOKEN_DELIMITER_CHAR ' '
//The expected number of program arguments
#define EXPECTED_NUMBER_OF_ARGUMENTS 3
//The expected number of user arguments
#define EXPECTED_USER_NUMBER_OF_ARGUMENTS (EXPECTED_NUMBER_OF_ARGUMENTS - 1)
//The number of bytes in one Mb
#define BYTES_ONE_MB 1024u
//The considered maximum length of the N-gram
#define MAX_NGRAM_LEVEL 5u
//Defined the minimum possible N-gram level
#define MIN_NGRAM_LEVEL 1u

//The string representation values for debug levels
#define USAGE_PARAM_VALUE "usage"
#define RESULT_PARAM_VALUE "result"
#define ERROR_PARAM_VALUE "error"
#define WARNING_PARAM_VALUE "warning"
#define INFO_PARAM_VALUE "info"
#define DEBUG_PARAM_VALUE "debug"
#define DEBUG1_PARAM_VALUE "debug1"
#define DEBUG2_PARAM_VALUE "debug2"

//The string representation values for debug levels
#define DEBUG_OPTION_VALUES "{" USAGE_PARAM_VALUE ", " ERROR_PARAM_VALUE ", " WARNING_PARAM_VALUE ", " RESULT_PARAM_VALUE ", " INFO_PARAM_VALUE ", " DEBUG_PARAM_VALUE ", " DEBUG1_PARAM_VALUE ", " DEBUG2_PARAM_VALUE "}"

        //The following type definitions are important for storing the Tries information
        namespace tries {
            //This typedef if used in the tries in order to specify the type of the N-gram level N
            typedef unsigned short int TModelLevel;

            //The amount of memory dedicated for storing frequency
            //WARNING: Do not use smaller size as I get overflows
            //for "unsigned short int" on a text large corpus!
            typedef unsigned int TFrequencySize;
        }

        //The following type definitions are important for creating hashes
        namespace hashing {
            //This is the smallest size which I've tested and it works for the hash without collisions
            typedef unsigned int TWordHashSize;
            //This is the hash reference size which should be twice as long as the TWordHashSize
            typedef unsigned long int TReferenceHashSize;
        }

    }
}

#endif	/* GLOBALS_HPP */

