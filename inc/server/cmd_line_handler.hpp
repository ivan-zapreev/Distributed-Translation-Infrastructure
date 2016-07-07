/* 
 * File:   cmd_line_handler.hpp
 * Author: zapreevis
 *
 * Created on March 9, 2016, 3:42 PM
 */

#ifndef CMD_LINE_HANDLER_HPP
#define CMD_LINE_HANDLER_HPP

#include "common/utils/logging/logger.hpp"
#include "common/utils/cmd/cmd_line_base.hpp"

#include "server/server_parameters.hpp"
#include "server/translation_server.hpp"

using namespace uva::utils::logging;
using namespace uva::utils::cmd;

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
                static const string PROGRAM_SET_D_CMD = "set d ";
                static const string PROGRAM_SET_EDL_CMD = "set edl ";
                static const string PROGRAM_SET_PT_CMD = "set pt ";
                static const string PROGRAM_SET_SC_CMD = "set sc ";
                static const string PROGRAM_SET_WP_CMD = "set wp ";
                static const string PROGRAM_SET_LDP_CMD = "set ldp ";
                static const string PROGRAM_SET_GL_CMD = "set gl ";

                /**
                 * The command line handler class for the translation server.
                 */
                class cmd_line_handler : public cmd_line_base {
                public:

                    /**
                     * The basic constructor
                     * @param params the reference to the server parameters
                     * @param server the reference to the server
                     * @param server_thread the reference to the server thread
                     */
                    cmd_line_handler(server_parameters & params, translation_server &server, thread &server_thread)
                    : cmd_line_base(), m_params(params), m_server(server), m_server_thread(server_thread) {
                    }

                    /**
                     * The basic destructor
                     */
                    virtual ~cmd_line_handler() {
                    }

                protected:

                    /**
                     * Prints the available server commands
                     */
                    virtual void print_server_commands() {
                        LOG_USAGE << "Available server commands: " << END_LOG;
                        LOG_USAGE << "\t'" << PROGRAM_EXIT_CMD << " & <enter>'  - to exit." << END_LOG;
                        LOG_USAGE << "\t'" << PROGRAM_HELP_CMD << " & <enter>'  - print HELP info." << END_LOG;
                        LOG_USAGE << "\t'" << PROGRAM_RUNTIME_CMD << " & <enter>'  - run-time statistics." << END_LOG;
                        LOG_USAGE << "\t'" << PROGRAM_PARAMS_CMD << " & <enter>'  - print server parameters." << END_LOG;
                        LOG_USAGE << "\t'" << PROGRAM_SET_LL_CMD << "<level> & <enter>'  - set log level." << END_LOG;
                        LOG_USAGE << "\t'" << PROGRAM_SET_NT_CMD << " <positive integer> & <enter>'  - set the number of worker threads." << END_LOG;
                        LOG_USAGE << "\t'" << PROGRAM_SET_D_CMD << "<integer> & <enter>'  - set the distortion limit." << END_LOG;
                        LOG_USAGE << "\t'" << PROGRAM_SET_PT_CMD << "<unsigned float> & <enter>'  - set pruning threshold." << END_LOG;
                        LOG_USAGE << "\t'" << PROGRAM_SET_SC_CMD << "<integer> & <enter>'  - set stack capacity." << END_LOG;
                        LOG_USAGE << "\t'" << PROGRAM_SET_LDP_CMD << "<float> & <enter>'  - set linear distortion penalty." << END_LOG;
                        LOG_USAGE << "\t'" << PROGRAM_SET_WP_CMD << "<float> & <enter>'  - set word penalty." << END_LOG;
#if IS_SERVER_TUNING_MODE
                        LOG_USAGE << "\t'" << PROGRAM_SET_GL_CMD << "<bool> & <enter>'  - enable/disable search lattice generation." << END_LOG;
#endif
                    }

                    /**
                     * Allows to process the command
                     * @param command the command sting to handle
                     * @return true if we need to stop, otherwise false
                     */
                    virtual bool process_input_cmd(char * command) {
                        //Convert the buffer into string
                        string cmd(command);

                        //We are to print the command prompt
                        if (cmd == PROGRAM_NOTHING_CMD) {
                            return false;
                        }

                        //Stop the server
                        if (cmd == PROGRAM_EXIT_CMD) {
                            stop();
                            return true;
                        }

                        //Print the server commands menu
                        if (cmd == PROGRAM_HELP_CMD) {
                            print_server_commands();
                            return false;
                        }

                        //Report the runtime information to the console
                        if (cmd == PROGRAM_RUNTIME_CMD) {
                            m_server.report_run_time_info();
                            return false;
                        }

                        //Lor parameters
                        if (cmd == PROGRAM_PARAMS_CMD) {
                            LOG_USAGE << "Log level: " << logger::get_curr_level_str()
                                    << ", " << m_params << END_LOG;
                            return false;
                        }

                        //Set the debug level
                        if (begins_with(cmd, PROGRAM_SET_LL_CMD)) {
                            set_log_level(cmd, PROGRAM_SET_LL_CMD);
                            return false;
                        }

                        //Set the number of threads
                        if (begins_with(cmd, PROGRAM_SET_NT_CMD)) {
                            set_num_threads(cmd, PROGRAM_SET_LL_CMD);
                            return false;
                        }

                        //Set other decoder parameters
                        set_decoder_params(cmd, m_params.m_de_params);

                        //Continue to the next command.
                        return false;
                    }

                    /**
                     * Allows to stop the server;
                     */
                    void stop() {
                        //Stop the translation server
                        LOG_USAGE << "Stopping the server ..." << END_LOG;
                        m_server.stop();

                        //Wait until the server's thread stops
                        m_server_thread.join();

                        LOG_USAGE << "The server has stopped!" << END_LOG;
                    }

                    /**
                     * Allows to set the number of worker threads
                     * @param cmd the input command 
                     * @param prefix the command prefix
                     */
                    inline void set_num_threads(const string & cmd, const string & prefix) {
                        try {
                            //Get the specified number of threads
                            int32_t num_threads = get_int_value(cmd, prefix);
                            ASSERT_CONDITION_THROW((num_threads <= 0),
                                    "The number of worker threads is to be > 0!");

                            if (((size_t) num_threads) != m_params.m_num_threads) {
                                //Set the number of threads
                                m_server.set_num_threads(num_threads);
                                //Remember the new number of threads
                                m_params.m_num_threads = num_threads;
                            } else {
                                LOG_WARNING << "The number of worker threads is already: "
                                        << num_threads << "!" << END_LOG;
                            }
                        } catch (std::exception &ex) {
                            LOG_ERROR << ex.what() << "\nEnter '" << PROGRAM_HELP_CMD << "' for help!" << END_LOG;
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

                            //Stores the boolean flag indicating whether the command was recognized or not.
                            bool is_recognized = false;

                            if (begins_with(cmd, PROGRAM_SET_D_CMD)) {
                                de_local.m_dist_limit = get_int_value(cmd, PROGRAM_SET_D_CMD);
                                is_recognized = true;
                            }
                            if (begins_with(cmd, PROGRAM_SET_PT_CMD)) {
                                de_local.m_pruning_threshold = get_float_value(cmd, PROGRAM_SET_PT_CMD);
                                is_recognized = true;
                            }
                            if (begins_with(cmd, PROGRAM_SET_SC_CMD)) {
                                de_local.m_stack_capacity = get_int_value(cmd, PROGRAM_SET_SC_CMD);
                                is_recognized = true;
                            }
                            if (begins_with(cmd, PROGRAM_SET_WP_CMD)) {
                                de_local.m_word_penalty = get_float_value(cmd, PROGRAM_SET_WP_CMD);
                                is_recognized = true;
                            }
                            if (begins_with(cmd, PROGRAM_SET_LDP_CMD)) {
                                de_local.m_lin_dist_penalty = get_float_value(cmd, PROGRAM_SET_LDP_CMD);
                                is_recognized = true;
                            }

#if IS_SERVER_TUNING_MODE
                            if (begins_with(cmd, PROGRAM_SET_GL_CMD)) {
                                de_local.m_is_gen_lattice = get_bool_value(cmd, PROGRAM_SET_GL_CMD);
                                is_recognized = true;
                            }
#endif

                            if (!is_recognized) {
                                THROW_EXCEPTION(string("The command '") + cmd + string("' is unknown!"));
                            }

                            //Finalize the parameters
                            de_local.finalize();

                            //Set the parameters back
                            de_params = de_local;

                        } catch (std::exception &ex) {
                            LOG_ERROR << ex.what() << " Enter '" << PROGRAM_HELP_CMD << "' for help!" << END_LOG;
                        }
                    }

                private:
                    //Stores the reference to the server parameters
                    server_parameters & m_params;
                    //Stores the reference to the server
                    translation_server &m_server;
                    //Stores the reference to the server thread
                    thread &m_server_thread;
                };
            }
        }
    }
}

#endif /* CMD_LINE_HANDLER_HPP */

