/* 
 * File:   StatisticsMonitor.cpp
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

#include "common/utils/monitore/statistics_monitore.hpp"
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

namespace uva {
    namespace utils {
        namespace monitore {

            /**
             * This implementation is derived from 
             * http://locklessinc.com/articles/memory_usage/
             * This here is actually C-style code and also pretty ugly.
             */
#ifdef __APPLE__

            void stat_monitore::get_mem_stat(TMemotyUsage & memStat) {
                LOG_DEBUG << "Unable to obtain memory usage statistics on Mac OS yet!" << END_LOG;
                memStat = {};
            }
#else

            void stat_monitore::get_mem_stat(TMemotyUsage & memStat) {
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
                if (!f) throw uva_exception("Unable to open /proc/self/status for reading!");

                /* Read memory size data from /proc/self/status */
                while (!vmsize || !vmpeak || !vmrss || !vmhwm) {
                    if (getline(&line, &len, f) == -1) {
                        /* Some of the information isn't there, die */
                        throw uva_exception("Unable to read memory statistics data from /proc/self/status");
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
            double stat_monitore::get_cpu_time() {
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
        }
    }
}