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
                cmd_line_base() {
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
                    print_server_commands();

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

                /**
                 * Prints the available commands. Is to be overloaded by the concrete implementation class.
                 */
                virtual void print_server_commands() = 0;

                /**
                 * Allows to process the command. Is to be overloaded by the concrete implementation class
                 * @return true if we need to stop, otherwise false
                 */
                virtual bool process_input_cmd(char * command) = 0;

                /**
                 * Allows to print the prompt
                 */
                inline void print_the_prompt() {
                    cout << ">> ";
                }

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

