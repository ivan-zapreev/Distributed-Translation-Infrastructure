/* 
 * File:   lm_query.cpp
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
#include <math.h>       //std::pow

#include <stdexcept>
#include <execinfo.h>

#include "tclap/CmdLine.h"

#include "Exceptions.hpp"
#include "StatisticsMonitor.hpp"
#include "Logger.hpp"
#include "StringUtils.hpp"

#include "AFileReader.hpp"
#include "MemoryMappedFileReader.hpp"
#include "FileStreamReader.hpp"
#include "CStyleFileReader.hpp"

#include "Executor.hpp"

using namespace std;
using namespace TCLAP;
using namespace uva::smt;
using namespace uva::smt::tries;
using namespace uva::smt::tries::dictionary;
using namespace uva::utils::file;
using namespace uva::utils::logging;
using namespace uva::utils::text;
using namespace uva::utils::exceptions;

//Declare the program version string
#define PROGRAM_VERSION_STR "1.1"

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

//Declare the maximum stack trace depth
#define MAX_STACK_TRACE_LEN 100

/**
 * This functions does nothing more but printing the program header information
 */
static void print_info() {
    LOG_USAGE << " ------------------------------------------------------------------ " << END_LOG;
    LOG_USAGE << "|                 Back Off Language Model(s) for SMT     :)\\___/(: |" << END_LOG;
    LOG_USAGE << "|                       Software version " << PROGRAM_VERSION_STR << "             {(@)v(@)} |" << END_LOG;
    LOG_USAGE << "|                         The Owl release.               {|~- -~|} |" << END_LOG;
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

//The pointer to the command line parameters parser
static CmdLine * p_cmd_args = NULL;
static ValueArg<string> * p_model_arg = NULL;
static ValueArg<string> * p_query_arg = NULL;
static vector<string> trie_types;
static ValuesConstraint<string> * p_trie_types_constr = NULL;
static ValueArg<string> * p_trie_type_arg = NULL;
static SwitchArg * p_cumulative_prob_arg = NULL;
static vector<string> debug_levels;
static ValuesConstraint<string> * p_debug_levels_constr = NULL;
static ValueArg<string> * p_debug_level_arg = NULL;

/**
 * Creates and sets up the command line parameters parser
 */
void create_arguments_parser() {
    //Declare the command line arguments parser
    p_cmd_args = new CmdLine("", ' ', PROGRAM_VERSION_STR);

    //Add the -m the input language model file parameter - compulsory
    p_model_arg = new ValueArg<string>("m", "model", "A back-off language model file name in ARPA format", true, "", "model file name", *p_cmd_args);

    //Add the -q the input test queries file parameter - compulsory 
    p_query_arg = new ValueArg<string>("q", "query", "A text file containing new line separated M-gram queries", true, "", "query file name", *p_cmd_args);

    //Add the -t the trie type parameter - optional, default is one of the tries (e.g. c2wa)
    __Executor::get_trie_types_str(&trie_types);
    p_trie_types_constr = new ValuesConstraint<string>(trie_types);
    p_trie_type_arg = new ValueArg<string>("t", "trie", "The trie type to be used", false, __Executor::get_default_trie_type_str(), p_trie_types_constr, *p_cmd_args);

    //Add the -c the "cumulative" probability switch - optional, default is cumulative
    p_cumulative_prob_arg = new SwitchArg("c", "cumulative", "Compute the sum of cumulative log probabilities for each query m-gram", *p_cmd_args, false);

    //Add the -d the debug level parameter - optional, default is e.g. RESULT
    Logger::get_reporting_levels(&debug_levels);
    p_debug_levels_constr = new ValuesConstraint<string>(debug_levels);
    p_debug_level_arg = new ValueArg<string>("d", "debug", "The debug level to be used", false, RESULT_PARAM_VALUE, p_debug_levels_constr, *p_cmd_args);
}

#define SAFE_DESTROY(ptr) \
    if (ptr != NULL) { \
        delete ptr; \
        ptr = NULL; \
    }

/**
 * Allows to deallocate the parameters parser if it is needed
 */
void destroy_arguments_parser() {
    SAFE_DESTROY(p_model_arg);
    SAFE_DESTROY(p_query_arg);

    SAFE_DESTROY(p_trie_types_constr);
    SAFE_DESTROY(p_trie_type_arg);

    SAFE_DESTROY(p_cumulative_prob_arg);

    SAFE_DESTROY(p_debug_levels_constr);
    SAFE_DESTROY(p_debug_level_arg);

    SAFE_DESTROY(p_cmd_args);
}

/**
 * This function tries to extract the 
 * @param argc the number of program arguments
 * @param argv the array of program arguments
 * @param params the structure that will be filled in with the parsed program arguments
 */
static void extract_arguments(const uint argc, char const * const * const argv, __Executor::TExecutionParams & params) {
    //Parse the arguments
    try {
        p_cmd_args->parse(argc, argv);
    } catch (ArgException &e) {
        stringstream msg;
        msg << "Error: " << e.error() << ", for argument: " << e.argId();
        throw Exception(msg.str());
    }

    //Store the parsed parameter values
    params.is_cumulative_prob = p_cumulative_prob_arg->getValue();
    params.m_model_file_name = p_model_arg->getValue();
    params.m_queries_file_name = p_query_arg->getValue();
    params.m_trie_type_name = p_trie_type_arg->getValue();

    //Set the logging level right away
    Logger::set_reporting_level(p_debug_level_arg->getValue());
}

/**
 * The uncaught exceptions handler
 */
void handler() {
    void *trace_elems[20];
    int trace_elem_count(backtrace(trace_elems, MAX_STACK_TRACE_LEN));
    char **stack_syms(backtrace_symbols(trace_elems, trace_elem_count));
    LOG_ERROR << "Ooops, Sorry! Something terrible has happened, we crashed!" << END_LOG;
    for (int i = 0; i < trace_elem_count; ++i) {
        LOG_ERROR << stack_syms[i] << END_LOG;
    }
    free(stack_syms);
    exit(1);
}

/**
 * The main program entry point
 */
int main(int argc, char** argv) {
    //Declare the return code
    int returnCode = 0;

    //Set the uncaught exception handler
    std::set_terminate(handler);

    //First print the program info
    print_info();

    //Set up possible program arguments
    create_arguments_parser();

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
        returnCode = 1;
    }

    //Destroy the command line parameters parser
    destroy_arguments_parser();

    return returnCode;
}
