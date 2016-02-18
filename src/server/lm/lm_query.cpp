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

#include "tclap/CmdLine.h"

#include "main.hpp"

#include "common/utils/monitore/statistics_monitore.hpp"
#include "common/utils/logging/logger.hpp"

#include "common/utils/string_utils.hpp"
#include "common/utils/exceptions.hpp"

#include "common/utils/file/afile_reader.hpp"
#include "common/utils/file/memory_mapped_file_reader.hpp"
#include "common/utils/file/file_stream_reader.hpp"
#include "common/utils/file/cstyle_file_reader.hpp"

#include "server/lm/lm_executor.hpp"

using namespace std;
using namespace TCLAP;
using namespace uva::smt;
using namespace uva::smt::bpbd::common;
using namespace uva::smt::bpbd::server::lm;
using namespace uva::smt::bpbd::server::lm::dictionary;
using namespace uva::utils::file;
using namespace uva::utils::logging;
using namespace uva::utils::text;
using namespace uva::utils::exceptions;

//Declare the program version string
#define PROGRAM_VERSION_STR "1.1"

/**
 * This functions does nothing more but printing the program header information
 */
static void print_info() {
    print_info("Back Off Language Model(s) for SMT", PROGRAM_VERSION_STR);
}

//The pointer to the command line parameters parser
static CmdLine * p_cmd_args = NULL;
static ValueArg<string> * p_model_arg = NULL;
static ValueArg<string> * p_query_arg = NULL;
static vector<string> trie_types_vec;
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

    //Add the -c the "cumulative" probability switch - optional, default is cumulative
    p_cumulative_prob_arg = new SwitchArg("c", "cumulative", "Compute the sum of cumulative log probabilities for each query m-gram", *p_cmd_args, false);

    //Add the -d the debug level parameter - optional, default is e.g. RESULT
    Logger::get_reporting_levels(&debug_levels);
    p_debug_levels_constr = new ValuesConstraint<string>(debug_levels);
    p_debug_level_arg = new ValueArg<string>("d", "debug", "The debug level to be used", false, RESULT_PARAM_VALUE, p_debug_levels_constr, *p_cmd_args);
}

/**
 * Allows to deallocate the parameters parser if it is needed
 */
void destroy_arguments_parser() {
    SAFE_DESTROY(p_model_arg);
    SAFE_DESTROY(p_query_arg);

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
static void extract_arguments(const uint argc, char const * const * const argv, __executor::lm_exec_params & params) {
    //Parse the arguments
    try {
        p_cmd_args->parse(argc, argv);
    } catch (ArgException &e) {
        THROW_EXCEPTION(string("Error: ") + e.error() + string(", for argument: ") + e.argId());
    }

    //Set the logging level right away
    Logger::set_reporting_level(p_debug_level_arg->getValue());

    //Store the parsed parameter values
    params.m_is_cum_prob = p_cumulative_prob_arg->getValue();
    params.m_query_file_name = p_query_arg->getValue();
    params.m_lm_params.m_conn_string = p_model_arg->getValue();
    params.m_lm_params.num_lm_weights = 0;
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
        __executor::lm_exec_params params = {};

        LOG_INFO << "Checking on the program arguments ..." << END_LOG;

        //Attempt to extract the program arguments
        extract_arguments(argc, argv, params);

        LOG_DEBUG << "Checking on the provided files \'"
                << params.m_lm_params.m_conn_string << "\' and \'"
                << params.m_query_file_name << "\' ..." << END_LOG;

        //Do the actual work, read the text corpse, create trie and do queries
        __executor::perform_tasks(params);

    } catch (Exception & ex) {
        //The argument's extraction has failed, print the error message and quit
        LOG_ERROR << ex.get_message() << END_LOG;
        returnCode = 1;
    }

    //Destroy the command line parameters parser
    destroy_arguments_parser();

    return returnCode;
}
