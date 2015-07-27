/* 
 * File:   Logger.hpp
 * Author: Dr. Ivan S. Zapreev
 *
 * Some of the ideas and code implemented here were taken from:
 *  http://www.drdobbs.com/cpp/logging-in-c/201804215?pgno=1
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
 * Created on July 26, 2015, 4:08 PM
 */

#include "Logger.hpp"
#include "StatisticsMonitor.hpp"

using uva::smt::monitore::StatisticsMonitor;

namespace uva {
    namespace smt {
        namespace logging {
            Logger::DebugLevel Logger::currLEvel;

            const char * Logger::DebugLevelStr[] = {"USAGE", "ERROR", "WARNING", "RESULT", "INFO", "DEBUG", "DEBUG1", "DEBUG2"};

            std::ostream& Logger::Get(DebugLevel level) {
                std::ostream & os = cout;

                os << DebugLevelStr[level] << ": ";

                return os;
            }

            //Initialize the progress bar chars array
            const vector<string> Logger::progressChars({"///", "---", "\\\\\\", "|||", "\r\r\r"});

            //It is the number of characters minus one as the last one is backspace
            const unsigned short int Logger::numProgChars = progressChars.size() - 1;

            //Set the initial index to zerro
            unsigned short int Logger::currProgCharIdx = 0;

            //Set the initial update time to zero
            double Logger::lastProgressUpdate = 0.0;

            void Logger::startProgressBar() {
                if (currLEvel <= INFO) {
                    currProgCharIdx = 0;
                    cout << progressChars[currProgCharIdx];
                    lastProgressUpdate = StatisticsMonitor::getCPUTime();
                }
            }

            void Logger::updateProgressBar() {
                if (currLEvel <= INFO) {
                    const double currProgressUpdate = StatisticsMonitor::getCPUTime();
                    if ((currProgressUpdate - lastProgressUpdate) > PROGRESS_UPDATE_PERIOD) {
                        currProgCharIdx = (currProgCharIdx + 1) % numProgChars;
                        cout << progressChars[progressChars.size() - 1] << progressChars[currProgCharIdx];
                        cout.flush();
                        lastProgressUpdate = currProgressUpdate;
                    }
                }
            }

            void Logger::stopProgressBar() {
                if (currLEvel <= INFO) {
                    currProgCharIdx = 0;
                    lastProgressUpdate = 0.0;
                    cout << progressChars[numProgChars];
                }
            }

        }
    }
}