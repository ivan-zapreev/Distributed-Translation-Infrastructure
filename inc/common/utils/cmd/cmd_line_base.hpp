/* 
 * File:   cmd_line_base.hpp
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
 * Created on July 7, 2016, 2:27 PM
 */

#ifndef CMD_LINE_BASE_HPP
#define CMD_LINE_BASE_HPP

#include <iostream>

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"

using namespace std;

using namespace uva::utils::logging;
using namespace uva::utils::exceptions;

namespace uva {
    namespace utils {
        namespace cmd {

            //Declare the program exit command
            static const string PROGRAM_EXIT_CMD = "q";
            //Declare the program help command
            static const string PROGRAM_HELP_CMD = "h";
            //Declare the program runtime command
            static const string PROGRAM_RUNTIME_CMD = "r";
            //Declare the command for parameters logging
            static const string PROGRAM_PARAMS_CMD = "p";
            //The log level setting command
            static const string PROGRAM_SET_LL_CMD = "set ll ";
            //Declare the program nothing command
            static const string PROGRAM_NOTHING_CMD = "";

            /**
             * This is the baseline class for the command line handlers.
             * It contains some common functionality needed to implement
             * a trivial command line.
             */
            class cmd_line_base {
            public:

                //Stores the command buffer size
                static constexpr size_t CMD_BUFF_SIZE = 256;

                /**
                 * The basic constructor
                 */
                cmd_line_base() : m_is_stopped(false) {
                }

                /**
                 * The basic destructor
                 */
                virtual ~cmd_line_base() {
                }

                /**
                 * Runs the command-line loop
                 */
                inline void perform_command_loop() {
                    //Command buffer
                    char command[CMD_BUFF_SIZE];

                    LOG_USAGE << "--------------------------------------------------------" << END_LOG;

                    //Print the server commands menu
                    print_commands_help();

                    //Print the prompt
                    print_the_prompt();

                    //Perform infinite looping processing commands until we shall quit.
                    while (true) {
                        //Wait for the input
                        cin.getline(command, CMD_BUFF_SIZE);

                        //Process input
                        if (process_input_cmd(command)) {
                            return;
                        }

                        //Print the prompt
                        print_the_prompt();
                    }
                }

            protected:
                //Stores the flag indicating if the console was stopped or not
                bool m_is_stopped;

                /**
                 * Allows to print a single command help
                 * @param cmd the command string
                 * @param params the command parameters
                 * @param help the help message
                 */
                inline void print_command_help(const string & cmd, const string & params, const string & help) {
                    LOG_USAGE << "\t'" << cmd << " " << params << (params.empty() ? "" : " ")
                            << "& <enter>'  - " << help << "." << END_LOG;
                }

                /**
                 * Prints the available specific commands
                 */
                virtual void print_specific_commands() = 0;

                /**
                 * Allows to report the program run-time information, 
                 * is to be implemented by the child class.
                 */
                virtual void report_run_time_info() = 0;

                /**
                 * Allows to handle some specific command, is to be implemented
                 * by the child class.
                 * @param cmd the so-far unrecognized command to be processed
                 * @return true if the command could not be recognized, otherwise false!
                 */
                virtual bool process_specific_cmd(const string & cmd) = 0;

                /**
                 * Is to be called when one needs to get the program parameters reported.
                 * Is to be implemented by the child class.
                 */
                virtual void report_program_params() = 0;

                /**
                 * The stop function that will be called when the application is
                 * requested to be stopped, is to be implemented by the child class
                 */
                virtual void stop() = 0;

                /**
                 * Allows to test if a string begins with a substring
                 * @param str the string to check
                 * @param prefix the prefix
                 * @return true if the string begins with the prefix
                 */
                inline bool begins_with(const string & str, const string & prefix) {
                    return (str.compare(0, prefix.length(), prefix) == 0);
                }

                /**
                 * Allows to parse the command parameter and return it as a string
                 * @param str the command string
                 * @param prefix the command prefix
                 * @return the parsed value
                 */
                inline string get_string_value(const string & str, const string & prefix) {
                    return str.substr(prefix.length(), str.length() - prefix.length() + 1);
                }

                /**
                 * Allows to parse the command parameter and return it
                 * @param str the command string
                 * @param prefix the command prefix
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
                    THROW_EXCEPTION(string("Could not parse integer: ") + str_val);
                }

                /**
                 * Allows to parse the command parameter and return it
                 * @param str the command string
                 * @param prefix the command prefix
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
                    THROW_EXCEPTION(string("Could not parse float: ") + str_val);
                }

                /**
                 * Allows to parse the command parameter and return it
                 * @param str the command string
                 * @param prefix the command prefix
                 * @return the parsed value
                 */
                inline bool get_bool_value(const string & str, const string & prefix) {
                    //Get the string value to be parsed
                    string str_val = get_string_value(str, prefix);

                    if (str_val == "true") {
                        return true;
                    } else {
                        if (str_val == "false") {
                            return false;
                        }
                    }

                    //Throw an exception
                    THROW_EXCEPTION(string("Could not parse boolean: ") + str_val);
                }

            private:

                /**
                 * Prints the available commands. Is to be overloaded by the concrete implementation class.
                 */
                void print_commands_help() {
                    LOG_USAGE << "General console commands: " << END_LOG;
                    print_command_help(PROGRAM_EXIT_CMD, "", "to exit");
                    print_command_help(PROGRAM_HELP_CMD, "", "print HELP info");
                    print_command_help(PROGRAM_RUNTIME_CMD, "", "run-time statistics");
                    print_command_help(PROGRAM_PARAMS_CMD, "", "print program parameters");
                    print_command_help(PROGRAM_SET_LL_CMD, "<level>", "set log level");

                    LOG_USAGE << "Specific console commands: " << END_LOG;
                    print_specific_commands();
                }

                /**
                 * Allows to process the command
                 * @param command the command sting to handle
                 * @return true if we need to stop, otherwise false
                 */
                bool process_input_cmd(const char * command) {
                    //Convert the buffer into string
                    const string cmd(command);

                    //Stop the server
                    if (cmd == PROGRAM_EXIT_CMD) {
                        //Set the stop flag
                        m_is_stopped = true;
                        //Call the stop function
                        stop();
                        return true;
                    } else {
                        //We are to print the command prompt
                        if (cmd == PROGRAM_NOTHING_CMD) {
                            //Do nothing, just a new command prompt will be printed
                        } else {
                            //Print the server commands menu
                            if (cmd == PROGRAM_HELP_CMD) {
                                print_commands_help();
                            } else {
                                //Report the runtime information to the console
                                if (cmd == PROGRAM_RUNTIME_CMD) {
                                    report_run_time_info();
                                } else {
                                    //Lor parameters
                                    if (cmd == PROGRAM_PARAMS_CMD) {
                                        LOG_USAGE << "Log level: " << logger::get_curr_level_str() << END_LOG;
                                        report_program_params();
                                    } else {
                                        //Set the debug level
                                        if (begins_with(cmd, PROGRAM_SET_LL_CMD)) {
                                            set_log_level(cmd, PROGRAM_SET_LL_CMD);
                                        } else {
                                            //Process the remaining commands
                                            if (process_specific_cmd(cmd)) {
                                                LOG_ERROR << "The command '" << cmd << "' is not recognized!" << END_LOG;
                                                LOG_USAGE << "Enter '" << PROGRAM_HELP_CMD << "' for help!" << END_LOG;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    //Continue to the next command.
                    return false;
                }

                /**
                 * Allows to print the prompt
                 */
                inline void print_the_prompt() {
                    cout << ">> ";
                }

                /**
                 * Allows to set the debug level
                 * @param cmd the debug level
                 */
                inline void set_log_level(const string & cmd, const string & prefix) {
                    logger::set_reporting_level(get_string_value(cmd, prefix));
                }
            };
        }
    }
}

#endif /* CMD_LINE_BASE_HPP */

