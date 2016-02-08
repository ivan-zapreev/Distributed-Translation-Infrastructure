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

using namespace uva::utils::exceptions;

namespace uva {
    namespace utils {
        namespace monitore {

            /**
             * This structure stores the memory statistics.
             * Resident Set Size: number of pages the process has
             * in real memory.  This is just the pages which count
             * toward text, data, or stack space.  This does not
             * include pages which have not been demand-loaded in,
             * or which are swapped out.
             * For more information see http://man7.org/linux/man-pages/man5/proc.5.html
             */
            struct SMemotyUsage {
                //Virtual memory size in Kb
                int vmsize;
                //Peak virtual memory size in Kb
                int vmpeak;
                //Resident set size in Kb
                int vmrss;
                //Peak resident set size in Kb
                int vmhwm;

                SMemotyUsage() : vmsize(0), vmpeak(0), vmrss(0), vmhwm(0) {
                }
            };

            typedef SMemotyUsage TMemotyUsage;

            /**
             * This class is responsible for monitoring the program statistics, such as the used memory and CPU times.
             * This class is a trivial singleton
             */
            class StatisticsMonitor {
            public:

                /**
                 * Allows to get the current memory usage of the process.
                 * @param memStat this is an out parameter that will store the obtained data
                 * @throws Exception in case the memory statistics can not be obtained.
                 */
                static void getMemoryStatistics(TMemotyUsage & memStat) throw (Exception);

                /**
                 * This function returns the current CPU time as given in the article
                 * http://nadeausoftware.com/articles/2012/03/c_c_tip_how_measure_cpu_time_benchmarking
                 * @return Returns the amount of CPU time used by the current process,
                 *         in seconds, or -1.0 if an error occurred.
                 */
                static double getCPUTime();

            private:

                StatisticsMonitor() {
                }

                StatisticsMonitor(const StatisticsMonitor& orig) {
                }

                virtual ~StatisticsMonitor() {
                }

            };


            //The number of bytes in one Mb
            const uint32_t BYTES_ONE_MB = 1024u;

            /**
             * This function is meant to give the memory statistics information delta
             * @param action the monitored action
             * @param msStart the start memory usage statistics
             * @param msEnd the end memory usage statistics
             * @param isDoInfo true if the memory info may be print
             */
            static inline void report_memory_usage(const char* action, TMemotyUsage msStart, TMemotyUsage msEnd, const bool isDoInfo) {
                LOG_USAGE << "Action: \'" << action << "\' memory change:" << END_LOG;
                LOG_DEBUG << "\tmemory before: vmsize=" << SSTR(msStart.vmsize) << " Kb, vmpeak="
                        << SSTR(msStart.vmpeak) << " Kb, vmrss=" << SSTR(msStart.vmrss)
                        << " Kb, vmhwm=" << SSTR(msStart.vmhwm) << " Kb" << END_LOG;
                LOG_DEBUG << "memory after: vmsize=" << SSTR(msEnd.vmsize) << " Kb, vmpeak="
                        << SSTR(msEnd.vmpeak) << " Kb, vmrss=" << SSTR(msEnd.vmrss)
                        << " Kb, vmhwm=" << SSTR(msEnd.vmhwm) << " Kb" << END_LOG;

                int vmsize = ((msEnd.vmsize < msStart.vmsize) ? 0 : msEnd.vmsize - msStart.vmsize) / BYTES_ONE_MB;
                int vmpeak = ((msEnd.vmpeak < msStart.vmpeak) ? 0 : msEnd.vmpeak - msStart.vmpeak) / BYTES_ONE_MB;
                int vmrss = ((msEnd.vmrss < msStart.vmrss) ? 0 : msEnd.vmrss - msStart.vmrss) / BYTES_ONE_MB;
                int vmhwm = ((msEnd.vmhwm < msStart.vmhwm) ? 0 : msEnd.vmhwm - msStart.vmhwm) / BYTES_ONE_MB;
                LOG_USAGE << showpos << "vmsize=" << vmsize << " Mb, vmpeak=" << vmpeak
                        << " Mb, vmrss=" << vmrss << " Mb, vmhwm=" << vmhwm
                        << " Mb" << noshowpos << END_LOG;
                if (isDoInfo) {
                    LOG_INFO << "  vmsize - Virtual memory size; vmpeak - Peak virtual memory size" << END_LOG;
                    LOG_INFO << "    Virtual memory size is how much virtual memory the process has in total (RAM+SWAP)" << END_LOG;
                    LOG_INFO << "  vmrss  - Resident set size; vmhwm  - Peak resident set size" << END_LOG;
                    LOG_INFO << "    Resident set size is how much memory this process currently has in main memory (RAM)" << END_LOG;
                }
            }
        }
    }
}
#endif /* STATISTICSMONITOR_HPP */

