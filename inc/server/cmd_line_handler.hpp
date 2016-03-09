/* 
 * File:   cmd_line_handler.hpp
 * Author: zapreevis
 *
 * Created on March 9, 2016, 3:42 PM
 */

#ifndef CMD_LINE_HANDLER_HPP
#define	CMD_LINE_HANDLER_HPP

#include "server/server_parameters.hpp"
#include "server/translation_server.hpp"

using namespace uva::smt::bpbd::server;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {

                //Declare the program exit command
                static const string PROGRAM_EXIT_CMD = "q";
                //Declare the program info command
                static const string PROGRAM_INFO_CMD = "i";
                //Declare the program runtime command
                static const string PROGRAM_RUNTIME_CMD = "r";
                //Declare the program nothing command
                static const string PROGRAM_NOTHING_CMD = "";
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
                    LOG_USAGE << "\t'" << PROGRAM_INFO_CMD << " & <enter>'  - print info." << END_LOG;
                    LOG_USAGE << "\t'" << PROGRAM_RUNTIME_CMD << " & <enter>'  -run-time statistics." << END_LOG;
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
                 * Allows to parse the command parameter and return it
                 * @param str the command string
                 * @param prefix the command pregix
                 * @return the parsed value
                 */
                inline int32_t get_int_value(const string & str, const string & prefix) {
                    string value_str = str.substr(prefix.length(), str.length() - prefix.length() + 1);
                    return stoi(value_str);
                }

                /**
                 * Allows to parse the command parameter and return it
                 * @param str the command string
                 * @param prefix the command pregix
                 * @return the parsed value
                 */
                inline float get_float_value(const string & str, const string & prefix) {
                    string value_str = str.substr(prefix.length(), str.length() - prefix.length() + 1);
                    return stof(value_str);
                }

                /**
                 * Allowsto process the command
                 * @params params some server parameters
                 * @param server the server being run
                 * @param server_thread the server thread
                 * @param command the command sting to handle
                 * @return true if we need to stop, otherwise false
                 */
                bool process_input_cmd(server_parameters & params, translation_server &server,
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
                    if (cmd == PROGRAM_INFO_CMD) {
                        print_server_commands();
                        return false;
                    }

                    //Report the runtime information to the console
                    if (cmd == PROGRAM_RUNTIME_CMD) {
                        server.report_run_time_info();
                        return false;
                    }

                    //Set some of the server runtime parameters
                    try {
                        //Get a copy of current decoder parameters
                        de_parameters de_local = params.m_de_params;

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
                                                    THROW_EXCEPTION(string("The command '") + string(command) + string("' is unknown!"));
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
                        params.m_de_params = de_local;
                        
                    } catch (Exception &ex) {
                        LOG_ERROR << ex.get_message() << " Enter '" << PROGRAM_INFO_CMD << "' for help!" << END_LOG;
                    }

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

