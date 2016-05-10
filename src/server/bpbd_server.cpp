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

#include "main.hpp"

#include "server/server_parameters.hpp"
#include "server/translation_server.hpp"

#include "common/utils/exceptions.hpp"

#include "server/decoder/de_configurator.hpp"
#include "server/lm/lm_configurator.hpp"
#include "server/tm/tm_configurator.hpp"
#include "server/rm/rm_configurator.hpp"

#include "server/cmd_line_handler.hpp"

using namespace std;
using namespace TCLAP;

using namespace uva::smt::bpbd::common;
using namespace uva::smt::bpbd::server;
using namespace uva::smt::bpbd::server::decoder;
using namespace uva::smt::bpbd::server::rm;
using namespace uva::smt::bpbd::server::lm;
using namespace uva::smt::bpbd::server::tm;

using namespace uva::utils::exceptions;

using websocketpp::lib::bind;

//Declare the program version string
#define PROGRAM_VERSION_STR "1.0"

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
    logger::get_reporting_levels(&debug_levels);
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
    logger::set_reporting_level(p_debug_level_arg->getValue());

    //Get the configuration file name and read the config values from the file
    const string config_file_name = p_config_file_arg->getValue();
    LOG_USAGE << "Loading the server configuration option from: " << config_file_name << END_LOG;
    INI<> ini(config_file_name, false);

    //Parse the configuration file
    if (ini.parse()) {
        LOG_INFO << "The configuration file has been parsed!" << END_LOG;
        
        //Get the configuration options from the file
        string section = "Server Options";
        params.m_server_port = get_integer<uint16_t>(ini, section, "server_port");
        params.m_num_threads = get_integer<uint16_t>(ini, section, "num_threads");
        params.m_source_lang = get_string(ini, section, "source_lang");
        params.m_target_lang = get_string(ini, section, "target_lang");
        params.verify();

        LOG_USAGE << "Translation server from '" << params.m_source_lang << "' into '"
                << params.m_target_lang << "' on port: '" << params.m_server_port
                << "' translation threads: '" << params.m_num_threads << "'" << END_LOG;

        section = "Language Models";
        params.m_lm_params.m_conn_string = get_string(ini, section, "connection_string");
        tokenize_s_t_f<NUM_LM_FEATURES>("lm_feature_weights",
                get_string(ini, section, "lm_feature_weights"),
                params.m_lm_params.m_lambdas,
                params.m_lm_params.m_num_lambdas,
                LM_FEATURE_WEIGHTS_DELIMITER_STR);
        params.m_lm_params.finalize();
        LOG_INFO << params.m_lm_params << END_LOG;

        section = "Translation Models";
        params.m_tm_params.m_conn_string = get_string(ini, section, "connection_string");
        tokenize_s_t_f<NUM_TM_FEATURES>("tm_feature_weights",
                get_string(ini, section, "tm_feature_weights"),
                params.m_tm_params.m_lambdas,
                params.m_tm_params.m_num_lambdas,
                TM_FEATURE_WEIGHTS_DELIMITER_STR);
        tokenize_s_t_f<NUM_TM_FEATURES>("tm_unk_features",
                get_string(ini, section, "tm_unk_features"),
                params.m_tm_params.m_unk_features,
                params.m_tm_params.m_num_unk_features,
                TM_FEATURE_WEIGHTS_DELIMITER_STR);
        params.m_tm_params.m_trans_limit = get_integer<size_t>(ini, section, "translation_limit");
        params.m_tm_params.m_min_tran_prob = get_float(ini, section, "min_trans_prob");
        params.m_tm_params.finalize();
        LOG_INFO << params.m_tm_params << END_LOG;

        section = "Reordering Models";
        params.m_rm_params.m_conn_string = get_string(ini, section, "connection_string");
        tokenize_s_t_f<NUM_RM_FEATURES>("rm_feature_weights",
                get_string(ini, section, "rm_feature_weights"),
                params.m_rm_params.m_lambdas,
                params.m_rm_params.m_num_lambdas,
                RM_FEATURE_WEIGHTS_DELIMITER_STR);
        params.m_rm_params.finalize();
        LOG_INFO << params.m_rm_params << END_LOG;

        section = "Decoding Options";
        params.m_de_params.m_num_best_trans = get_integer<uint32_t>(ini, section, "num_best_trans");
        params.m_de_params.m_distortion = get_integer<int32_t>(ini, section, "distortion");
        params.m_de_params.m_pruning_threshold = get_float(ini, section, "pruning_threshold");
        params.m_de_params.m_stack_capacity = get_integer<uint32_t>(ini, section, "stack_capacity");
        params.m_de_params.m_max_s_phrase_len = get_integer<phrase_length>(ini, section, "max_source_phrase_length");
        params.m_de_params.m_max_t_phrase_len = get_integer<phrase_length>(ini, section, "max_target_phrase_length");
        params.m_de_params.m_word_penalty = get_float(ini, section, "word_penalty");
        params.m_de_params.m_phrase_penalty = get_float(ini, section, "phrase_penalty");
        params.m_de_params.m_lin_dist_penalty = get_float(ini, section, "lin_dist_penalty");
        params.m_de_params.m_is_gen_lattice = get_bool(ini, section, "is_gen_lattice");
        params.m_de_params.finalize();
        LOG_INFO << params.m_de_params << END_LOG;

        LOG_INFO3 << "Sanity checks are: " << (DO_SANITY_CHECKS ? "ON" : "OFF") << " !" << END_LOG;
    } else {
        //We could not parse the configuration file, report an error
        THROW_EXCEPTION(string("Could not find or parse the configuration file: ") + config_file_name);
    }
}

/**
 * Allows to establish connections to the models: language, translation, reordering
 * @param params the parameters needed to establish connections to the models
 */
void connect_to_models(const server_parameters & params) {
    //Connect to the language model
    lm_configurator::connect(params.m_lm_params);

    //Connect to the translation model
    tm_configurator::connect(params.m_tm_params);

    //Connect to the reordering model
    rm_configurator::connect(params.m_rm_params);

    //Connect to the decoder
    de_configurator::connect(params.m_de_params);
}

/**
 * Allows to disconnect from the models: language, translation, reordering
 */
void disconnect_from_models() {
    //Disconnect from the language model
    lm_configurator::disconnect();

    //Disconnect from the translation model
    tm_configurator::disconnect();

    //Disconnect from the reordering model
    rm_configurator::disconnect();

    //Disconnect from the decoder
    de_configurator::disconnect();
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
        server_parameters params;

        //Attempt to extract the program arguments
        extract_arguments(argc, argv, params);

        //Initialize connections to the used models
        connect_to_models(params);

        //Instantiate the translation server
        translation_server server(params.m_server_port, params.m_num_threads);

        //Run the translation server in a separate thread
        thread server_thread(bind(&translation_server::run, &server));

        LOG_USAGE << "The server is started!" << END_LOG;

        //Override the reporting level for testing purposes
        //logger::get_reporting_level() = debug_levels_enum::DEBUG2;

        //Wait until the server is stopped by pressing and exit button
        perform_command_loop(params, server, server_thread);

    } catch (uva_exception & ex) {
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

