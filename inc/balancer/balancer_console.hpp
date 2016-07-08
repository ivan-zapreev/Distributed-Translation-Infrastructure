/* 
 * File:   balancer_console.hpp
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
 * Created on July 7, 2016, 12:08 PM
 */

#ifndef BALANCER_CONSOLE_HPP
#define BALANCER_CONSOLE_HPP

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"
#include "common/utils/cmd/cmd_line_base.hpp"

#include "balancer/balancer_server.hpp"
#include "balancer/translation_manager.hpp"
#include "balancer/translation_servers_manager.hpp"

using namespace std;

using namespace uva::utils::logging;
using namespace uva::utils::exceptions;
using namespace uva::utils::cmd;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace balancer {

                /**
                 * This is the load balancer console class:
                 * Responsibilities:
                 *      Provides the balancer console
                 *      Allows to execute commands
                 *      Allows to get run time information
                 *      Allows to change run time settings
                 */
                class balancer_console : public cmd_line_base {
                public:

                    /**
                     * The basic constructor
                     * @param params the balancer parameters
                     * @param balancer_thread the balancer server main thread
                     */
                    balancer_console(balancer_parameters & params, thread &balancer_thread)
                    : cmd_line_base(), m_params(params), m_balancer_thread(balancer_thread) {
                    }

                    /**
                     * The basic destructor
                     */
                    virtual ~balancer_console() {
                    }

                protected:

                    /**
                     * @see cmd_line_base
                     */
                    virtual void print_specific_commands() {
                        //ToDo: Implement
                        THROW_NOT_IMPLEMENTED();
                    }

                    /**
                     * @see cmd_line_base
                     */
                    virtual void report_run_time_info() {
                        //ToDo: Implement
                        THROW_NOT_IMPLEMENTED();
                    }

                    /**
                     * @see cmd_line_base
                     */
                    virtual void process_specific_cmd(const string & cmd) {
                        //ToDo: Implement
                        THROW_NOT_IMPLEMENTED();
                    }

                    /**
                     * @see cmd_line_base
                     */
                    virtual void report_program_params() {
                        //ToDo: Implement
                        THROW_NOT_IMPLEMENTED();
                    }

                    /**
                     * @see cmd_line_base
                     */
                    virtual void stop() {
                        //Stop the translation manager
                        LOG_USAGE << "Stopping the translation manager ..." << END_LOG;
                        translation_manager::stop();
                        
                        //Stop the translation servers manager
                        LOG_USAGE << "Stopping the translation server clients ..." << END_LOG;
                        translation_servers_manager::stop();

                        //Stop the translation server
                        LOG_USAGE << "Stopping the balancer server ..." << END_LOG;
                        balancer_server::stop();

                        //Wait until the server's thread stops
                        m_balancer_thread.join();

                        LOG_USAGE << "The server has stopped!" << END_LOG;
                    }

                private:
                    //Stores the reference to the balancer parameters
                    balancer_parameters & m_params;
                    //Stores the reference to the balancer thread
                    thread & m_balancer_thread;

                };
            }
        }
    }
}

#endif /* BALANCER_CONSOLE_HPP */

