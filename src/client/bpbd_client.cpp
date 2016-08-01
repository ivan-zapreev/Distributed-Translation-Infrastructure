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

#include "main.hpp"
#include "client/client_parameters.hpp"
#include "client/trans_manager.hpp"
#include "common/utils/exceptions.hpp"

using namespace std;

using namespace TCLAP;

using namespace uva::utils::exceptions;
using namespace uva::utils::file;

using namespace uva::smt::bpbd::client;

using namespace uva::smt::bpbd::common;
using namespace uva::smt::bpbd::common::messaging;

/**
 * This functions does nothing more but printing the program header information
 */
static void print_info() {
    print_info("The translation client application");
}

//The pointer to the command line parameters parser
static CmdLine * p_cmd_args = NULL;
static ValueArg<string> * p_source_file_arg = NULL;
static ValueArg<string> * p_source_lang_arg = NULL;
static ValueArg<string> * p_target_file_arg = NULL;
static ValueArg<string> * p_target_lang_arg = NULL;
static ValueArg<string> * p_pre_uri_arg = NULL;
static ValueArg<string> * p_trans_uri_arg = NULL;
static ValueArg<string> * p_post_uri_arg = NULL;
static ValueArg<uint32_t> * p_max_sent = NULL;
static ValueArg<uint32_t> * p_min_sent = NULL;
static vector<string> debug_levels;
static ValuesConstraint<string> * p_debug_levels_constr = NULL;
static ValueArg<string> * p_debug_level_arg = NULL;
static SwitchArg * p_trans_details_arg = NULL;

/**
 * Creates and sets up the command line parameters parser
 */
void create_arguments_parser() {
    //Declare the command line arguments parser
    p_cmd_args = new CmdLine("", ' ', PROGRAM_VERSION_STR);

    //Add the input file to translate parameter - compulsory

    p_source_file_arg = new ValueArg<string>("I", "input-file", string("The utf8 source file with the ") +
            string("input corpus to translate"), true, "", "source file name", *p_cmd_args);

    //Add the source language type for the source file parameter - compulsory
    p_source_lang_arg = new ValueArg<string>("i", "input-lang", string("The source language to ") +
            string("translate from"), true, "", "source language", *p_cmd_args);

    //Add the output file for translated text parameter - compulsory
    p_target_file_arg = new ValueArg<string>("O", "output-file", string("The utf8 output file to ") +
            string("put the translation into"), true, "", "target file name", *p_cmd_args);

    //Add the the target language to translate into parameter - optional, by default is "English"
    p_target_lang_arg = new ValueArg<string>("o", "output-lang", string("The target language to ") +
            string("translate into, default is 'English'"), false, "English", "target language", *p_cmd_args);

    p_pre_uri_arg = new ValueArg<string>("r", "pre-processor", string("The pre-processor server ") +
            string("URI to connect to, optional"), false, DEFAULT_PROCESSOR_URI, "pre-processor uri", *p_cmd_args);    
    
    //Add the the translation server ip address or name parameter - optional, by default is "localhost"
    const string def_server_uri = "ws://localhost:9002";
    p_trans_uri_arg = new ValueArg<string>("s", "trans-server", string("The translation server ") +
            string("URI to connect to, default is '") + def_server_uri + string("'"), false,
            def_server_uri, "server uri", *p_cmd_args);

    p_post_uri_arg = new ValueArg<string>("p", "post-processor", string("The post-processor server ") +
            string("URI to connect to, optional"), false, DEFAULT_PROCESSOR_URI, "post-processor uri", *p_cmd_args);    

    //Add the maximum number of sentences to send by a job parameter - optional, by default is 100
    p_max_sent = new ValueArg<uint32_t>("u", "upper-size", string("The maximum number of sentences ") +
            string("to send per request, default is 100"), false, 100, "max #sentences per request", *p_cmd_args);

    //Add the minimum number of sentences to send by a job parameter - optional, by default is 10
    p_min_sent = new ValueArg<uint32_t>("l", "lower-size", string("The minimum number of sentences ") +
            string("to send per request, default is 100"), false, 100, "min #sentences per request", *p_cmd_args);

    //Add the translation details switch parameter - ostring(ptional, default is false
    p_trans_details_arg = new SwitchArg("c", "clues", string("Request the server to provide the ") +
            string("translation details"), *p_cmd_args, false);

    //Add the -d the debug level parameter - optional, default is e.g. RESULT
    logger::get_reporting_levels(&debug_levels);
    p_debug_levels_constr = new ValuesConstraint<string>(debug_levels);
    p_debug_level_arg = new ValueArg<string>("d", "debug", "The debug level to be used",
            false, RESULT_PARAM_VALUE, p_debug_levels_constr, *p_cmd_args);
}

/**
 * Allows to deallocate the parameters parser if it is needed
 */
void destroy_arguments_parser() {
    SAFE_DESTROY(p_source_file_arg);
    SAFE_DESTROY(p_source_lang_arg);
    SAFE_DESTROY(p_target_file_arg);
    SAFE_DESTROY(p_target_lang_arg);
    SAFE_DESTROY(p_pre_uri_arg);
    SAFE_DESTROY(p_trans_uri_arg);
    SAFE_DESTROY(p_post_uri_arg);
    SAFE_DESTROY(p_max_sent);
    SAFE_DESTROY(p_min_sent);
    SAFE_DESTROY(p_trans_details_arg);
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
static void extract_arguments(const uint argc, char const * const * const argv, client_parameters & params) {
    //Parse the arguments
    try {
        p_cmd_args->parse(argc, argv);
    } catch (ArgException &e) {
        THROW_EXCEPTION(string("Error: ") + e.error() + string(", for argument: ") + e.argId());
    }

    //Set the logging level right away
    logger::set_reporting_level(p_debug_level_arg->getValue());

    //Store the parsed parameter values
    params.m_source_file = p_source_file_arg->getValue();
    params.m_source_lang = p_source_lang_arg->getValue();
    LOG_USAGE << "Given input file: '" << params.m_source_file << "', language: '" << params.m_source_lang << "'" << END_LOG;

    params.m_target_file = p_target_file_arg->getValue();
    params.m_target_lang = p_target_lang_arg->getValue();
    LOG_USAGE << "Given output file: '" << params.m_target_file << "', language: '" << params.m_target_lang << "'" << END_LOG;

    params.m_pre_uri = p_pre_uri_arg->getValue();
    if (params.is_pre_process()) {
        LOG_USAGE << "Using pre-processor server URI: " << params.m_pre_uri << END_LOG;
    } else {
        LOG_WARNING << "No text pre-processing enabled!" << END_LOG;
    }

    params.m_trans_uri = p_trans_uri_arg->getValue();
    LOG_USAGE << "Using translation server URI: " << params.m_trans_uri << END_LOG;

    params.m_post_uri = p_post_uri_arg->getValue();
    if (params.is_post_process()) {
        LOG_USAGE << "Using post-processor server URI: " << params.m_post_uri << END_LOG;
    } else {
        LOG_WARNING << "No text post-processing enabled!" << END_LOG;
    }

    params.m_min_sent = p_min_sent->getValue();
    params.m_max_sent = p_max_sent->getValue();
    LOG_USAGE << "Min/max number of sentences per request: '" << params.m_min_sent << "/" << params.m_max_sent << "'" << END_LOG;

    params.m_is_trans_info = p_trans_details_arg->getValue();
    LOG_USAGE << "Translation info is " << (params.m_is_trans_info ? "ON" : "OFF") << END_LOG;

    //Finalize the results
    params.finalize();
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
        client_parameters params = {};

        //Attempt to extract the program arguments
        extract_arguments(argc, argv, params);

        //Create the translation manager
        trans_manager manager(params);

        //Start the translation manager
        LOG_USAGE << "Starting the translation process ..." << END_LOG;
        manager.start();

        //Wait until the translations are done
        LOG_USAGE << "Waiting for the translation process to finish ..." << END_LOG;
        manager.wait();

        //Stop the translation manager
        LOG_USAGE << "Finishing the translation process ..." << END_LOG;
        manager.stop();

        LOG_USAGE << "The translation process is finished" << END_LOG;
    } catch (std::exception & ex) {
        //The argument's extraction has failed, print the error message and quit
        LOG_ERROR << ex.what() << END_LOG;
        return_code = 1;
    }

    //Destroy the command line parameters parser
    destroy_arguments_parser();

    return return_code;
}

