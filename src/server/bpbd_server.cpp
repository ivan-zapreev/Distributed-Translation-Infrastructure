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

#include <cctype>
#include <cstdlib>
#include <string>

#include <websocketpp/common/thread.hpp>
#include <tclap/CmdLine.h>
#include <INI.h>

#include "main.hpp"

#include "server/server_parameters.hpp"
#include "server/translation_server.hpp"
#include "common/utils/exceptions.hpp"
#include "server/lm/lm_configurator.hpp"
#include "server/tm/tm_configurator.hpp"
#include "server/rm/rm_configurator.hpp"

using namespace std;
using namespace TCLAP;
using namespace uva::smt::bpbd::server;
using namespace uva::smt::bpbd::common;
using namespace uva::utils::exceptions;

using websocketpp::lib::bind;

//Declare the program version string
#define PROGRAM_VERSION_STR "1.0"

//Declare the program exit letter
#define PROGRAM_EXIT_LETTER 'q'

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
static void extract_arguments(const uint argc, char const * const * const argv, server_parameters & params) {
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
    LOG_USAGE << "Loading the server configuration option from: " << config_file_name << END_LOG;
    INI<> ini(config_file_name, false);

    //Parse the configuration file
    if (ini.parse()) {
        //Get the configuration options from the file
        string section = "Server Options";
        params.m_de_params.m_server_port = get_integer<uint16_t>(ini, section, "server_port");
        params.m_de_params.m_num_threads = get_integer<uint16_t>(ini, section, "num_threads");

        section = "Language Options";
        params.m_de_params.m_source_lang = get_string(ini, section, "source_lang");
        params.m_de_params.m_target_lang = get_string(ini, section, "target_lang");

        LOG_USAGE << "Translation server from '" << params.m_de_params.m_source_lang << "' into '"
                << params.m_de_params.m_target_lang << "' on port: '" << params.m_de_params.m_server_port
                << "' translation threads: '" << params.m_de_params.m_num_threads << "'" << END_LOG;

        section = "Language Models";
        params.m_lm_params.m_conn_string = get_string(ini, section, "language_model");
        LOG_INFO << "Language model file: " << params.m_lm_params.m_conn_string << END_LOG;
        params.m_lm_params.m_trie_type_name = get_string(ini, section, "trie_type_name");
        LOG_INFO << "Trie type name: " << params.m_lm_params.m_trie_type_name << END_LOG;
        params.m_lm_params.m_max_trie_level = get_integer<uint8_t>(ini, section, "max_trie_level");
        LOG_INFO << "The trie maximum level: " << params.m_lm_params.m_max_trie_level << END_LOG;

        //Set the trie and word index type
        __configurator::get_trie_and_word_index_types(params.m_lm_params);

        section = "Translation Models";
        params.m_tm_params.m_conn_string = get_string(ini, section, "translation_model");
        LOG_INFO << "Translation model file: " << params.m_tm_params.m_conn_string << END_LOG;

        section = "Reordering Models";
        params.m_rm_params.m_conn_string = get_string(ini, section, "reordering_model");
        LOG_INFO << "Reordering model file: " << params.m_rm_params.m_conn_string << END_LOG;

        section = "Decoding Options";
        params.m_de_params.m_distortion_limit = get_integer<uint32_t>(ini, section, "distortion_limit");
        params.m_de_params.m_pruning_threshold = get_float(ini, section, "pruning_threshold");
        params.m_de_params.m_stack_capacity = get_integer<uint32_t>(ini, section, "stack_capacity");
        LOG_INFO << "Distortion limit: " << params.m_de_params.m_distortion_limit
                << ", pruning threshold: " << params.m_de_params.m_pruning_threshold
                << ", stack capacity: " << params.m_de_params.m_stack_capacity << END_LOG;
        params.m_de_params.m_expansion_strategy = get_string(ini, section, "expansion_strategy");
        LOG_INFO << "Expansion strategy: " << params.m_de_params.m_expansion_strategy << END_LOG;

        LOG_INFO3 << "Sanity checks are: " << (DO_SANITY_CHECKS ? "ON" : "OFF") << " !" << END_LOG;
    } else {
        //We could not parse the configuration file, report an error
        THROW_EXCEPTION(string("Could not find or parse the configuration file: ") + config_file_name);
    }
}

/**
 * Allows to print the prompt
 */
void print_the_prompt() {
    cout << ">> ";
}

/**
 * Prints the available server commands
 */
void print_server_commands() {
    LOG_USAGE << "Available server commands: " << END_LOG;
    LOG_USAGE << "\t'" << PROGRAM_EXIT_LETTER << " & <enter>'  - to exit." << END_LOG;
}

/**
 * Allows to establish connections to the models: language, translation, reordering
 * @param params the parameters needed to establish connections to the models
 */
void connect_to_models(const server_parameters & params){
    //Connect to the language model
    lm_configurator::connect(params.m_lm_params);
    
    //Connect to the translation model
    tm_configurator::connect(params.m_tm_params);
    
    //Connect to the reordering model
    rm_configurator::connect(params.m_rm_params);
}

/**
 * Allows to disconnect from the models: language, translation, reordering
 */
void disconnect_from_models(){
    //Disconnect from the language model
    lm_configurator::disconnect();
    
    //Disconnect from the translation model
    tm_configurator::disconnect();
    
    //Disconnect from the reordering model
    rm_configurator::disconnect();
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
        server_parameters params = {};

        //Attempt to extract the program arguments
        extract_arguments(argc, argv, params);

        //Initialize connections to the used models
        connect_to_models(params);

        //Instantiate the translation server
        translation_server server(params.m_de_params.m_server_port, params.m_de_params.m_num_threads);

        //Run the translation server in a separate thread
        thread server_thread(bind(&translation_server::run, &server));

        LOG_USAGE << "The server is started!" << END_LOG;
        //Print the server commands menu
        print_server_commands();
        //Print the prompt
        print_the_prompt();

        //Wait until the server is stopped by pressing and exit button
        while (true) {
            char letter;
            cin >> letter;
            if (tolower(letter) == PROGRAM_EXIT_LETTER) {
                break;
            } else {
                LOG_ERROR << "The command '" << letter << "' is unknown!" << END_LOG;
                //Print the server commands menu
                print_server_commands();
            }
            //Print the prompt
            print_the_prompt();
        }

        //Stop the translation server
        LOG_USAGE << "Stopping the server ..." << END_LOG;
        server.stop();

        //Wait until the server's thread stops
        server_thread.join();

        LOG_USAGE << "The server has stopped!" << END_LOG;
    } catch (Exception & ex) {
        //The argument's extraction has failed, print the error message and quit
        LOG_ERROR << ex.get_message() << END_LOG;
        return_code = 1;
    }
    
    //Disconnect from the used models
    disconnect_from_models();

    //Destroy the command line parameters parser
    destroy_arguments_parser();

    return return_code;
}

