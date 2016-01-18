/* 
 * File:   bpbd_server.cpp
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
 * Created on January 14, 2016, 11:08 AM
 */

#include <cstdlib>
#include <string>

#include "tclap/CmdLine.h"
#include "INI.h"

#include "common.hpp"

#include "server/translation_server.hpp"

#include "utils/Exceptions.hpp"

using namespace std;
using namespace TCLAP;
using namespace uva::smt::decoding::server;
using namespace uva::smt::decoding::common;
using namespace uva::utils::exceptions;

//Declare the program version string
#define PROGRAM_VERSION_STR "1.0"

/**
 * This structure stores the program execution parameters
 */
typedef struct {
    //The language model file name 
    string m_language_model;
    //The translation model file name 
    string m_translation_model;
    //The reordering model file name 
    string m_reordering_model;

    //The target language name
    string m_target_lang;
    //The source language name
    string m_source_lang;

    //The port to listen to
    uint16_t m_server_port;

    //The distortion limit to use
    uint32_t m_distortion_limit;
    //The pruning threshold to be used
    float m_pruning_threshold;
    //The stack capacity for stack pruning
    uint32_t m_stack_capacity;
    //The stack expansion strategy
    string m_expansion_strategy;
} TExecutionParams;

/**
 * This functions does nothing more but printing the program header information
 */
static void print_info() {
    print_info("The translation server application", PROGRAM_VERSION_STR);
}

//The pointer to the command line parameters parser
static CmdLine * p_cmd_args = NULL;
static ValueArg<string> * p_config_file_arg = NULL;
static vector<string> debug_levels;
static ValuesConstraint<string> * p_debug_levels_constr = NULL;
static ValueArg<string> * p_debug_level_arg = NULL;

/**
 * Creates and sets up the command line parameters parser
 */
void create_arguments_parser() {
    //Declare the command line arguments parser
    p_cmd_args = new CmdLine("", ' ', PROGRAM_VERSION_STR);

    //Add the configuration file parameter - compulsory
    p_config_file_arg = new ValueArg<string>("c", "config", "The configuration file with the server options", true, "", "server configuration file", *p_cmd_args);

    //Add the -d the debug level parameter - optional, default is e.g. RESULT
    Logger::get_reporting_levels(&debug_levels);
    p_debug_levels_constr = new ValuesConstraint<string>(debug_levels);
    p_debug_level_arg = new ValueArg<string>("d", "debug", "The debug level to be used", false, RESULT_PARAM_VALUE, p_debug_levels_constr, *p_cmd_args);
}

/**
 * Allows to deallocate the parameters parser if it is needed
 */
void destroy_arguments_parser() {
    SAFE_DESTROY(p_config_file_arg);
    SAFE_DESTROY(p_debug_levels_constr);
    SAFE_DESTROY(p_debug_level_arg);
    SAFE_DESTROY(p_cmd_args);
}

//Declare the default value
static const string UNKNOWN_INI_FILE_VALUE = "<UNKNOWN_VALUE>";

//Allows to get and assert on the given section/key value presence
#define GET_ASSERT(ini, section, key, value_str) \
    const string value_str = ini.get(section, key, UNKNOWN_INI_FILE_VALUE); \
    ASSERT_CONDITION_THROW((value_str == UNKNOWN_INI_FILE_VALUE), \
            string("Could not find '[") + section + string("]/") + \
            key + string("' section/key in the configuration file!"));

template<typename INT_TYPE>
INT_TYPE get_integer(INI<> &ini, string section, string key) {
    //Get the value and assert on its presence
    GET_ASSERT(ini, section, key, value_str);

    //Parse this value to an integer
    return (INT_TYPE) stoi(value_str);
}

string get_string(INI<> &ini, string section, string key) {
    //Get the value and assert on its presence
    GET_ASSERT(ini, section, key, value_str);

    //Parse this value to an integer
    return value_str;
}

float get_float(INI<> &ini, string section, string key) {
    //Get the value and assert on its presence
    GET_ASSERT(ini, section, key, value_str);

    //Parse this value to an integer
    return stof(value_str);
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
        THROW_EXCEPTION(string("Error: ") + e.error() + string(", for argument: ") + e.argId());
    }

    //Set the logging level right away
    Logger::set_reporting_level(p_debug_level_arg->getValue());

    //Get the configuration file name and read the config values from the file
    const string config_file_name = p_config_file_arg->getValue();
    INI<> ini(config_file_name, false);

    //Parse the configuration file
    if (ini.parse()) {
        //Get the configuration options from the file
        string section = "Server Options";
        params.m_server_port = get_integer<uint16_t>(ini, section, "server_port");
        section = "Language Options";
        params.m_source_lang = get_string(ini, section, "source_lang");
        params.m_target_lang = get_string(ini, section, "target_lang1");

        section = "Input Models";
        params.m_language_model = get_string(ini, section, "language_model");
        params.m_translation_model = get_string(ini, section, "translation_model");
        params.m_reordering_model = get_string(ini, section, "reordering_model");

        section = "Decoding Options";
        params.m_distortion_limit = get_integer<uint32_t>(ini, section, "distortion_limit");
        params.m_pruning_threshold = get_float(ini, section, "pruning_threshold");
        params.m_stack_capacity = get_integer<uint32_t>(ini, section, "stack_capacity");
        params.m_expansion_strategy = get_string(ini, section, "expansion_strategy");
    } else {
        //We could not parse the configuration file, report an error
        THROW_EXCEPTION(string("Could not parse the configuration file: ") + config_file_name);
    }
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

        //Instantiate the translation server
        translation_server server(params.m_server_port);

        //Run the translation server
        server.run();
    } catch (Exception & ex) {
        //The argument's extraction has failed, print the error message and quit
        LOG_ERROR << ex.getMessage() << END_LOG;
        returnCode = 1;
    }

    //Destroy the command line parameters parser
    destroy_arguments_parser();

    return returnCode;
}

