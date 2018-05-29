/* 
 * File:   bpbd_balancer.cpp
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
 * Created on July 8, 2016, 10:29 AM
 */

#include <vector>
#include <websocketpp/common/thread.hpp>
#include <tclap/CmdLine.h>

#include "main.hpp"

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"
#include "common/utils/text/string_utils.hpp"

#include "balancer/balancer_console.hpp"
#include "balancer/balancer_parameters.hpp"
#include "balancer/balancer_server.hpp"
#include "balancer/balancer_manager.hpp"

using namespace std;
using namespace TCLAP;

using namespace uva::utils::exceptions;
using namespace uva::utils::logging;
using namespace uva::utils::text;
using namespace uva::smt::bpbd::balancer;
using namespace uva::smt::bpbd::common;

//Include a helper function headers with no other includes or using directives
#include "common/messaging/websocket/server_params_getter.hpp"
#include "common/messaging/websocket/client_params_getter.hpp"

/**
 * This functions does nothing more but printing the program header information
 */
static void print_info() {
    print_info(" The load balancer application    ");
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
    p_config_file_arg = new ValueArg<string>("c", "config", "The configuration file with the balancer options", true, "", "balancer configuration file", *p_cmd_args);

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
 * @return the configuration file name
 */
static void prepare_config_structures(const uint argc, char const * const * const argv, balancer_parameters & bl_params) {
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
        const string section = balancer_parameters::SE_CONFIG_SECTION_NAME;
        bl_params.m_server_port = get_integer<uint16_t>(ini, section,
                balancer_parameters::SE_SERVER_PORT_PARAM_NAME);

        //Parse the TLS server related parameters
        get_server_params(ini, section, bl_params);

        bl_params.m_num_req_threads = get_integer<uint16_t>(ini, section,
                balancer_parameters::SE_NUM_REQ_THREADS_PARAM_NAME);
        bl_params.m_num_resp_threads = get_integer<uint16_t>(ini, section,
                balancer_parameters::SE_NUM_RESP_THREADS_PARAM_NAME);
        bl_params.m_recon_time_out = get_integer<uint32_t>(ini, section,
                balancer_parameters::SC_RECONNECT_TIME_OUT_PARAM_NAME);

        //Get the translation server names
        vector<string> server_names;
        tokenize(get_string(ini, section,
                balancer_parameters::SE_TRANSLATION_SERVER_NAMES_PARAM_NAME),
                server_names, balancer_parameters::TRANS_SERV_NAMES_DELIMITER_STR);

        //Read the translation server names configuration data
        for (auto iter = server_names.begin(); iter != server_names.end(); ++iter) {
            //Get the translation server parameters
            const string & server_name = *iter;

            //Define the translation service parameters object
            trans_server_params ts_params(server_name);

            //Get the WebSocket client related parameters
            get_client_params(ini, server_name, ts_params);

            //Get the load weight parameter value
            ts_params.m_load_weight = get_integer<uint32_t>(ini, server_name,
                    trans_server_params::TC_LOAD_WEIGHT_PARAM_NAME);

            //Add the translator configuration
            bl_params.add_translator(ts_params);
        }

        //Finalize the parameters
        bl_params.finalize();

        //Log the server configuration
        LOG_INFO << bl_params << END_LOG;

        LOG_INFO3 << "Sanity checks are: " << (DO_SANITY_CHECKS ? "ON" : "OFF") << " !" << END_LOG;
    } else {
        //We could not parse the configuration file, report an error
        THROW_EXCEPTION(string("Could not find or parse the configuration file: ") + config_file_name);
    }
}

/**
 * Allows to run the server of the given type
 * @param params the server parameters
 */
template<typename server_type>
static void run_server(balancer_parameters & params) {
    //Instantiate the balancer server
    server_type server(params);

    LOG_USAGE << "Running the balancer server ..." << END_LOG;

    //Run the translation server in a separate thread
    thread balancer_thread(bind(&server_type::run, &server));

    LOG_USAGE << "The balancer is started!" << END_LOG;

    //Wait until the balancer is stopped by pressing and exit button
    balancer_console cmd(params, server, balancer_thread);
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
        balancer_parameters params;

        //Prepare the configuration structures, parse the config file
        prepare_config_structures(argc, argv, params);

        //Run the server
        if (params.m_is_tls_server) {
#if defined(WITH_TLS) && WITH_TLS
            run_server<balancer_server_tls_mod>(params);
#else
            THROW_EXCEPTION("The server was not build with support TLS!");
#endif
        } else {
            run_server<balancer_server_no_tls>(params);
        }
    } catch (std::exception & ex) {
        //The argument's extraction has failed, print the error message and quit
        LOG_ERROR << ex.what() << END_LOG;
        return_code = 1;
    }
    LOG_USAGE << "The balancer is stopped!" << END_LOG;

    //Destroy the command line parameters parser
    destroy_arguments_parser();

    return return_code;
}