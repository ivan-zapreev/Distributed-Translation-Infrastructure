/* 
 * File:   bpbd_client.cpp
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
 * Created on January 14, 2016, 11:07 AM
 */

#include <string>

#include "tclap/CmdLine.h"

#include "common.hpp"
#include "client/client_config.hpp"
#include "client/trans_manager.hpp"
#include "common/utils/Exceptions.hpp"

using namespace std;
using namespace TCLAP;
using namespace uva::smt::decoding::client;
using namespace uva::smt::decoding::common;
using namespace uva::smt::decoding::common::messaging;
using namespace uva::utils::exceptions;
using namespace uva::utils::file;

//Declare the program version string
#define PROGRAM_VERSION_STR "1.0"

/**
 * This functions does nothing more but printing the program header information
 */
static void print_info() {
    print_info("The translation client application", PROGRAM_VERSION_STR);
}

//The pointer to the command line parameters parser
static CmdLine * p_cmd_args = NULL;
static ValueArg<string> * p_source_file_arg = NULL;
static ValueArg<string> * p_source_lang_arg = NULL;
static ValueArg<string> * p_target_file_arg = NULL;
static ValueArg<string> * p_target_lang_arg = NULL;
static ValueArg<string> * p_server_arg = NULL;
static ValueArg<uint16_t> * p_port_arg = NULL;
static ValueArg<uint32_t> * p_max_sent = NULL;
static ValueArg<uint32_t> * p_min_sent = NULL;
static vector<string> debug_levels;
static ValuesConstraint<string> * p_debug_levels_constr = NULL;
static ValueArg<string> * p_debug_level_arg = NULL;

/**
 * Creates and sets up the command line parameters parser
 */
void create_arguments_parser() {
    //Declare the command line arguments parser
    p_cmd_args = new CmdLine("", ' ', PROGRAM_VERSION_STR);

    //Add the  parameter - compulsory
    p_source_file_arg = new ValueArg<string>("I", "input-file", "The source file with the input corpus to translate", true, "", "source file name", *p_cmd_args);

    //Add the  parameter - compulsory
    p_source_lang_arg = new ValueArg<string>("i", "input-lang", "The source language to translate from", true, "", "source language", *p_cmd_args);

    //Add the  parameter - compulsory
    p_target_file_arg = new ValueArg<string>("O", "output-file", "The output file to put the translation into", true, "", "target file name", *p_cmd_args);

    //Add the  parameter - optional, by default is "English"
    p_target_lang_arg = new ValueArg<string>("o", "output-lang", "The target language to translate into, default is 'English'", false, "English", "target language", *p_cmd_args);

    //Add the  parameter - optional, by default is "localhost"
    p_server_arg = new ValueArg<string>("s", "server", "The server address to connect to, default is 'localhost'", false, "localhost", "server address", *p_cmd_args);

    //Add the  parameter - optional, by default is 9002
    p_port_arg = new ValueArg<uint16_t>("p", "port", "The server port to connect to, default is 9002", false, 9002, "server port", *p_cmd_args);

    //Add the  parameter - optional, by default is 100
    p_max_sent = new ValueArg<uint32_t>("u", "upper-size", "The maximum number of sentences to send per request, default is 100", false, 100, "max #sentences per request", *p_cmd_args);

    //Add the  parameter - optional, by default is 100
    p_min_sent = new ValueArg<uint32_t>("l", "lower-size", "The minimum number of sentences to send per request, default is 10", false, 10, "min #sentences per request", *p_cmd_args);

    //Add the -d the debug level parameter - optional, default is e.g. RESULT
    Logger::get_reporting_levels(&debug_levels);
    p_debug_levels_constr = new ValuesConstraint<string>(debug_levels);
    p_debug_level_arg = new ValueArg<string>("d", "debug", "The debug level to be used", false, RESULT_PARAM_VALUE, p_debug_levels_constr, *p_cmd_args);
}

/**
 * Allows to deallocate the parameters parser if it is needed
 */
void destroy_arguments_parser() {
    SAFE_DESTROY(p_source_file_arg);
    SAFE_DESTROY(p_source_lang_arg);
    SAFE_DESTROY(p_target_file_arg);
    SAFE_DESTROY(p_target_lang_arg);
    SAFE_DESTROY(p_server_arg);
    SAFE_DESTROY(p_port_arg);
    SAFE_DESTROY(p_max_sent);
    SAFE_DESTROY(p_min_sent);
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
static void extract_arguments(const uint argc, char const * const * const argv, client_config & params) {
    //Parse the arguments
    try {
        p_cmd_args->parse(argc, argv);
    } catch (ArgException &e) {
        THROW_EXCEPTION(string("Error: ") + e.error() + string(", for argument: ") + e.argId());
    }

    //Set the logging level right away
    Logger::set_reporting_level(p_debug_level_arg->getValue());

    //Store the parsed parameter values
    params.m_source_file = p_source_file_arg->getValue();
    params.m_source_lang = p_source_lang_arg->getValue();
    LOG_USAGE << "Given input file: '" << params.m_source_file << "', language: '" << params.m_source_lang << "'" << END_LOG;

    params.m_target_file = p_target_file_arg->getValue();
    params.m_target_lang = p_target_lang_arg->getValue();
    LOG_USAGE << "Given output file: '" << params.m_target_file << "', language: '" << params.m_target_lang << "'" << END_LOG;

    params.m_server = p_server_arg->getValue();
    params.m_port = p_port_arg->getValue();
    LOG_USAGE << "Using server address: '" << params.m_server << "', port: '" << params.m_port << "'" << END_LOG;

    params.m_min_sent = p_min_sent->getValue();
    params.m_max_sent = p_max_sent->getValue();
    LOG_USAGE << "The min/max number of sentences per request: '" << params.m_min_sent << "/" << params.m_max_sent << "'" << END_LOG;
}

/**
 * The main program entry point
 */
int main(int argc, char** argv) {
    //Declare the return code
    int return_code = 0;

    //Set the uncaught exception handler
    std::set_terminate(handler);

    //First print the program info
    print_info();

    //Set up possible program arguments
    create_arguments_parser();

    try {
        //Define en empty parameters structure
        client_config params = {};

        //Attempt to extract the program arguments
        extract_arguments(argc, argv, params);

        //Create the translation manager
        trans_manager manager(params);

        //Start the translation manager
        LOG_USAGE << "Starting the translation process ..." << END_LOG;
        manager.start();

        //Wait until the translations are done
        LOG_USAGE << "Waiting for the the translation process to finish ..." << END_LOG;
        manager.wait();
        
        //Stop the translation manager
        LOG_USAGE << "Finishing the translation process ..." << END_LOG;
        manager.stop();

        LOG_USAGE << "The job is done, the results are in: '" << params.m_target_file << "'" << END_LOG;
    } catch (Exception & ex) {
        //The argument's extraction has failed, print the error message and quit
        LOG_ERROR << ex.get_message() << END_LOG;
        return_code = 1;
    }

    //Destroy the command line parameters parser
    destroy_arguments_parser();

    return return_code;
}

