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

#include "components/logging/Logger.hpp"

using namespace uva::utils::logging;

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


#endif	/* SYSTEM_HPP */

