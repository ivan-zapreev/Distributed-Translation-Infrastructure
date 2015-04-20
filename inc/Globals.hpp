/* 
 * File:   Globals.hpp
 * Author: zapreevis
 *
 * Created on April 20, 2015, 8:58 PM
 */

#ifndef GLOBALS_HPP
#define	GLOBALS_HPP

//This is the pattern used for file path separation
#define PATH_SEPARATION_SYMBOLS "/\\"
//This is a delimiter used in the text corpus and test files
#define TOKEN_DELIMITER_CHAR ' '
//The expected number of program arguments
#define EXPECTED_NUMBER_OF_ARGUMENTS 3
//The expected number of user arguments
#define EXPECTED_USER_NUMBER_OF_ARGUMENTS (EXPECTED_NUMBER_OF_ARGUMENTS - 1)
//The number of bytes in one Mb
#define BYTES_ONE_MB 1024
//The considered maximum length of the N-gram
#define N_GRAM_PARAM 5u

//The command line option values for debug levels
#define INFO_PARAM_VALUE "info"
#define DEBUG_PARAM_VALUE "debug"
#define DEBUG_OPTION_VALUES "{" INFO_PARAM_VALUE ", " DEBUG_PARAM_VALUE "}"

typedef unsigned short int TTrieSize;

#endif	/* GLOBALS_HPP */

