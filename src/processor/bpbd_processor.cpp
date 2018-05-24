/* 
 * File:   bpbd_processor.cpp
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
 * Created on July 25, 2016, 11:42 AM
 */

#include <string>
#include <vector>
#include <websocketpp/common/thread.hpp>
#include <tclap/CmdLine.h>

#include "main.hpp"

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"
#include "common/utils/text/string_utils.hpp"
#include "common/utils/file/utils.hpp"

#include "processor/processor_console.hpp"
#include "processor/processor_parameters.hpp"
#include "processor/processor_server.hpp"
#include "processor/processor_manager.hpp"

using namespace std;
using namespace TCLAP;

using namespace uva::utils::exceptions;
using namespace uva::utils::logging;
using namespace uva::utils::text;
using namespace uva::utils::file;

using namespace uva::smt::bpbd::processor;
using namespace uva::smt::bpbd::common;

//Add the TLS server as an option in case the TLS support is enabled
#if defined(WITH_TLS) && WITH_TLS
typedef processor_server<websocketpp::config::asio_tls> processor_server_tls;
#endif
typedef processor_server<websocketpp::config::asio> processor_server_no_tls;

/**
 * This functions does nothing more but printing the program header information
 */
static void print_info() {
    print_info(" The text processor application   ");
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
    p_config_file_arg = new ValueArg<string>("c", "config", "The configuration file with the processor options", true, "", "processor configuration file", *p_cmd_args);

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
static void prepare_config_structures(const uint argc, char const * const * const argv, processor_parameters & params) {
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
    LOG_USAGE << "Loading the processor configuration option from: " << config_file_name << END_LOG;
    INI<> ini(config_file_name, false);

    //Parse the configuration file
    if (ini.parse()) {
        LOG_INFO << "The configuration file has been parsed!" << END_LOG;

        //Get the configuration options from the file
        const string section = processor_parameters::SE_CONFIG_SECTION_NAME;
        params.m_server_port = get_integer<uint16_t>(ini, section, processor_parameters::SE_SERVER_PORT_PARAM_NAME);
        params.m_is_tls_server = get_bool(ini, section, processor_parameters::SE_IS_TLS_SERVER_PARAM_NAME, "false", false);
        params.m_num_threads = get_integer<uint16_t>(ini, section, processor_parameters::SE_NUM_THREADS_PARAM_NAME);
        params.m_work_dir = get_string(ini, section, processor_parameters::SE_WORK_DIR_PARAM_NAME);
        string def_pre_call_templ = get_string(ini, section, processor_parameters::SE_PRE_CALL_TEMPL_PARAM_NAME, "", false);
        string def_post_call_templ = get_string(ini, section, processor_parameters::SE_POST_CALL_TEMPL_PARAM_NAME, "", false);
        params.set_processors(def_pre_call_templ, def_post_call_templ);

        //Finalize the parameters
        params.finalize();
        
        //Check that the lattices folder does, if not - create.
        check_create_folder(params.m_work_dir);
        
        //Log the server configuration
        LOG_INFO << params << END_LOG;

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
static void run_server(processor_parameters & params) {
        //Instantiate the balancer server
        server_type server(params);

        LOG_USAGE << "Running the processor server ..." << END_LOG;

        //Run the translation server in a separate thread
        thread balancer_thread(bind(&server_type::run, &server));

        LOG_USAGE << "The processor is started!" << END_LOG;

        //Wait until the balancer is stopped by pressing and exit button
        processor_console cmd(params, server, balancer_thread);
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
        processor_parameters params;

        //Prepare the configuration structures, parse the config file
        prepare_config_structures(argc, argv, params);
        
        //Run the server
        if (params.m_is_tls_server) {
#if defined(WITH_TLS) && WITH_TLS
            run_server<processor_server_tls>(params);
#else
            THROW_EXCEPTION("The server was not build with support TLS!");
#endif
        } else {
            run_server<processor_server_no_tls>(params);
        }
    } catch (std::exception & ex) {
        //The argument's extraction has failed, print the error message and quit
        LOG_ERROR << ex.what() << END_LOG;
        return_code = 1;
    }
    LOG_USAGE << "The processor is stopped!" << END_LOG;

    //Destroy the command line parameters parser
    destroy_arguments_parser();

    return return_code;
}

