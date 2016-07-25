/* 
 * File:   processor_console.hpp
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

#ifndef PROCESSOR_CONSOLE_HPP
#define PROCESSOR_CONSOLE_HPP

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"
#include "common/utils/cmd/cmd_line_base.hpp"

#include "processor/processor_server.hpp"
#include "processor/processor_manager.hpp"

using namespace std;

using namespace uva::utils::logging;
using namespace uva::utils::exceptions;
using namespace uva::utils::cmd;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace processor {
                //The number of worker threads - for the processor jobs pool
                static const string PROGRAM_SET_NT_CMD = "set nt ";

                /**
                 * This is the load processor console class:
                 * Responsibilities:
                 *      Provides the processor console
                 *      Allows to execute commands
                 *      Allows to get run time information
                 *      Allows to change run time settings
                 */
                class processor_console : public cmd_line_base {
                public:

                    /**
                     * The basic constructor
                     * @param params the processor parameters
                     * @param server the processor server
                     * @param processor_thread the processor server main thread
                     */
                    processor_console(processor_parameters & params, processor_server &server, thread &processor_thread)
                    : cmd_line_base(), m_params(params), m_server(server), m_processor_thread(processor_thread) {
                    }

                    /**
                     * The basic destructor
                     */
                    virtual ~processor_console() {
                        //If the user did not do stop, i,e, we are likely to exit because of an exception
                        if (!m_is_stopped) {
                            //Stop the thing
                            stop();
                        }
                    }

                protected:

                    /**
                     * @see cmd_line_base
                     */
                    virtual void print_specific_commands() {
                        print_command_help(PROGRAM_SET_NT_CMD, "<positive integer>", "set the number of processor threads");
                    }

                    /**
                     * @see cmd_line_base
                     */
                    virtual void report_run_time_info() {
                        //Report the run time info from the server
                        m_server.report_run_time_info();
                    }

                    /**
                     * @see cmd_line_base
                     */
                    virtual bool process_specific_cmd(const string & cmd) {
                        //Set the number of incoming pool threads
                        if (begins_with(cmd, PROGRAM_SET_NT_CMD)) {
                            try {
                                int32_t num_threads = get_int_value(cmd, PROGRAM_SET_NT_CMD);
                                ASSERT_CONDITION_THROW((num_threads <= 0),
                                        "The number of worker threads is to be > 0!");
                                //Set the number of threads
                                m_server.set_num_threads(num_threads);
                                //Remember the new number of threads
                                m_params.m_num_threads = num_threads;
                            } catch (std::exception &ex) {
                                LOG_ERROR << ex.what() << "\nEnter '" << PROGRAM_HELP_CMD << "' for help!" << END_LOG;
                            }
                            return false;
                        } else {
                            return true;
                        }
                    }

                    /**
                     * @see cmd_line_base
                     */
                    virtual void report_program_params() {
                        LOG_USAGE << m_params << END_LOG;
                    }

                    /**
                     * @see cmd_line_base
                     */
                    virtual void stop() {
                        //Stop the translation server
                        m_server.stop();

                        //Wait until the server's thread stops
                        m_processor_thread.join();

                        LOG_USAGE << "The processor server thread is stopped!" << END_LOG;
                    }

                private:
                    //Stores the reference to the processor parameters
                    processor_parameters & m_params;
                    //Stores the reference to the processor server
                    processor_server & m_server;
                    //Stores the reference to the processor thread
                    thread & m_processor_thread;

                };
            }
        }
    }
}

#endif /* PROCESSOR_CONSOLE_HPP */

