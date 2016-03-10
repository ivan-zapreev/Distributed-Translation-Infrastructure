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
            class stat_monitore {
            public:

                /**
                 * Allows to get the current memory usage of the process.
                 * @param memStat this is an out parameter that will store the obtained data
                 * @throws Exception in case the memory statistics can not be obtained.
                 */
                static void get_mem_stat(TMemotyUsage & memStat);

                /**
                 * This function returns the current CPU time as given in the article
                 * http://nadeausoftware.com/articles/2012/03/c_c_tip_how_measure_cpu_time_benchmarking
                 * @return Returns the amount of CPU time used by the current process,
                 *         in seconds, or -1.0 if an error occurred.
                 */
                static double get_cpu_time();

            private:

                stat_monitore() {
                }

                stat_monitore(const stat_monitore& orig) {
                }

                virtual ~stat_monitore() {
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

