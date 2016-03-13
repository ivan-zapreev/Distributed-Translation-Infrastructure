/* 
 * File:   cmd_line_handler.hpp
 * Author: zapreevis
 *
 * Created on March 9, 2016, 3:42 PM
 */

#ifndef CMD_LINE_HANDLER_HPP
#define	CMD_LINE_HANDLER_HPP

#include "common/utils/logging/logger.hpp"

#include "server/server_parameters.hpp"
#include "server/translation_server.hpp"

using namespace uva::utils::logging;

using namespace uva::smt::bpbd::server;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {

                //Declare the program exit command
                static const string PROGRAM_EXIT_CMD = "q";
                //Declare the program help command
                static const string PROGRAM_HELP_CMD = "h";
                //Declare the program runtime command
                static const string PROGRAM_RUNTIME_CMD = "r";
                //Declare the command for parameters logging
                static const string PROGRAM_PARAMS_CMD = "p";
                //Declare the program nothing command
                static const string PROGRAM_NOTHING_CMD = "";
                //The number of threads setting command
                static const string PROGRAM_SET_NT_CMD = "set nt ";
                //The log level setting command
                static const string PROGRAM_SET_LL_CMD = "set ll ";
                //Declare the program "set" commands, NOTE the end spaces are needed!
                static const string PROGRAM_SET_NBT_CMD = "set nbt ";
                static const string PROGRAM_SET_D_CMD = "set d ";
                static const string PROGRAM_SET_EDL_CMD = "set edl ";
                static const string PROGRAM_SET_PT_CMD = "set pt ";
                static const string PROGRAM_SET_SC_CMD = "set sc ";
                static const string PROGRAM_SET_WP_CMD = "set wp ";
                static const string PROGRAM_SET_PP_CMD = "set pp ";

                /**
                 * Allows to stop the server;
                 * @param server the server being run
                 * @param server_thread the server thread
                 */
                void stop(translation_server &server, thread &server_thread) {
                    //Stop the translation server
                    LOG_USAGE << "Stopping the server ..." << END_LOG;
                    server.stop();

                    //Wait until the server's thread stops
                    server_thread.join();

                    LOG_USAGE << "The server has stopped!" << END_LOG;
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
                    LOG_USAGE << "\t'" << PROGRAM_EXIT_CMD << " & <enter>'  - to exit." << END_LOG;
                    LOG_USAGE << "\t'" << PROGRAM_HELP_CMD << " & <enter>'  - print HELP info." << END_LOG;
                    LOG_USAGE << "\t'" << PROGRAM_RUNTIME_CMD << " & <enter>'  - run-time statistics." << END_LOG;
                    LOG_USAGE << "\t'" << PROGRAM_PARAMS_CMD << " & <enter>'  - print server parameters." << END_LOG;
                    LOG_USAGE << "\t'" << PROGRAM_SET_LL_CMD << "<level> & <enter>'  - set log level." << END_LOG;
                    LOG_USAGE << "\t'" << PROGRAM_SET_NT_CMD << " <positive integer> & <enter>'  - set the number of worker threads." << END_LOG;
                    LOG_USAGE << "\t'" << PROGRAM_SET_NBT_CMD << "<unsigned integer> & <enter>'  - set the number of best translations." << END_LOG;
                    LOG_USAGE << "\t'" << PROGRAM_SET_D_CMD << "<integer> & <enter>'  - set the distortion limit." << END_LOG;
                    LOG_USAGE << "\t'" << PROGRAM_SET_EDL_CMD << "<unsigned integer> & <enter>'  - set the extra left distortion." << END_LOG;
                    LOG_USAGE << "\t'" << PROGRAM_SET_PT_CMD << "<unsigned float> & <enter>'  - set pruning threshold." << END_LOG;
                    LOG_USAGE << "\t'" << PROGRAM_SET_SC_CMD << "<integer> & <enter>'  - set stack capacity." << END_LOG;
                    LOG_USAGE << "\t'" << PROGRAM_SET_WP_CMD << "<float> & <enter>'  - set word penalty." << END_LOG;
                    LOG_USAGE << "\t'" << PROGRAM_SET_PP_CMD << "<float> & <enter>'  - set phrase penalty." << END_LOG;
                }

                //Stores the command buffer size
                static constexpr size_t CMD_BUFF_SIZE = 256;

                /**
                 * Allows to test if a string begins with a substring
                 * @param str the string to check
                 * @param the prefix
                 * @return true if the string begins with the prefix
                 */
                inline bool begins_with(const string & str, const string & prefix) {
                    return (str.compare(0, prefix.length(), prefix) == 0);
                }

                /**
                 * Allows to parse the command parameter and return it as a string
                 * @param str the command string
                 * @param prefix the command pregix
                 * @return the parsed value
                 */
                inline string get_string_value(const string & str, const string & prefix) {
                    return str.substr(prefix.length(), str.length() - prefix.length() + 1);
                }

                /**
                 * Allows to parse the command parameter and return it
                 * @param str the command string
                 * @param prefix the command pregix
                 * @return the parsed value
                 */
                inline int32_t get_int_value(const string & str, const string & prefix) {
                    //Get the string value to be parsed
                    string str_val = get_string_value(str, prefix);
                    try {
                        return stoi(str_val);
                    } catch (std::invalid_argument & ex1) {
                    } catch (std::out_of_range & ex2) {
                    }

                    //Throw an exception
                    THROW_EXCEPTION(string("Could not parse: ") + str_val);
                }

                /**
                 * Allows to parse the command parameter and return it
                 * @param str the command string
                 * @param prefix the command pregix
                 * @return the parsed value
                 */
                inline float get_float_value(const string & str, const string & prefix) {
                    //Get the string value to be parsed
                    string str_val = get_string_value(str, prefix);
                    try {
                        return stof(str_val);
                    } catch (std::invalid_argument & ex1) {
                    } catch (std::out_of_range & ex2) {
                    }

                    //Throw an exception
                    THROW_EXCEPTION(string("Could not parse: ") + str_val);
                }

                /**
                 * Allows to set the debug level
                 * @param cmd the debug level
                 */
                inline void set_log_level(const string & cmd, const string & prefix) {
                    logger::set_reporting_level(get_string_value(cmd, prefix));
                }

                /**
                 * Allows to set the number of worker threads
                 * @param params the server parameters
                 * @param server the translation server
                 * @param cmd the input command 
                 * @param prefix the command prefix
                 */
                inline void set_num_threads(server_parameters & params,
                        translation_server &server,
                        const string & cmd, const string & prefix) {
                    try {
                        //Get the specified number of threads
                        int32_t num_threads = get_int_value(cmd, prefix);
                        ASSERT_CONDITION_THROW((num_threads <= 0),
                                "The number of worker threads is to be > 0!");

                        if (((size_t) num_threads) != params.m_num_threads) {
                            //Set the number of threads
                            server.set_num_threads(num_threads);
                            //Remember the new number of threads
                            params.m_num_threads = num_threads;
                        } else {
                            LOG_WARNING << "The number of worker threads is already: "
                                    << num_threads << "!" << END_LOG;
                        }
                    } catch (uva_exception &ex) {
                        LOG_ERROR << ex.get_message() << "\nEnter '" << PROGRAM_HELP_CMD << "' for help!" << END_LOG;
                    }
                }

                /**
                 * Allows to set some decoder parameters
                 * @param cmd the command to process, if not a command for
                 * setting decoder parameters an error will be reported.
                 * @param m_de_params the reference to the decoder parameters to set with new values.
                 */
                inline void set_decoder_params(const string & cmd, de_parameters & de_params) {
                    //Set some of the server runtime parameters
                    try {
                        //Get a copy of current decoder parameters
                        de_parameters de_local = de_params;

                        if (begins_with(cmd, PROGRAM_SET_NBT_CMD)) {
                            de_local.m_num_best_trans = get_int_value(cmd, PROGRAM_SET_NBT_CMD);
                        } else {
                            if (begins_with(cmd, PROGRAM_SET_D_CMD)) {
                                de_local.m_distortion = get_int_value(cmd, PROGRAM_SET_D_CMD);
                            } else {
                                if (begins_with(cmd, PROGRAM_SET_EDL_CMD)) {
                                    de_local.m_ext_dist_left = get_int_value(cmd, PROGRAM_SET_EDL_CMD);
                                } else {
                                    if (begins_with(cmd, PROGRAM_SET_PT_CMD)) {
                                        de_local.m_pruning_threshold = get_float_value(cmd, PROGRAM_SET_PT_CMD);
                                    } else {
                                        if (begins_with(cmd, PROGRAM_SET_SC_CMD)) {
                                            de_local.m_stack_capacity = get_int_value(cmd, PROGRAM_SET_SC_CMD);
                                        } else {
                                            if (begins_with(cmd, PROGRAM_SET_WP_CMD)) {
                                                de_local.m_word_penalty = get_float_value(cmd, PROGRAM_SET_WP_CMD);
                                            } else {
                                                if (begins_with(cmd, PROGRAM_SET_PP_CMD)) {
                                                    de_local.m_phrase_penalty = get_float_value(cmd, PROGRAM_SET_PP_CMD);
                                                } else {
                                                    THROW_EXCEPTION(string("The command '") + cmd + string("' is unknown!"));
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        //Finalize the parameters
                        de_local.finalize();

                        //Set the parameters back
                        de_params = de_local;

                    } catch (uva_exception &ex) {
                        LOG_ERROR << ex.get_message() << " Enter '" << PROGRAM_HELP_CMD << "' for help!" << END_LOG;
                    }
                }

                /**
                 * Allowsto process the command
                 * @params params some server parameters
                 * @param server the server being run
                 * @param server_thread the server thread
                 * @param command the command sting to handle
                 * @return true if we need to stop, otherwise false
                 */
                inline bool process_input_cmd(server_parameters & params, translation_server &server,
                        thread &server_thread, char command[CMD_BUFF_SIZE]) {
                    //Convert the buffer into string
                    string cmd(command);

                    //We are to print the command prompt
                    if (cmd == PROGRAM_NOTHING_CMD) {
                        return false;
                    }

                    //Stop the server
                    if (cmd == PROGRAM_EXIT_CMD) {
                        stop(server, server_thread);
                        return true;
                    }

                    //Print the server commands menu
                    if (cmd == PROGRAM_HELP_CMD) {
                        print_server_commands();
                        return false;
                    }

                    //Report the runtime information to the console
                    if (cmd == PROGRAM_RUNTIME_CMD) {
                        server.report_run_time_info();
                        return false;
                    }

                    //Lor parameters
                    if (cmd == PROGRAM_PARAMS_CMD) {
                        LOG_USAGE << "Log level: " << logger::get_curr_level_str()
                                << ", " << params << END_LOG;
                        return false;
                    }

                    //Set the debug level
                    if (begins_with(cmd, PROGRAM_SET_LL_CMD)) {
                        set_log_level(cmd, PROGRAM_SET_LL_CMD);
                        return false;
                    }

                    //Set the number of threads
                    if (begins_with(cmd, PROGRAM_SET_NT_CMD)) {
                        set_num_threads(params, server, cmd, PROGRAM_SET_LL_CMD);
                        return false;
                    }

                    //Set other decoder parameters
                    set_decoder_params(cmd, params.m_de_params);

                    //Continue to the next command.
                    return false;
                }

                /**
                 * Runs the server's command loop
                 * @param params some server params
                 * @param server the server being run
                 * @param server_thread the server thread
                 */
                void perform_command_loop(server_parameters & params, translation_server &server, thread &server_thread) {
                    //Command buffer
                    char command[CMD_BUFF_SIZE];

                    //Print the server commands menu
                    print_server_commands();

                    //Print the prompt
                    print_the_prompt();

                    //Perform infinite looping processing commands until we shall quit.
                    while (true) {
                        //Wait for the input
                        cin.getline(command, CMD_BUFF_SIZE);

                        //Process input
                        if (process_input_cmd(params, server, server_thread, command)) {
                            return;
                        }

                        //Print the prompt
                        print_the_prompt();
                    }
                }
            }
        }
    }
}

#endif	/* CMD_LINE_HANDLER_HPP */

