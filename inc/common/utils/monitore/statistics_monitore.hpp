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
#define	STATISTICSMONITOR_HPP

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
                
                SMemotyUsage() : vmsize(0), vmpeak(0), vmrss(0), vmhwm(0) {}
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
        }
    }
}
#endif	/* STATISTICSMONITOR_HPP */

