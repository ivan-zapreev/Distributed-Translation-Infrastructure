/* 
 * File:   system.hpp
 * Author: zapreevis
 *
 * Created on January 14, 2016, 4:27 PM
 */

#ifndef SYSTEM_HPP
#define SYSTEM_HPP

#include <string>
#include <stdexcept>
#include <execinfo.h>
#include <INI.h>

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"

using namespace std;

using namespace uva::utils::exceptions;
using namespace uva::utils::logging;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace common {

                //Declare the program version string
#define PROGRAM_VERSION_STR "1.6"

                // Check windows
#if _WIN32 || _WIN64
#if _WIN64
#define ENVIRONMENT64
#else
#define ENVIRONMENT32
#endif
#endif

                // Check GCC
#if __GNUC__
#if __x86_64__ || __ppc64__
#define ENVIRONMENT64
#else
#define ENVIRONMENT32
#endif
#endif

#define SAFE_DESTROY(ptr) \
    if (ptr != NULL) { \
        delete ptr; \
        ptr = NULL; \
    }

                /**
                 * This functions does nothing more but printing the program header information
                 */
                static void print_info(const char * prog_name_str) {
                    LOG_USAGE << " ------------------------------------------------------------------ " << END_LOG;
                    LOG_USAGE << "|                 " << prog_name_str << "     :)\\___/(: |" << END_LOG;
                    LOG_USAGE << "|                       Software version " << PROGRAM_VERSION_STR << "             {(@)v(@)} |" << END_LOG;
                    LOG_USAGE << "|                         The Owl release.               {|~- -~|} |" << END_LOG;
                    LOG_USAGE << "|            Copyright (C) Dr. Ivan S Zapreev, 2015-2016 {/^'^'^\\} |" << END_LOG;
                    LOG_USAGE << "|  ═════════════════════════════════════════════════════════m-m══  |" << END_LOG;
                    LOG_USAGE << "|        This software is distributed under GPL 2.0 license        |" << END_LOG;
                    LOG_USAGE << "|          (GPL stands for GNU General Public License)             |" << END_LOG;
                    LOG_USAGE << "|          The product comes with ABSOLUTELY NO WARRANTY.          |" << END_LOG;
                    LOG_USAGE << "|   This is a free software, you are welcome to redistribute it.   |" << END_LOG;
#ifdef ENVIRONMENT64
                    LOG_USAGE << "|                     Running in 64 bit mode!                      |" << END_LOG;
#else
                    LOG_USAGE << "|                     Running in 32 bit mode!                      |" << END_LOG;
#endif
                    LOG_USAGE << "|                 Build on: " << __DATE__ << " " << __TIME__ << "                   |" << END_LOG;
                    LOG_USAGE << " ------------------------------------------------------------------ " << END_LOG;
                }

                //Declare the maximum stack trace depth
#define MAX_STACK_TRACE_LEN 100

                /**
                 * The uncaught exceptions handler
                 */
                static void handler() {
                    void *trace_elems[20];
                    int trace_elem_count(backtrace(trace_elems, MAX_STACK_TRACE_LEN));
                    char **stack_syms(backtrace_symbols(trace_elems, trace_elem_count));
                    LOG_ERROR << "Ooops, Sorry! Something terrible has happened, we crashed!" << END_LOG;
                    for (int i = 0; i < trace_elem_count; ++i) {
                        LOG_ERROR << stack_syms[i] << END_LOG;
                    }
                    free(stack_syms);
                    exit(1);
                }

                //Allows to get and assert on the given section/key value presence
#define GET_ASSERT(ini, section, key, value_str, unk_value, is_compulsory) \
    const string def_value = (is_compulsory ? "<UNKNOWN_VALUE>" : unk_value); \
    const string value_str = ini.get(section, key, def_value); \
    ASSERT_CONDITION_THROW(is_compulsory && (value_str == def_value), \
            string("Could not find '[") + section + string("]/") + \
            key + string("' section/key in the configuration file!"));

                template<typename INT_TYPE>
                INT_TYPE get_integer(INI<> &ini, string section, string key,
                        string unk_value = "0", bool is_compulsory = true) {
                    //Get the value and assert on its presence
                    GET_ASSERT(ini, section, key, value_str, unk_value, is_compulsory);

                    try {
                        //Parse this value to an integer
                        return (INT_TYPE) stoi(value_str);
                    } catch (std::invalid_argument & ex1) {
                    } catch (std::out_of_range & ex2) {
                    }

                    //Throw an exception
                    THROW_EXCEPTION(string("Could not parse integer: ") + value_str);
                }

                string get_string(INI<> &ini, string section, string key,
                        string unk_value = "", bool is_compulsory = true) {
                    //Get the value and assert on its presence
                    GET_ASSERT(ini, section, key, value_str, unk_value, is_compulsory);

                    //Parse this value to an integer
                    return value_str;
                }

                float get_float(INI<> &ini, string section, string key,
                        string unk_value = "0.0", bool is_compulsory = true) {
                    //Get the value and assert on its presence
                    GET_ASSERT(ini, section, key, value_str, unk_value, is_compulsory);

                    try {
                        //Parse this value to an integer
                        return stof(value_str);
                    } catch (std::invalid_argument & ex1) {
                    } catch (std::out_of_range & ex2) {
                    }

                    //Throw an exception
                    THROW_EXCEPTION(string("Could not parse float: ") + value_str);
                }

                bool get_bool(INI<> &ini, string section, string key,
                        string unk_value = "false", bool is_compulsory = true) {
                    //Get the value and assert on its presence
                    GET_ASSERT(ini, section, key, value_str, unk_value, is_compulsory);

                    if (value_str == "true") {
                        return true;
                    } else {
                        if (value_str == "false") {
                            return false;
                        }
                    }

                    //Throw an exception
                    THROW_EXCEPTION(string("Could not parse boolean: ") + value_str);
                }
            }
        }
    }
}

#endif /* SYSTEM_HPP */

