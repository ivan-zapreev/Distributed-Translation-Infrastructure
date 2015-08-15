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

#include <algorithm>    // std::transform

using uva::smt::monitore::StatisticsMonitor;

namespace uva {
    namespace smt {
        namespace logging {
            Logger::DebugLevel Logger::currLEvel = Logger::RESULT;

            const uint NUM_DEBUG_FLAGS = 9;
            const char * Logger::_debugLevelStr[NUM_DEBUG_FLAGS] = {USAGE_PARAM_VALUE,
                ERROR_PARAM_VALUE, WARNING_PARAM_VALUE, RESULT_PARAM_VALUE,
                INFO_PARAM_VALUE, DEBUG_PARAM_VALUE, DEBUG1_PARAM_VALUE,
                DEBUG2_PARAM_VALUE, DEBUG3_PARAM_VALUE};

            //Initialize the progress bar chars array
            const vector<string> Logger::progressChars({"///", "---", "\\\\\\", "|||", "\r\r\r"});

            //It is the number of characters minus one as the last one is backspace
            const unsigned short int Logger::numProgChars = progressChars.size() - 1;

            //Set the initial index to zerro
            unsigned short int Logger::currProgCharIdx = 0;

            //Set the initial update time to zero
            double Logger::lastProgressUpdate = 0.0;

            //The progress bar is not running first
            bool Logger::isPBOn = false;

            //Initialize the progress bar begin time
            clock_t Logger::beginTime = 0;
            //Initialize the previous output time string length
            size_t Logger::timeStrLen = 0;

            string Logger::getReportingLevels() {
                string result = "{ ";

                for (uint idx = 0; idx < NUM_DEBUG_FLAGS; idx++) {
                    string level = _debugLevelStr[idx];
                    transform(level.begin(), level.end(), level.begin(), ::toupper);
                    result += level + ", ";
                }

                return result + " }";
            }

            void Logger::setReportingLevel(string level) {
                bool isGoodLevel = true;
                transform(level.begin(), level.end(), level.begin(), ::toupper);

                if (!level.compare(USAGE_PARAM_VALUE)) {
                    Logger::ReportingLevel() = Logger::USAGE;
                } else {
                    if (!level.compare(RESULT_PARAM_VALUE)) {
                        Logger::ReportingLevel() = Logger::RESULT;
                    } else {
                        if (!level.compare(WARNING_PARAM_VALUE)) {
                            Logger::ReportingLevel() = Logger::WARNING;
                        } else {
                            if (!level.compare(INFO_PARAM_VALUE)) {
                                Logger::ReportingLevel() = Logger::INFO;
                            } else {
                                if (!level.compare(DEBUG_PARAM_VALUE)) {
                                    Logger::ReportingLevel() = Logger::DEBUG;
                                } else {
                                    if (!level.compare(DEBUG1_PARAM_VALUE)) {
                                        Logger::ReportingLevel() = Logger::DEBUG1;
                                    } else {
                                        if (!level.compare(DEBUG2_PARAM_VALUE)) {
                                            Logger::ReportingLevel() = Logger::DEBUG2;
                                        } else {
                                            if (!level.compare(DEBUG3_PARAM_VALUE)) {
                                                Logger::ReportingLevel() = Logger::DEBUG3;
                                            } else {
                                                isGoodLevel = false;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                if (isGoodLevel) {
                    LOG_USAGE << "The debugging level is set to: \'" << level << "\'" << END_LOG;
                } else {
                    LOG_WARNING << "Ignoring an unsupported value of [debug-level] parameter: '" << level << "'" << END_LOG;
                }
            }

            string Logger::computeTimeString(const clock_t elapsedClockTime, size_t & timeStrLen) {
                const float timeSec = ( ((float) elapsedClockTime) / CLOCKS_PER_SEC );
                const uint minute = (((uint) timeSec) % 3600) / 60;
                const uint hour = ((uint) timeSec) / 3600;
                const float second = (float) (((uint) ((timeSec - minute * 60 - hour * 3600 )* 100))/100);
                string result = SSTR( hour ) + " hour(s) " + SSTR( minute ) + " minute(s) " + SSTR( second ) + " second(s) ";
                timeStrLen = result.size();
                return result;
            }

            string Logger::computeTimeClearString(const size_t length) {
                string result = "";
                for(size_t i = 0 ; i < length ; i++) {
                    result += "\r";
                }
                return result;
            }

#if 0

            void Logger::startProgressBar() {
                if (currLEvel <= INFO && !isPBOn) {
                    currProgCharIdx = 0;
                    cout << progressChars[currProgCharIdx];
                    lastProgressUpdate = StatisticsMonitor::getCPUTime();
                    isPBOn = true;
                }
            }

            void Logger::updateProgressBar() {
                if (currLEvel <= INFO && isPBOn) {
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
                if (currLEvel <= INFO && isPBOn) {
                    currProgCharIdx = 0;
                    lastProgressUpdate = 0.0;
                    cout << progressChars[numProgChars];
                    isPBOn = false;
                }
            }
#else

            void Logger::startProgressBar() {
                if (currLEvel <= INFO && !isPBOn) {
                    //Output the time string
                    cout << computeTimeString(beginTime, timeStrLen);

                    //Store the current time
                    beginTime = clock();

                    //Update the cpu time and set the progress bar on flag
                    lastProgressUpdate = StatisticsMonitor::getCPUTime();
                    isPBOn = true;
                }
            }

            void Logger::updateProgressBar() {
                if (currLEvel <= INFO && isPBOn) {
                    const double currProgressUpdate = StatisticsMonitor::getCPUTime();
                    if ((currProgressUpdate - lastProgressUpdate) > PROGRESS_UPDATE_PERIOD) {
                        
                        //Output the current time
                        cout << computeTimeClearString(timeStrLen) << computeTimeString(clock() - beginTime, timeStrLen);
                        cout.flush();

                        lastProgressUpdate = currProgressUpdate;
                    }
                }
            }

            void Logger::stopProgressBar() {
                if (currLEvel <= INFO && isPBOn) {
                    //Clear the progress
                    cout << computeTimeClearString(timeStrLen) << "\n";

                    //Reset class variables
                    beginTime = 0;
                    timeStrLen = 0;
                    lastProgressUpdate = 0.0;
                    isPBOn = false;
                }
            }
#endif
        }
    }
}