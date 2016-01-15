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

#include <cstdlib>
#include <string>

#include "tclap/CmdLine.h"

#include "common.hpp"

#include "client/translation_client.hpp"

#include "utils/Exceptions.hpp"

using namespace std;
using namespace TCLAP;
using namespace uva::smt::decoding::client;
using namespace uva::smt::decoding::common;
using namespace uva::utils::exceptions;

//Declare the program version string
#define PROGRAM_VERSION_STR "1.0"

/**
 * This structure stores the program execution parameters
 */
typedef struct {
    string file_in;
    string lang_in;
    string file_out;
    string lang_out;
    string host;
    uint16_t port;
} TExecutionParams;

/**
 * This functions does nothing more but printing the program header information
 */
static void print_info() {
    print_info("The translation client application", PROGRAM_VERSION_STR);
}

//The pointer to the command line parameters parser
static CmdLine * p_cmd_args = NULL;
static ValueArg<string> * p_file_in_arg = NULL;
static ValueArg<string> * p_lang_in_arg = NULL;
static ValueArg<string> * p_file_out_arg = NULL;
static ValueArg<string> * p_lang_out_arg = NULL;
static ValueArg<string> * p_host_arg = NULL;
static ValueArg<uint16_t> * p_port_arg = NULL;
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
    p_file_in_arg = new ValueArg<string>("if", "input-file", "A back-off language model file name in ARPA format", true, "", "model file name", *p_cmd_args);

    //Add the  parameter - compulsory
    p_lang_in_arg = new ValueArg<string>("il", "input-lang", "A back-off language model file name in ARPA format", true, "", "model file name", *p_cmd_args);

    //Add the  parameter - compulsory
    p_file_out_arg = new ValueArg<string>("of", "output-file", "A text file containing new line separated M-gram queries", true, "", "query file name", *p_cmd_args);

    //Add the  parameter - optional, by default is "English"
    p_lang_out_arg = new ValueArg<string>("ol", "output-lang", "A text file containing new line separated M-gram queries", false, "English", "query file name", *p_cmd_args);

    //Add the  parameter - optional, by default is "localhost"
    p_host_arg = new ValueArg<string>("h", "host", "A text file containing new line separated M-gram queries", false, "localhost", "query file name", *p_cmd_args);

    //Add the  parameter - optional, by default is 9002
    p_port_arg = new ValueArg<uint16_t>("p", "port", "A text file containing new line separated M-gram queries", false, 9002, "query file name", *p_cmd_args);

    //Add the -d the debug level parameter - optional, default is e.g. RESULT
    Logger::get_reporting_levels(&debug_levels);
    p_debug_levels_constr = new ValuesConstraint<string>(debug_levels);
    p_debug_level_arg = new ValueArg<string>("d", "debug", "The debug level to be used", false, RESULT_PARAM_VALUE, p_debug_levels_constr, *p_cmd_args);
}

/**
 * Allows to deallocate the parameters parser if it is needed
 */
void destroy_arguments_parser() {
    SAFE_DESTROY(p_file_in_arg);
    SAFE_DESTROY(p_lang_in_arg);
    SAFE_DESTROY(p_file_out_arg);
    SAFE_DESTROY(p_lang_out_arg);
    SAFE_DESTROY(p_host_arg);
    SAFE_DESTROY(p_port_arg);
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
static void extract_arguments(const uint argc, char const * const * const argv, TExecutionParams & params) {
    //Parse the arguments
    try {
        p_cmd_args->parse(argc, argv);
    } catch (ArgException &e) {
        stringstream msg;
        msg << "Error: " << e.error() << ", for argument: " << e.argId();
        throw Exception(msg.str());
    }

    //Store the parsed parameter values
    params.file_in = p_file_in_arg->getValue();
    params.lang_in = p_lang_in_arg->getValue();
    params.file_out = p_file_out_arg->getValue();
    params.lang_out = p_lang_out_arg->getValue();
    params.host = p_host_arg->getValue();
    params.port = p_port_arg->getValue();

    //Set the logging level right away
    Logger::set_reporting_level(p_debug_level_arg->getValue());
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
        TExecutionParams params = {};

        //Attempt to extract the program arguments
        extract_arguments(argc, argv, params);

        //Create the translation client
        translation_client client(params.host, params.port);
        
        //Connect to the translation server
        client.connect();
        
        //ToDo: Read the source corpus from the text file
        
        //ToDo: Query the translation job and wait for the reply

        //ToDo: Write the translation result to the text file

    } catch (Exception & ex) {
        //The argument's extraction has failed, print the error message and quit
        LOG_ERROR << ex.getMessage() << END_LOG;
        returnCode = 1;
    }

    //Destroy the command line parameters parser
    destroy_arguments_parser();

    return returnCode;
}

