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
#include <sstream>

#include "tclap/CmdLine.h"
#include "md5.hpp"

#include "main.hpp"

#include "client/client_consts.hpp"
#include "client/client_parameters.hpp"
#include "client/trans_manager.hpp"
#include "client/proc_manager.hpp"

#include "common/utils/file/cstyle_file_reader.hpp"
#include "common/utils/exceptions.hpp"
#include "common/utils/text/string_utils.hpp"

using namespace std;

using namespace TCLAP;

using namespace uva::utils::exceptions;
using namespace uva::utils::file;
using namespace uva::utils::text;

using namespace uva::smt::bpbd::client;

using namespace uva::smt::bpbd::common;
using namespace uva::smt::bpbd::common::messaging;

//Include a helper function headers with no other includes or using directives
#include "common/messaging/websocket/client_params_getter.hpp"

typedef stringstream * stringstream_ptr;

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

static ValueArg<uint32_t> * p_max_sent = NULL;
static ValueArg<uint32_t> * p_min_sent = NULL;
static ValueArg<int32_t> * p_priority = NULL;
static SwitchArg * p_trans_info_arg = NULL;

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

    //Add the minimum number of sentences to send by a job parameter - optional, by default is 10
    p_min_sent = new ValueArg<uint32_t>("l", "lower-size", string("The minimum number of sentences ") +
            string("to send per request, default is ") + to_string(client_parameters::CL_DEF_MIN_SENT_VAL),
            false, client_parameters::CL_DEF_MIN_SENT_VAL, "min #sentences per request", *p_cmd_args);

    //Add the maximum number of sentences to send by a job parameter - optional, by default is 100
    p_max_sent = new ValueArg<uint32_t>("u", "upper-size", string("The maximum number of sentences ") +
            string("to send per request, default is ") + to_string(client_parameters::CL_DEF_MAX_SENT_VAL),
            false, client_parameters::CL_DEF_MAX_SENT_VAL, "max #sentences per request", *p_cmd_args);

    //Add the translation priority optional parameter
    p_priority = new ValueArg<int32_t>("s", "seniority", string("The priority of the translation task ") +
            string("is a 32 bit integer, the default is ") + to_string(client_parameters::CL_DEF_PRIORITY_VAL),
            false, client_parameters::CL_DEF_PRIORITY_VAL, "the translation priority", *p_cmd_args);

    //Add the translation details switch parameter - ostring(optional, default is false
    p_trans_info_arg = new SwitchArg("f", "info", string("Request the server to provide ") +
            string("information about the translation process"), *p_cmd_args, client_parameters::CL_DEF_IS_TRANS_INFO_VAL);

    //Add the configuration file parameter - compulsory
    p_config_file_arg = new ValueArg<string>("c", "config", "The configuration file with the client options",
            false, "", "client configuration file", *p_cmd_args);

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

    SAFE_DESTROY(p_max_sent);
    SAFE_DESTROY(p_min_sent);
    SAFE_DESTROY(p_priority);
    SAFE_DESTROY(p_trans_info_arg);

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
static void extract_arguments(const uint argc, char const * const * const argv, client_parameters & tc_params) {
    //Parse the arguments
    try {
        p_cmd_args->parse(argc, argv);
    } catch (ArgException &e) {
        THROW_EXCEPTION(string("Error: ") + e.error() + string(", for argument: ") + e.argId());
    }

    //Set the logging level right away
    logger::set_reporting_level(p_debug_level_arg->getValue());

    //Get the command-line-only parameter values
    tc_params.m_source_file = p_source_file_arg->getValue();
    tc_params.m_source_lang = p_source_lang_arg->getValue();
    tc_params.m_target_file = p_target_file_arg->getValue();
    tc_params.m_target_lang = p_target_lang_arg->getValue();

    //Check if the configuration file is provided
    if (p_config_file_arg->isSet()) {
        //Get the configuration file name
        const string config_file_name = p_config_file_arg->getValue();

        LOG_USAGE << "Loading the server configuration option from: "
                << config_file_name << END_LOG;

        //Parse the configuration file
        INI<> ini(config_file_name, false);
        if (ini.parse()) {
            //Get the common client parameters from the configuration file
            const string & section = client_parameters::CL_CONFIG_SECTION_NAME;
            tc_params.m_min_sent = get_integer<uint32_t>(ini, section,
                    client_parameters::CL_MIN_SENT_PARAM_NAME, client_parameters::CL_DEF_MIN_SENT_VAL, false);
            tc_params.m_max_sent = get_integer<uint32_t>(ini, section,
                    client_parameters::CL_MAX_SENT_PARAM_NAME, client_parameters::CL_DEF_MAX_SENT_VAL, false);
            tc_params.m_priority = get_integer<int32_t>(ini, section,
                    client_parameters::CL_PRIORITY_PARAM_NAME, client_parameters::CL_DEF_PRIORITY_VAL, false);
            tc_params.m_is_trans_info = get_bool(ini, section,
                    client_parameters::CL_IS_TRANS_INFO_PARAM_NAME, client_parameters::CL_DEF_IS_TRANS_INFO_VAL, false);

            //Parse the pre-processor server related parameters
            get_client_params<true>(ini,
                    client_parameters::CL_PRE_PARAMS_SECTION_NAME,
                    tc_params.m_pre_params, client_parameters::CL_DEF_PRE_PROC_URI);
            //Parse the translation server related parameters
            get_client_params<true>(ini,
                    client_parameters::CL_TRANS_PARAMS_SECTION_NAME,
                    tc_params.m_trans_params, client_parameters::CL_DEF_TRANS_URI);
            //Parse the post-processor server related parameters
            get_client_params<true>(ini,
                    client_parameters::CL_POST_PARAMS_SECTION_NAME,
                    tc_params.m_post_params, client_parameters::CL_DEF_POST_PROC_URI);
        } else {
            //We could not parse the configuration file, report an error
            THROW_EXCEPTION(string("Could not find or parse the configuration file: ") + config_file_name);
        }

        //After the configuration file is read, update the parameters from those
        //that can be duplicated on the command-line, this has a higher priority.
        if (p_min_sent->isSet()) {
            tc_params.m_min_sent = p_min_sent->getValue();
        }
        if (p_max_sent->isSet()) {
            tc_params.m_max_sent = p_max_sent->getValue();
        }
        if (p_priority->isSet()) {
            tc_params.m_priority = p_priority->getValue();
        }
        if (p_trans_info_arg->isSet()) {
            tc_params.m_is_trans_info = p_trans_info_arg->getValue();
        }
    } else {
        //Initialize the default parameters
        tc_params.m_pre_params.m_server_uri = client_parameters::CL_DEF_PRE_PROC_URI;
        tc_params.m_pre_params.m_tls_mode_name = client_parameters::CL_DEF_TLS_MODE;
        tc_params.m_pre_params.m_tls_ciphers = client_parameters::CL_DEF_TLS_CIPHERS;

        tc_params.m_trans_params.m_server_uri = client_parameters::CL_DEF_TRANS_URI;
        tc_params.m_trans_params.m_tls_mode_name = client_parameters::CL_DEF_TLS_MODE;
        tc_params.m_trans_params.m_tls_ciphers = client_parameters::CL_DEF_TLS_CIPHERS;

        tc_params.m_post_params.m_server_uri = client_parameters::CL_DEF_POST_PROC_URI;
        tc_params.m_post_params.m_tls_mode_name = client_parameters::CL_DEF_TLS_MODE;
        tc_params.m_post_params.m_tls_ciphers = client_parameters::CL_DEF_TLS_CIPHERS;

        //Read the other command line parameters
        tc_params.m_min_sent = p_min_sent->getValue();
        tc_params.m_max_sent = p_max_sent->getValue();
        tc_params.m_priority = p_priority->getValue();
        tc_params.m_is_trans_info = p_trans_info_arg->getValue();
    }

    //Finalize the results
    tc_params.finalize();

    //Log the client configuration
    LOG_INFO << tc_params << END_LOG;

    LOG_INFO3 << "Sanity checks are: " << (DO_SANITY_CHECKS ? "ON" : "OFF") << " !" << END_LOG;
}

/**
 * Allows to populate the input stream with the source text
 * @param params the client parameters
 * @param output the stream to put the text into
 */
template<bool IS_PRE_PROCESS = false >
void read_input(const client_parameters & params, stringstream_ptr & output) {
    //Open the input file with the source text
    cstyle_file_reader source_file(params.m_source_file);

    //If the input file could not be opened, we through!
    ASSERT_CONDITION_THROW(!source_file.is_open(),
            string("Could not open the source text file: ") + params.m_source_file);

    //Log the source file reader info
    source_file.log_reader_type_info();

    LOG_USAGE << "Reading source text from '" << params.m_source_file << "'" << END_LOG;

    //Declare the variable to store the sentence line
    text_piece_reader line;

    //Read the file line by line and place it into the buffer
    while (source_file.get_first_line(line)) {
        //Get the line to work with
        string text = line.has_more() ? line.str() : "";

        //Log the read line
        LOG_DEBUG1 << "Source text line: '" << text << "'" << END_LOG;

        //If no pre-processor is specified then try to reduce the string as
        //there can be occasionally improper white spaces or too many!
        *output << (IS_PRE_PROCESS ? text : reduce(text)) << std::endl;
    }

    //Close the source file
    source_file.close();
}

/**
 * Allows to compute the initial job token to be the md5 sum of the source file.
 * @param params [in] the parameters from which the input file is to be detected
 * @param job_token [out] the string storing the md5 sum of the source file
 */
void get_job_token(const client_parameters & params, string & job_token) {
    //Declare the md5 class instance
    MD5 md5;

    //Compute the md5 sum of the file
    char * sum = md5.digestFile(params.m_source_file.c_str());

    //Set the resulting md5 sum
    job_token = string(sum);
}

/**
 * Allows to perform the text pre-processing if needed
 * @param params the client parameters
 * @param input the input stream storing the text to pre-process
 * @param output the output stream to write the pre-processed text into
 * @param job_token [in/out] the job token will be updated by the server
 */
void pre_process(client_parameters & params, stringstream_ptr & input,
        stringstream_ptr & output, string & job_token) {
    //Check if pre-processing is needed
    if (params.is_pre_process()) {
        //Create the pre-processor manager
        pre_proc_manager manager(params, *input, *output,
                params.m_source_lang, job_token);

        //Start the pre-processor manager
        LOG_USAGE << "Starting the pre-processor process ..." << END_LOG;
        manager.start();

        //Wait until the pre-processor are done
        LOG_USAGE << "Waiting for the pre-processor process to finish ..." << END_LOG;
        manager.wait();

        //Stop the pre-processor manager
        LOG_USAGE << "Finishing the pre-processor process ..." << END_LOG;
        manager.stop();

        LOG_USAGE << "The pre-processor process is finished" << END_LOG;

        //Re-set the stream as we are going to use it
        input->clear();

        //Log the source language
        LOG_INFO1 << "The source language is: '" << params.m_source_lang << "'" << END_LOG;
    } else {
        //There is no need to pre-process. Swap the streams,
        //as the input is to be "placed" into the output.
        stringstream_ptr tmp = output;
        output = input;
        input = tmp;
    }
}

/**
 * Allows to perform the text translation
 * @param params the client parameters
 * @param input the input stream storing the text to translate
 * @param output the output stream to write the translated text into
 */
void translate(const client_parameters & params, stringstream_ptr & input,
        stringstream_ptr & output) {
    //Create the translation manager
    trans_manager manager(params, *input, *output);

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

    //Re-set the stream as we are going to use it
    input->clear();
}

/**
 * Allows to perform the text post-processing if needed
 * @param params the client parameters
 * @param input the input stream storing the text to post-process
 * @param output the output stream to write the post-processed text into
 * @param job_token [in/out] the job token can be updated by the server
 */
void post_process(client_parameters & params, stringstream_ptr & input,
        stringstream_ptr & output, string & job_token) {
    //Check if post-processing is needed
    if (params.is_post_process()) {
        //Create the post-processor manager
        post_proc_manager manager(params, *input, *output,
                params.m_target_lang, job_token);

        //Start the post-processor manager
        LOG_USAGE << "Starting the post-processor process ..." << END_LOG;
        manager.start();

        //Wait until the post-processor are done
        LOG_USAGE << "Waiting for the post-processor process to finish ..." << END_LOG;
        manager.wait();

        //Stop the post-processor manager
        LOG_USAGE << "Finishing the post-processor process ..." << END_LOG;
        manager.stop();

        LOG_USAGE << "The post-processor process is finished" << END_LOG;

        //Re-set the stream as we are going to use it
        input->clear();
    } else {
        //There is no need to post-process. Swap the streams,
        //as the input is to be "placed" into the output.
        stringstream_ptr tmp = output;
        output = input;
        input = tmp;
    }
}

/**
 * Allows to write the data from the output stream into the target text
 * @param params the client parameters
 * @param input the stream to get the text from
 */
void write_output(const client_parameters & params, stringstream_ptr & input) {
    //Open the output file to store the target text
    ofstream trans_file(params.m_target_file);

    //If the output file could not be opened, we through!
    ASSERT_CONDITION_THROW(!trans_file.is_open(),
            string("Could not open: ") + params.m_target_file);

    LOG_USAGE << "Dumping translation into '" << params.m_target_file << "'" << END_LOG;

    //Read the file line by line and place it into the buffer
    char buffer[LINE_MAX_BYTES_LEN];
    while (input->getline(buffer, sizeof (buffer))) {
        trans_file << buffer << std::endl;
    }

    //Close the source file
    trans_file.close();
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

    //Define the string streams for the tasks.
    stringstream str_a, str_b;
    stringstream_ptr pipe_a = &str_a;
    stringstream_ptr pipe_b = &str_b;

    //Define the string job uid that is first issued
    //by the client and is then updated by the server.
    string job_uid;

    try {
        //Define en empty parameters structure
        client_parameters params = {};

        //Attempt to extract the program arguments
        extract_arguments(argc, argv, params);

        //Read from the source file: file -> pipe_a
        if (params.is_pre_process()) {
            read_input<true>(params, pipe_a);
        } else {
            read_input<false>(params, pipe_a);
        }

        //Compute the first job uid for the pre-processor request
        get_job_token(params, job_uid);

        //Pre-process text if requested: pipe_a -> pipe_b
        pre_process(params, pipe_a, pipe_b, job_uid);

        //Translate text: pipe_b -> pipe_a
        translate(params, pipe_b, pipe_a);

        //Post-process text if requested: pipe_a -> pipe_b
        post_process(params, pipe_a, pipe_b, job_uid);

        //Write the result to target file: pipe_b -> file
        write_output(params, pipe_b);
    } catch (std::exception & ex) {
        //The argument's extraction has failed, print the error message and quit
        LOG_ERROR << ex.what() << END_LOG;
        return_code = 1;
    }

    //Destroy the command line parameters parser
    destroy_arguments_parser();

    return return_code;
}

