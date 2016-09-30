/* 
 * File:   server_console.hpp
 * Author: zapreevis
 *
 * Created on March 9, 2016, 3:42 PM
 */

#ifndef SERVER_CONSOLE_HPP
#define SERVER_CONSOLE_HPP

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
                //The number of worker threads - decoders
                static const string PROGRAM_SET_NT_CMD = "set nt ";
                //Declare the program "set" commands, NOTE the end spaces are needed!
                static const string PROGRAM_SET_D_CMD = "set d ";
                static const string PROGRAM_SET_EDL_CMD = "set edl ";
                static const string PROGRAM_SET_PT_CMD = "set pt ";
                static const string PROGRAM_SET_SC_CMD = "set sc ";
                static const string PROGRAM_SET_LDP_CMD = "set ldp ";
                static const string PROGRAM_SET_GL_CMD = "set gl ";

                /**
                 * The command line handler class for the translation server.
                 */
                class server_console : public cmd_line_base {
                public:

                    /**
                     * The basic constructor
                     * @param params the reference to the server parameters
                     * @param server the reference to the server
                     * @param server_thread the reference to the server thread
                     */
                    server_console(server_parameters & params, translation_server &server, thread &server_thread)
                    : cmd_line_base(), m_params(params), m_server(server), m_server_thread(server_thread) {
                    }

                    /**
                     * The basic destructor
                     */
                    virtual ~server_console() {
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
                        print_command_help(PROGRAM_SET_NT_CMD, "<positive integer>", "set the number of translating threads");
                        print_command_help(PROGRAM_SET_D_CMD, "<integer>", "set the distortion limit");
                        print_command_help(PROGRAM_SET_PT_CMD, "<unsigned float>", "set pruning threshold");
                        print_command_help(PROGRAM_SET_SC_CMD, "<integer>", "set stack capacity");
                        print_command_help(PROGRAM_SET_LDP_CMD, "<float>", "set linear distortion penalty");
#if IS_SERVER_TUNING_MODE
                        print_command_help(PROGRAM_SET_GL_CMD, "<bool>", "enable/disable search lattice generation");
#endif
                    }

                    /**
                     * @see cmd_line_base
                     */
                    virtual void report_run_time_info() {
                        m_server.report_run_time_info();
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
                    virtual bool process_specific_cmd(const string & cmd) {
                        //Set the number of threads
                        if (begins_with(cmd, PROGRAM_SET_NT_CMD)) {
                            set_num_threads(cmd, PROGRAM_SET_NT_CMD);
                            return false;
                        } else {
                            //Set other decoder parameters
                            return set_decoder_params(cmd, m_params.m_de_params);
                        }
                    }

                    /**
                     * @see cmd_line_base
                     */
                    virtual void stop() {
                        //Stop the translation server
                        LOG_USAGE << "Stopping the server ..." << END_LOG;
                        m_server.stop();

                        //Wait until the server's thread stops
                        m_server_thread.join();

                        LOG_USAGE << "The server has stopped!" << END_LOG;
                    }

                private:

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
                     * @return true if the command could not be recognized, otherwise false!
                     */
                    inline bool set_decoder_params(const string & cmd, de_parameters & de_params) {
                        //Stores the boolean flag indicating whether the command was recognized or not.
                        bool is_recognized = false;

                        //Set some of the server runtime parameters
                        try {
                            //Get a copy of current decoder parameters
                            de_parameters de_local = de_params;

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

                            //Finalize the parameters
                            de_local.finalize();

                            //Set the parameters back
                            de_params = de_local;
                        } catch (std::exception &ex) {
                            LOG_ERROR << ex.what() << " Enter '" << PROGRAM_HELP_CMD << "' for help!" << END_LOG;
                        }

                        return !is_recognized;
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

