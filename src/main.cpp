/* 
 * File:   main.cpp
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
 * Created on April 18, 2015, 11:35 AM
 */

#include <string>       // std::string
#include <iostream>     // std::cout
#include <sstream>      // std::stringstream, std::stringbuf
#include <fstream>      // std::ifstream
#include <math.h>    //std::pow

#include "Globals.hpp"
#include "Exceptions.hpp"
#include "StatisticsMonitor.hpp"
#include "Logger.hpp"
#include "StringUtils.hpp"

#include "AFileReader.hpp"
#include "MemoryMappedFileReader.hpp"
#include "FileStreamReader.hpp"
#include "CStyleFileReader.hpp"

#include "TrieDriver.hpp"
#include "Executor.hpp"

using namespace std;
using namespace uva::smt;
using namespace uva::smt::tries;
using namespace uva::smt::tries::dictionary;
using namespace uva::smt::file;
using namespace uva::smt::logging;
using namespace uva::smt::utils::text;
using namespace uva::smt::exceptions;

/**
 * This functions does nothing more but printing the program header information
 */
static void print_info() {
    LOG_USAGE << " ------------------------------------------------------------------ " << END_LOG;
    LOG_USAGE << "|                 Back Off Language Model(s) for SMT     :)\\___/(: |" << END_LOG;
    LOG_USAGE << "|                       Software version 1.0             {(@)v(@)} |" << END_LOG;
    LOG_USAGE << "|                                                        {|~- -~|} |" << END_LOG;
    LOG_USAGE << "|             Copyright (C) Dr. Ivan S Zapreev, 2015     {/^'^'^\\} |" << END_LOG;
    LOG_USAGE << "|  ═════════════════════════════════════════════════════════m-m══  |" << END_LOG;
    LOG_USAGE << "|        This software is distributed under GPL 2.0 license        |" << END_LOG;
    LOG_USAGE << "|          (GPL stands for GNU General Public License)             |" << END_LOG;
    LOG_USAGE << "|          The product comes with ABSOLUTELY NO WARRANTY.          |" << END_LOG;
    LOG_USAGE << "|   This is a free software, you are welcome to redistribute it.   |" << END_LOG;
#ifdef ENVIRONMENT64
    LOG_USAGE << "|                     Running in 64 bit mode!                      |" << END_LOG;
#else
    LOG_USAGE << "|                     Running in 32 bit mode!                      |" << END_LOG;
#endif
    LOG_USAGE << "|                 Build on: " << __DATE__ << " " << __TIME__ << "                   |" << END_LOG;
    LOG_USAGE << " ------------------------------------------------------------------ " << END_LOG;
}

/**
 * This function prints the usage information for the software
 * @param name the absolute name of the application 
 */
static void print_usage(const string name) {
    //Since we do not want the absolute file name, we need to chop off the directory prefix
    const unsigned int lastSlashBeforeFileName = name.find_last_of(PATH_SEPARATION_SYMBOLS);
    const string shortName = name.substr(lastSlashBeforeFileName + 1);

    LOG_USAGE << "Running: " << END_LOG;
    LOG_USAGE << "  " << shortName.c_str() << " <model_file> <test_file> <trie_type> [debug-level]" << END_LOG;
    LOG_USAGE << "      <model_file> - a text file containing the back-off language model." << END_LOG;
    LOG_USAGE << "                     This file is supposed to be in ARPA format, see: " << END_LOG;
    LOG_USAGE << "                          http://www.speech.sri.com/projects/srilm/manpages/ngram-format.5.html" << END_LOG;
    LOG_USAGE << "                     for more details. We also allow doe tags listed here:" << END_LOG;
    LOG_USAGE << "                          https://msdn.microsoft.com/en-us/library/office/hh378460%28v=office.14%29.aspx" << END_LOG;
    LOG_USAGE << "      <test_file>  - a text file containing test data." << END_LOG;
    LOG_USAGE << "                     The test file consists of a number of N-grams," << END_LOG;
    LOG_USAGE << "                     where each line in the file consists of one N-gram." << END_LOG;
    LOG_USAGE << "      <trie_type>  - the trie type, one of " << __Executor::get_trie_types_str() << END_LOG;
    LOG_USAGE << "     [debug-level] - the optional debug flag from " << Logger::getReportingLevels() << END_LOG;

    LOG_USAGE << "Output: " << END_LOG;
    LOG_USAGE << "    The program reads in the test lines from the <test_file>. " << END_LOG;
    LOG_USAGE << "    Each of these lines is a N-grams of the following form, e.g: " << END_LOG;
    LOG_USAGE << "       word1 word2 word3 word4 word5" << END_LOG;
    LOG_USAGE << "    For each of such N-grams the probability information is " << END_LOG;
    LOG_USAGE << "    computed, based on the data from the <model_file>. For" << END_LOG;
    LOG_USAGE << "    example, for a N-gram such as:" << END_LOG;
    LOG_USAGE << "       mortgages had lured borrowers and" << END_LOG;
    LOG_USAGE << "    the program may give the following output:" << END_LOG;
    LOG_USAGE << "        probability( mortgages had lured borrowers and ) = 0.00024" << END_LOG;
}

/**
 * This function tries to extract the 
 * @param argc the number of program arguments
 * @param argv the array of program arguments
 * @param params the structure that will be filled in with the parsed program arguments
 */
static void extract_arguments(const uint argc, char const * const * const argv, __Executor::TExecutionParams & params) {
    if (argc < EXPECTED_NUMBER_OF_ARGUMENTS) {
        stringstream msg;
        msg << "Incorrect number of arguments, expected >= " << EXPECTED_USER_NUMBER_OF_ARGUMENTS << ", got " << (argc - 1);
        throw Exception(msg.str());
    } else {
        params.m_model_file_name = argv[1];
        params.m_queries_file_name = argv[2];
        params.m_trie_type_name = argv[3];
        //Set the default reporting level information for the logger
        string errorLevelStr = RESULT_PARAM_VALUE;

        //The third argument should be a debug level, get it and try to set.
        if (argc > EXPECTED_NUMBER_OF_ARGUMENTS) {
            errorLevelStr = argv[4];
        }
        Logger::setReportingLevel(errorLevelStr);
    }
}

/**
 * The main program entry point
 */
int main(int argc, char** argv) {
    //Declare the return code
    int returnCode = 0;

    //First print the program info
    print_info();

    try {
        //Define en empty parameters structure
        __Executor::TExecutionParams params = {};

        LOG_INFO << "Checking on the program arguments ..." << END_LOG;

        //Attempt to extract the program arguments
        extract_arguments(argc, argv, params);

        LOG_DEBUG << "Checking on the provided files \'"
                << params.m_model_file_name << "\' and \'"
                << params.m_queries_file_name << "\' ..." << END_LOG;

        //Do the actual work, read the text corpse, create trie and do queries
        __Executor::perform_tasks(params);

    } catch (Exception & ex) {
        //The argument's extraction has failed, print the error message and quit
        LOG_ERROR << ex.getMessage() << END_LOG;
        print_usage((string) argv[0]);
        returnCode = 1;
    }

    return returnCode;
}
