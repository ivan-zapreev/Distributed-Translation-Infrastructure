/* 
 * File:   StatisticsMonitor.hpp
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
 * Created on April 18, 2015, 12:18 PM
 */

#ifndef STATISTICSMONITOR_HPP
#define STATISTICSMONITOR_HPP

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"

#if defined(_WIN32)
#include <Windows.h>

#elif defined(__unix__) || defined(__unix) || defined(unix) || (defined(__APPLE__) && defined(__MACH__))
#include <unistd.h>
#include <sys/resource.h>
#include <sys/times.h>
#include <time.h>

#else
#error "Unable to define getCPUTime( ) for an unknown OS."
#endif

#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <sstream>

using namespace uva::utils::logging;
using namespace uva::utils::exceptions;

namespace uva {
    namespace utils {
        namespace monitor {
            
#ifdef __APPLE__
            
#define DECLARE_MONITOR_STATS \
double start_time, end_time; \

#define INITIALIZE_STATS \
start_time = stat_monitor::get_cpu_time();
            
#define REPORT_STATS(ACTION_PARAM)\
end_time = stat_monitor::get_cpu_time(); \
LOG_USAGE << (ACTION_PARAM) << " took " \
<< (end_time - start_time) << " CPU seconds." << END_LOG;
            
#else
            
#define DECLARE_MONITOR_STATS \
double start_time, end_time; \
TMemotyUsage mem_stat_start = {}, mem_stat_end = {};
            
#define INITIALIZE_STATS \
stat_monitor::get_mem_stat(mem_stat_start); \
start_time = stat_monitor::get_cpu_time();
            
#define REPORT_STATS(ACTION_PARAM)\
end_time = stat_monitor::get_cpu_time(); \
stat_monitor::get_mem_stat(mem_stat_end); \
LOG_USAGE << (ACTION_PARAM) << " took " \
<< (end_time - start_time) << " CPU seconds." << END_LOG; \
report_memory_usage((ACTION_PARAM).c_str(), \
mem_stat_start, mem_stat_end, true);
            
#endif
            /**
             * This structure stores the memory statistics.
             * Resident Set Size: number of pages the process has
             * in real memory.  This is just the pages which count
             * toward text, data, or stack space.  This does not
             * include pages which have not been demand-loaded in,
             * or which are swapped out.
             * For more information see http://man7.org/linux/man-pages/man5/proc.5.html
             */
            struct memory_usage {
                //Virtual memory size in Kb
                int vmsize;
                //Peak virtual memory size in Kb
                int vmpeak;
                //Resident set size in Kb
                int vmrss;
                //Peak resident set size in Kb
                int vmhwm;
                
                memory_usage() : vmsize(0), vmpeak(0), vmrss(0), vmhwm(0) {
                }
            };
            
            typedef memory_usage TMemotyUsage;
            
            /**
             * This class is responsible for monitoring the program statistics, such as the used memory and CPU times.
             * This class is a trivial singleton
             */
            class stat_monitor {
            public:
                
                /**
                 * This implementation is derived from
                 * http://locklessinc.com/articles/memory_usage/
                 * This here is actually C-style code and also pretty ugly.
                 */
#ifdef __APPLE__
                
                static void get_mem_stat(TMemotyUsage & memStat) {
                    LOG_DEBUG << "Unable to obtain memory usage statistics on Mac OS yet!" << END_LOG;
                    memStat = {};
                }
#else
                
                static void get_mem_stat(TMemotyUsage & memStat) {
                    char *line;
                    char *vmsize = NULL;
                    char *vmpeak = NULL;
                    char *vmrss = NULL;
                    char *vmhwm = NULL;
                    size_t len;
                    FILE *f;
                    
                    line = (char*) malloc(128);
                    len = 128;
                    
                    f = fopen("/proc/self/status", "r");
                    if (!f) THROW_EXCEPTION("Unable to open /proc/self/status for reading!");
                    
                    /* Read memory size data from /proc/self/status */
                    while (!vmsize || !vmpeak || !vmrss || !vmhwm) {
                        if (getline(&line, &len, f) == -1) {
                            /* Some of the information isn't there, die */
                            THROW_EXCEPTION("Unable to read memory statistics data from /proc/self/status");
                        }
                        
                        /* Find VmPeak */
                        if (!strncmp(line, "VmPeak:", 7)) {
                            vmpeak = strdup(&line[7]);
                        }/* Find VmSize */
                        else if (!strncmp(line, "VmSize:", 7)) {
                            vmsize = strdup(&line[7]);
                        }/* Find VmRSS */
                        else if (!strncmp(line, "VmRSS:", 6)) {
                            vmrss = strdup(&line[7]);
                        }/* Find VmHWM */
                        else if (!strncmp(line, "VmHWM:", 6)) {
                            vmhwm = strdup(&line[7]);
                        }
                    }
                    free(line);
                    
                    fclose(f);
                    
                    /* Get rid of " kB\n"*/
                    len = strlen(vmsize);
                    vmsize[len - 4] = 0;
                    len = strlen(vmpeak);
                    vmpeak[len - 4] = 0;
                    len = strlen(vmrss);
                    vmrss[len - 4] = 0;
                    len = strlen(vmhwm);
                    vmhwm[len - 4] = 0;
                    
                    /* Parse the results */
                    memStat.vmpeak = atoi(vmpeak);
                    memStat.vmsize = atoi(vmsize);
                    memStat.vmrss = atoi(vmrss);
                    memStat.vmhwm = atoi(vmhwm);
                    
                    /* Print some info and debug information */
                    LOG_DEBUG2 << "read: vmsize=" << vmsize << " Kb, vmpeak=" << vmpeak << " Kb, vmrss=" << vmrss << " Kb, vmhwm=" << vmhwm << " Kb" << END_LOG;
                    LOG_DEBUG2 << "parsed: vmsize=" << memStat.vmsize << " Kb, vmpeak=" << memStat.vmpeak << " Kb, vmrss=" << memStat.vmrss << " Kb, vmhwm=" << memStat.vmhwm << " Kb" << END_LOG;
                    
                    /* Free the allocated memory */
                    free(vmpeak);
                    free(vmsize);
                    free(vmrss);
                    free(vmhwm);
                }
#endif
                
                /*
                 * Author:  David Robert Nadeau
                 * Site:    http://NadeauSoftware.com/
                 * License: Creative Commons Attribution 3.0 Unported License
                 *          http://creativecommons.org/licenses/by/3.0/deed.en_US
                 *
                 * Returns the amount of CPU time used by the current process,
                 * in seconds, or -1.0 if an error occurred.
                 */
                static double get_cpu_time() {
#if defined(_WIN32)
                    /* Windows -------------------------------------------------- */
                    FILETIME createTime;
                    FILETIME exitTime;
                    FILETIME kernelTime;
                    FILETIME userTime;
                    if (GetProcessTimes(GetCurrentProcess(),
                                        &createTime, &exitTime, &kernelTime, &userTime) != -1) {
                        SYSTEMTIME userSystemTime;
                        if (FileTimeToSystemTime(&userTime, &userSystemTime) != -1)
                            return (double) userSystemTime.wHour * 3600.0 +
                            (double) userSystemTime.wMinute * 60.0 +
                            (double) userSystemTime.wSecond +
                            (double) userSystemTime.wMilliseconds / 1000.0;
                    }
                    
#elif defined(__unix__) || defined(__unix) || defined(unix) || (defined(__APPLE__) && defined(__MACH__))
                    /* AIX, BSD, Cygwin, HP-UX, Linux, OSX, and Solaris --------- */
                    
#if defined(_POSIX_TIMERS) && (_POSIX_TIMERS > 0)
                    /* Prefer high-res POSIX timers, when available. */
                    {
                        clockid_t id;
                        struct timespec ts;
#if _POSIX_CPUTIME > 0
                        /* Clock ids vary by OS.  Query the id, if possible. */
                        if (clock_getcpuclockid(0, &id) == -1)
#endif
#if defined(CLOCK_PROCESS_CPUTIME_ID)
                        /* Use known clock id for AIX, Linux, or Solaris. */
                            id = CLOCK_PROCESS_CPUTIME_ID;
#elif defined(CLOCK_VIRTUAL)
                        /* Use known clock id for BSD or HP-UX. */
                        id = CLOCK_VIRTUAL;
#else
                        id = (clockid_t) - 1;
#endif
                        if (id != (clockid_t) - 1 && clock_gettime(id, &ts) != -1)
                            return (double) ts.tv_sec +
                            (double) ts.tv_nsec / 1000000000.0;
                    }
#endif
                    
#if defined(RUSAGE_SELF)
                    {
                        struct rusage rusage;
                        if (getrusage(RUSAGE_SELF, &rusage) != -1)
                            return (double) rusage.ru_utime.tv_sec +
                            (double) rusage.ru_utime.tv_usec / 1000000.0;
                    }
#endif
                    
#if defined(_SC_CLK_TCK)
                    {
                        const double ticks = (double) sysconf(_SC_CLK_TCK);
                        struct tms tms;
                        if (times(&tms) != (clock_t) - 1)
                            return (double) tms.tms_utime / ticks;
                    }
#endif
                    
#if defined(CLOCKS_PER_SEC)
                    {
                        clock_t cl = clock();
                        if (cl != (clock_t) - 1)
                            return (double) cl / (double) CLOCKS_PER_SEC;
                    }
#endif
                    
#endif
                    
                    return -1; /* Failed. */
                }
                
            private:
                
                stat_monitor() {
                }
                
                stat_monitor(const stat_monitor& /*unused*/) {
                }
                
                virtual ~stat_monitor() {
                }
                
            };
            
            //The number of bytes in one Mb
            const uint32_t BYTES_ONE_MB = 1024u;
            
            /**
             * This function is meant to give the memory statistics information delta
             * @param action the monitored action
             * @param ms_start the start memory usage statistics
             * @param ms_end the end memory usage statistics
             * @param is_do_info true if the memory info may be print
             */
            static inline void report_memory_usage(const char* action, TMemotyUsage ms_start, TMemotyUsage ms_end, const bool is_do_info) {
                LOG_USAGE << "Action: \'" << action << "\' memory change:" << END_LOG;
                LOG_DEBUG << "\tmemory before: vmsize=" << SSTR(ms_start.vmsize) << " Kb, vmpeak="
                << SSTR(ms_start.vmpeak) << " Kb, vmrss=" << SSTR(ms_start.vmrss)
                << " Kb, vmhwm=" << SSTR(ms_start.vmhwm) << " Kb" << END_LOG;
                LOG_DEBUG << "memory after: vmsize=" << SSTR(ms_end.vmsize) << " Kb, vmpeak="
                << SSTR(ms_end.vmpeak) << " Kb, vmrss=" << SSTR(ms_end.vmrss)
                << " Kb, vmhwm=" << SSTR(ms_end.vmhwm) << " Kb" << END_LOG;
                
                int vmsize = ((ms_end.vmsize < ms_start.vmsize) ? 0 : ms_end.vmsize - ms_start.vmsize) / BYTES_ONE_MB;
                int vmpeak = ((ms_end.vmpeak < ms_start.vmpeak) ? 0 : ms_end.vmpeak - ms_start.vmpeak) / BYTES_ONE_MB;
                int vmrss = ((ms_end.vmrss < ms_start.vmrss) ? 0 : ms_end.vmrss - ms_start.vmrss) / BYTES_ONE_MB;
                int vmhwm = ((ms_end.vmhwm < ms_start.vmhwm) ? 0 : ms_end.vmhwm - ms_start.vmhwm) / BYTES_ONE_MB;
                LOG_USAGE << showpos << "vmsize=" << vmsize << " Mb, vmpeak=" << vmpeak
                << " Mb, vmrss=" << vmrss << " Mb, vmhwm=" << vmhwm
                << " Mb" << noshowpos << END_LOG;
                if (is_do_info) {
                    LOG_INFO3 << "  vmsize - Virtual memory size; vmpeak - Peak virtual memory size" << END_LOG;
                    LOG_INFO3 << "    Virtual memory size is how much virtual memory the process has in total (RAM+SWAP)" << END_LOG;
                    LOG_INFO3 << "  vmrss  - Resident set size; vmhwm  - Peak resident set size" << END_LOG;
                    LOG_INFO3 << "    Resident set size is how much memory this process currently has in main memory (RAM)" << END_LOG;
                }
            }
        }
    }
}
#endif /* STATISTICSMONITOR_HPP */

