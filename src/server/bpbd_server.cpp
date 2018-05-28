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
#include <vector>
#include <utility>
#include <sys/types.h>
#include <cstdlib>

#include <websocketpp/common/thread.hpp>
#include <tclap/CmdLine.h>

#include "main.hpp"

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"
#include "common/utils/file/utils.hpp"

#include "server/common/feature_id_registry.hpp"

#include "server/server_parameters.hpp"
#include "server/translation_server.hpp"
#include "server/server_consts.hpp"
#include "server/server_console.hpp"
#include "server/decoder/de_configurator.hpp"
#include "server/lm/lm_configurator.hpp"
#include "server/tm/tm_configurator.hpp"
#include "server/rm/rm_configurator.hpp"

using namespace std;
using namespace TCLAP;

using websocketpp::lib::bind;

using namespace uva::utils::exceptions;
using namespace uva::utils::logging;
using namespace uva::utils::file;

using namespace uva::smt::bpbd::common;
using namespace uva::smt::bpbd::server;
using namespace uva::smt::bpbd::server::common;
using namespace uva::smt::bpbd::server::decoder;
using namespace uva::smt::bpbd::server::rm;
using namespace uva::smt::bpbd::server::lm;
using namespace uva::smt::bpbd::server::tm;

/**
 * This functions does nothing more but printing the program header information
 */
static void print_info() {
    print_info("The translation server application");
}

//The pointer to the command line parameters parser
static CmdLine * p_cmd_args = NULL;
static ValueArg<string> * p_config_file_arg = NULL;
static vector<string> debug_levels;
static ValuesConstraint<string> * p_debug_levels_constr = NULL;
static ValueArg<string> * p_debug_level_arg = NULL;
#if IS_SERVER_TUNING_MODE
static SwitchArg * p_gen_fmap_arg = NULL;
#endif

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

#if IS_SERVER_TUNING_MODE
    //Add the translation details switch parameter - ostring(optional, default is false
    p_gen_fmap_arg = new SwitchArg("f", "feature", string("Only generate the feature id to name mapping file for the search lattice") +
            string("Only feature id file"), *p_cmd_args, false);
#endif
}

/**
 * Allows to deallocate the parameters parser if it is needed
 */
void destroy_arguments_parser() {
    SAFE_DESTROY(p_config_file_arg);
    SAFE_DESTROY(p_debug_levels_constr);
    SAFE_DESTROY(p_debug_level_arg);
#if IS_SERVER_TUNING_MODE
    SAFE_DESTROY(p_gen_fmap_arg);
#endif
    SAFE_DESTROY(p_cmd_args);
}

/**
 * This function allow to process the feature to id mappings needed
 * for the search lattice. As a part of this process we will need to
 * dump the id to feature weight name into the file and also set up
 * the parameters.
 * @param params the server parameters
 * @param dump_file if true then the id to feature mapping file will be dumped, otherwise not.
 * @param cfg_file_name the configuration file name
 */
inline void process_feature_to_id_mappings(server_parameters & params, const bool dump_file, const string & cfg_file_name) {
    //Declare the feature id registry
    feature_id_registry registry;

    //Add the features to the vector
    params.m_de_params.get_weight_names(registry);
    params.m_lm_params.get_weight_names(registry);
    params.m_rm_params.get_weight_names(registry);
    params.m_tm_params.get_weight_names(registry);

    //Dump the id to feature mapping into the file, if needed
    if (dump_file) {
        //Extract the configuration file name without any path prefix 
        string cfg_name = cfg_file_name;
        //Search for the last path delimiter symbol
        size_t last_pos = cfg_name.find_last_of("\\/");
        //If there path delimiter symbol was found take the remainder
        if (last_pos != string::npos) {
            cfg_name = cfg_name.substr(last_pos + 1);
        }

        //Construct the feature weight to id file name
        const string file_name = "./" + cfg_name + "." + params.m_de_params.m_li2n_file_ext;

        //Dump the mapping into the file
        registry.dump_feature_to_id_file(file_name);
    }

    //Set the number of features
    params.m_de_params.m_num_features = registry.size();
}

/**
 * Allows to parse the server config file and inialize the parameters
 * @param config_file_name the config file name
 * @param params the parameters to be initialized
 */
static void parse_config_file(const string & config_file_name, server_parameters & ts_params) {
    LOG_USAGE << "Loading the server configuration options from: " << config_file_name << END_LOG;
    INI<> ini(config_file_name, false);

    LOG_DEBUG << "Start parsing the configuration options from: " << config_file_name << END_LOG;
    //Parse the configuration file
    if (ini.parse()) {
        LOG_INFO << "The configuration file has been parsed!" << END_LOG;

        //Get the configuration options from the file
        string section = server_parameters::SE_CONFIG_SECTION_NAME;
        ts_params.m_server_port = get_integer<uint16_t>(ini, section,
                server_parameters::SE_SERVER_PORT_PARAM_NAME);
        ts_params.m_is_tls_server = get_bool(ini, section,
                server_parameters::SE_IS_TLS_SERVER_PARAM_NAME, "false", IS_TLS_SUPPORT);
        ts_params.m_tls_mode_name = get_string(ini, section,
                server_parameters::SE_TLS_MODE_PARAM_NAME, "", IS_TLS_SUPPORT);
        ts_params.m_tls_crt_file = get_string(ini, section,
                server_parameters::SE_TLS_CRT_FILE_PARAM_NAME, "", IS_TLS_SUPPORT);
        ts_params.m_tls_key_file = get_string(ini, section,
                server_parameters::SE_TLS_KEY_FILE_PARAM_NAME, "", IS_TLS_SUPPORT);
        ts_params.m_tls_dh_file = get_string(ini, section,
                server_parameters::SE_TLS_DH_FILE_PARAM_NAME, "", IS_TLS_SUPPORT);
        ts_params.m_num_threads = get_integer<uint16_t>(ini, section,
                server_parameters::SE_NUM_THREADS_PARAM_NAME);
        ts_params.m_source_lang = get_string(ini, section,
                server_parameters::SE_SOURCE_LANG_PARAM_NAME);
        ts_params.m_target_lang = get_string(ini, section,
                server_parameters::SE_TARGET_LANG_PARAM_NAME);

        section = lm_parameters::LM_CONFIG_SECTION_NAME;
        ts_params.m_lm_params.m_conn_string = get_string(ini, section,
                lm_parameters::LM_CONN_STRING_PARAM_NAME);
        tokenize_s_t_f<MAX_NUM_LM_FEATURES>(lm_parameters::LM_WEIGHTS_PARAM_NAME,
                get_string(ini, section, lm_parameters::LM_WEIGHTS_PARAM_NAME),
                ts_params.m_lm_params.m_lambdas,
                ts_params.m_lm_params.m_num_lambdas,
                LM_FEATURE_WEIGHTS_DELIMITER_STR);
        ts_params.m_lm_params.m_unk_word_log_e_prob = get_float(ini, section,
                lm_parameters::LM_UNK_WORD_LOG_E_PROB_PARAM_NAME);

        section = tm_parameters::TM_CONFIG_SECTION_NAME;
        ts_params.m_tm_params.m_conn_string = get_string(ini, section,
                tm_parameters::TM_CONN_STRING_PARAM_NAME);
        tokenize_s_t_f<MAX_NUM_TM_FEATURES>(tm_parameters::TM_WEIGHTS_PARAM_NAME,
                get_string(ini, section, tm_parameters::TM_WEIGHTS_PARAM_NAME),
                ts_params.m_tm_params.m_lambdas,
                ts_params.m_tm_params.m_num_lambdas,
                TM_FEATURE_WEIGHTS_DELIMITER_STR);
        tokenize_s_t_f<MAX_NUM_TM_FEATURES>(tm_parameters::TM_UNK_FEATURE_PARAM_NAME,
                get_string(ini, section, tm_parameters::TM_UNK_FEATURE_PARAM_NAME),
                ts_params.m_tm_params.m_unk_features,
                ts_params.m_tm_params.m_num_unk_features,
                TM_FEATURE_WEIGHTS_DELIMITER_STR);
        ts_params.m_tm_params.m_trans_limit = get_integer<size_t>(ini,
                section, tm_parameters::TM_TRANS_LIM_PARAM_NAME);
        ts_params.m_tm_params.m_min_tran_prob = get_float(ini, section,
                tm_parameters::TM_MIN_TRANS_PROB_PARAM_NAME);
        ts_params.m_tm_params.m_wp_lambda = get_float(ini, section,
                tm_parameters::TM_WORD_PENALTY_PARAM_NAME);

        section = rm_parameters::RM_CONFIG_SECTION_NAME;
        ts_params.m_rm_params.m_conn_string = get_string(ini, section,
                rm_parameters::RM_CONN_STRING_PARAM_NAME);
        tokenize_s_t_f<MAX_NUM_RM_FEATURES>(rm_parameters::RM_WEIGHTS_PARAM_NAME,
                get_string(ini, section, rm_parameters::RM_WEIGHTS_PARAM_NAME),
                ts_params.m_rm_params.m_lambdas,
                ts_params.m_rm_params.m_num_lambdas,
                RM_FEATURE_WEIGHTS_DELIMITER_STR);

        section = de_parameters::DE_CONFIG_SECTION_NAME;
        ts_params.m_de_params.m_pruning_threshold = get_float(ini, section,
                de_parameters::DE_PRUNING_THRESHOLD_PARAM_NAME);
        ts_params.m_de_params.m_stack_capacity = get_integer<uint32_t>(ini, section,
                de_parameters::DE_STACK_CAPACITY_PARAM_NAME);
        ts_params.m_de_params.m_max_s_phrase_len = get_integer<phrase_length>(ini, section,
                de_parameters::DE_MAX_SP_LEN_PARAM_NAME);
        ts_params.m_de_params.m_max_t_phrase_len = get_integer<phrase_length>(ini, section,
                de_parameters::DE_MAX_TP_LEN_PARAM_NAME);
        ts_params.m_de_params.m_lin_dist_penalty = get_float(ini, section,
                de_parameters::DE_LD_PENALTY_PARAM_NAME);
        ts_params.m_de_params.m_dist_limit = get_integer<int32_t>(ini, section,
                de_parameters::DE_DIST_LIMIT_PARAM_NAME);
        ts_params.m_de_params.m_is_gen_lattice = get_bool(ini, section,
                de_parameters::DE_IS_GEN_LATTICE_PARAM_NAME);
#if IS_SERVER_TUNING_MODE
        ts_params.m_de_params.m_li2n_file_ext = get_string(ini, section,
                de_parameters::DE_LI2N_FILE_EXT_PARAM_NAME);
        //If the lattice dumping is enabled then
        if (ts_params.m_de_params.m_is_gen_lattice) {
            LOG_USAGE << "--------------------------------------------------------" << END_LOG;
            ts_params.m_de_params.m_lattices_folder = get_string(ini, section,
                    de_parameters::DE_LATTICES_FOLDER_PARAM_NAME);
            ts_params.m_de_params.m_scores_file_ext = get_string(ini, section,
                    de_parameters::DE_SCORES_FILE_EXT_PARAM_NAME);
            ts_params.m_de_params.m_lattice_file_ext = get_string(ini, section,
                    de_parameters::DE_LATTICE_FILE_EXT_PARAM_NAME);
            //Check that the lattices folder does, if not - create.
            check_create_folder(ts_params.m_de_params.m_lattices_folder);
        }
        //Create global feature to id mapping, dump the file if needed
        process_feature_to_id_mappings(ts_params, ts_params.m_is_only_f2id, config_file_name);
#endif

        //Only finalize and log parameters if we are not only here to generate the feature to id mapping
        if (!ts_params.m_is_only_f2id) {
            ts_params.finalize();

            //Log the client configuration
            LOG_INFO << ts_params << END_LOG;

            LOG_INFO3 << "Sanity checks are: " << (DO_SANITY_CHECKS ? "ON" : "OFF") << " !" << END_LOG;
        }
    } else {
        //We could not parse the configuration file, report an error
        THROW_EXCEPTION(string("Could not find or parse the configuration file: ") + config_file_name);
    }
}

/**
 * This function tries to extract the 
 * @param argc the number of program arguments
 * @param argv the array of program arguments
 * @param params the structure that will be filled in with the parsed program arguments
 * @return the configuration file name
 */
static void prepare_config_structures(const uint argc, char const * const * const argv, server_parameters & params) {
    //Parse the arguments
    try {
        p_cmd_args->parse(argc, argv);
    } catch (ArgException &e) {
        THROW_EXCEPTION(string("Error: ") + e.error() + string(", for argument: ") + e.argId());
    }

    //Set the logging level right away
    logger::set_reporting_level(p_debug_level_arg->getValue());

    //Get the flag value or use the default
#if IS_SERVER_TUNING_MODE
    params.m_is_only_f2id = p_gen_fmap_arg->getValue();
#else
    params.m_is_only_f2id = false;
#endif

    //Get the configuration file name and read the config values from the file
    const string config_file_name = p_config_file_arg->getValue();
    parse_config_file(config_file_name, params);
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
 * Allows to run the server of the given type
 * @param params the server parameters
 */
template<typename server_type>
static void run_server(server_parameters & params) {
    //Instantiate the translation server
    server_type server(params);

    //Run the translation server in a separate thread
    thread server_thread(bind(&server_type::run, &server));

    LOG_USAGE << "The server is started!" << END_LOG;

    //Wait until the server is stopped by pressing and exit button
    server_console cmd(params, server, server_thread);
    cmd.perform_command_loop();
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

        //Prepare the configuration structures, parse the config file
        prepare_config_structures(argc, argv, params);

        //Run the server only if we are not requested
        //to only generate the feature to id mapping
        if (!params.m_is_only_f2id) {
            //Initialize connections to the used models
            connect_to_models(params);

            //Run the server
            if (params.m_is_tls_server) {
#if defined(WITH_TLS) && WITH_TLS
                run_server<translation_server_tls_mod>(params);
#else
                THROW_EXCEPTION("The server was not build with support TLS!");
#endif
            } else {
                run_server<translation_server_no_tls>(params);
            }

        } else {
            LOG_USAGE << "We were only requested to generate the feature to id mapping, exiting!" << END_LOG;
        }
    } catch (std::exception & ex) {
        //The argument's extraction has failed, print the error message and quit
        LOG_ERROR << ex.what() << END_LOG;
        return_code = 1;
    }

    //Disconnect from the used models
    disconnect_from_models();

    //Destroy the command line parameters parser
    destroy_arguments_parser();

    return return_code;
}

