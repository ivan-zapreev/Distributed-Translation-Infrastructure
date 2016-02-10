/* 
 * File:   system.hpp
 * Author: zapreevis
 *
 * Created on January 14, 2016, 4:27 PM
 */

#ifndef SYSTEM_HPP
#define	SYSTEM_HPP

#include <stdexcept>
#include <execinfo.h>

#include "common/utils/logging/logger.hpp"

using namespace uva::utils::logging;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace common {

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
                static void print_info(const char * prog_name_str, const char * prog_ver_str) {
                    LOG_USAGE << " ------------------------------------------------------------------ " << END_LOG;
                    LOG_USAGE << "|                 " << prog_name_str << "     :)\\___/(: |" << END_LOG;
                    LOG_USAGE << "|                       Software version " << prog_ver_str << "             {(@)v(@)} |" << END_LOG;
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
            }
        }
    }
}

#endif	/* SYSTEM_HPP */

