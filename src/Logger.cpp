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

            std::ostream& operator<<(std::ostream& stream, const unsigned char & value) {
                return stream << ((uint32_t) value);
            }

            std::ostream& operator<<(std::ostream& stream, const signed char & value) {
                return stream << ((int32_t) value);
            }

            DebugLevelsEnum Logger::currLEvel = DebugLevelsEnum::RESULT;

            const char * Logger::_debugLevelStr[DebugLevelsEnum::size] = {
                ERROR_PARAM_VALUE, WARNING_PARAM_VALUE, USAGE_PARAM_VALUE, RESULT_PARAM_VALUE,
                INFO_PARAM_VALUE, INFO1_PARAM_VALUE, INFO2_PARAM_VALUE, INFO3_PARAM_VALUE,
                DEBUG_PARAM_VALUE, DEBUG1_PARAM_VALUE, DEBUG2_PARAM_VALUE, DEBUG3_PARAM_VALUE,
                DEBUG4_PARAM_VALUE
            };

            //Initialize the progress bar chars array
            const vector<string> Logger::progressChars({"///", "---", "\\\\\\", "|||", "\r\r\r"});

            //It is the number of characters minus one as the last one is backspace
            const unsigned short int Logger::numProgChars = progressChars.size() - 1;

            //Set the initial index to zerro
            unsigned short int Logger::currProgCharIdx = 0;

            //Set the initial update time to zero
            unsigned int Logger::updateCounter = 0.0;

            //The progress bar is not running first
            bool Logger::isPBOn = false;

            //The action message to display
            string Logger::prefix = "";

            //Initialize the progress bar begin time
            clock_t Logger::beginTime = 0;
            //Initialize the previous output time string length
            size_t Logger::timeStrLen = 0;

            string Logger::getReportingLevels() {
                string result = "{ ";

                for (size_t idx = 0; idx < DebugLevelsEnum::size; idx++) {
                    string level = _debugLevelStr[idx];
                    transform(level.begin(), level.end(), level.begin(), ::toupper);
                    if (idx != (DebugLevelsEnum::size - 1)) {
                        result += level + ", ";
                    }
                }

                return result + " }";
            }

            void Logger::setReportingLevel(string level) {
                //ToDo: This function is ugly improve it by using a map, or a
                //      similar so that we could just get an appropriate level
                //      for the string.
                bool isGoodLevel = true;
                transform(level.begin(), level.end(), level.begin(), ::toupper);

                if (!level.compare(USAGE_PARAM_VALUE)) {
                    Logger::getReportingLevel() = DebugLevelsEnum::USAGE;
                } else {
                    if (!level.compare(RESULT_PARAM_VALUE)) {
                        Logger::getReportingLevel() = DebugLevelsEnum::RESULT;
                    } else {
                        if (!level.compare(WARNING_PARAM_VALUE)) {
                            Logger::getReportingLevel() = DebugLevelsEnum::WARNING;
                        } else {
                            if (!level.compare(INFO_PARAM_VALUE)) {
                                Logger::getReportingLevel() = DebugLevelsEnum::INFO;
                            } else {
                                if (!level.compare(INFO1_PARAM_VALUE)) {
                                    Logger::getReportingLevel() = DebugLevelsEnum::INFO1;
                                } else {
                                    if (!level.compare(INFO2_PARAM_VALUE)) {
                                        Logger::getReportingLevel() = DebugLevelsEnum::INFO2;
                                    } else {
                                        if (!level.compare(INFO3_PARAM_VALUE)) {
                                            Logger::getReportingLevel() = DebugLevelsEnum::INFO3;
                                        } else {
                                            if (!level.compare(DEBUG_PARAM_VALUE)) {
                                                Logger::getReportingLevel() = DebugLevelsEnum::DEBUG;
                                            } else {
                                                if (!level.compare(DEBUG1_PARAM_VALUE)) {
                                                    Logger::getReportingLevel() = DebugLevelsEnum::DEBUG1;
                                                } else {
                                                    if (!level.compare(DEBUG2_PARAM_VALUE)) {
                                                        Logger::getReportingLevel() = DebugLevelsEnum::DEBUG2;
                                                    } else {
                                                        if (!level.compare(DEBUG3_PARAM_VALUE)) {
                                                            Logger::getReportingLevel() = DebugLevelsEnum::DEBUG3;
                                                        } else {
                                                            if (!level.compare(DEBUG4_PARAM_VALUE)) {
                                                                Logger::getReportingLevel() = DebugLevelsEnum::DEBUG4;
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
                            }
                        }
                    }
                }
                if (isGoodLevel) {
                    LOG_USAGE << "The requested debug level is: \'" << level
                            << "\', the maximum build level is '"
                            << _debugLevelStr[LOGER_MAX_LEVEL] << "'"
                            << " the set level is '" << _debugLevelStr[currLEvel]
                            << "'" << END_LOG;
                } else {
                    LOG_WARNING << "Ignoring an unsupported value of [debug-level] parameter: '" << level << "'" << END_LOG;
                }
            }

            string Logger::computeTimeString(const clock_t elapsedClockTime, size_t & timeStrLen) {
                const float timeSec = (((float) elapsedClockTime) / CLOCKS_PER_SEC);
                const uint minute = (((uint) timeSec) % 3600) / 60;
                const uint hour = ((uint) timeSec) / 3600;
                const float second = (float) (((uint) ((timeSec - minute * 60 - hour * 3600)* 100)) / 100);
                stringstream msg;
                msg << " " << SSTR(hour) << " hour(s) " << SSTR(minute) << " minute(s) " << SSTR(second) << " second(s) ";
                string result = prefix + msg.str();
                timeStrLen = result.size();
                return result;
            }

            string Logger::computeTimeClearString(const size_t length) {
                string result = "";
                for (size_t i = 0; i < length; i++) {
                    result += "\r";
                }
                return result;
            }

            //This macro is used to check if we need to do the progress indications
#define IS_ENOUGH_LOGGING_LEVEL(level) (( PROGRESS_ACTIVE_LEVEL <= LOGER_MAX_LEVEL ) && ( PROGRESS_ACTIVE_LEVEL <= level ))

            void Logger::startProgressBar(const string & msg) {
                if (IS_ENOUGH_LOGGING_LEVEL(currLEvel)) {
                    if (!isPBOn) {
                        stringstream pref;
                        pref << _debugLevelStr[PROGRESS_ACTIVE_LEVEL] << ":\t" << msg << ":\t";
                        prefix = pref.str();

                        //Output the time string
                        cout << computeTimeString(beginTime, timeStrLen);
                        cout.flush();

                        //Store the current time
                        beginTime = clock();

                        //Update the update counter and set the progress bar on flag
                        updateCounter = 0;
                        isPBOn = true;
                    }
                } else {
                    LOG_INFO << msg << END_LOG;
                }
            }

            void Logger::updateProgressBar() {
                if (IS_ENOUGH_LOGGING_LEVEL(currLEvel)) {
                    //Do not update each time to save on computations
                    if (updateCounter > (CLOCKS_PER_SEC / 4)) {

                        //Output the current time
                        cout << computeTimeClearString(timeStrLen) << computeTimeString(clock() - beginTime, timeStrLen);
                        cout.flush();

                        updateCounter = 0;
                    } else {

                        updateCounter++;
                    }
                }
            }

            void Logger::stopProgressBar() {
                if (IS_ENOUGH_LOGGING_LEVEL(currLEvel) && isPBOn) {
                    //Clear the progress
                    cout << computeTimeClearString(timeStrLen) << "\n";
                    cout.flush();

                    //Reset class variables
                    prefix = "";
                    beginTime = 0;
                    timeStrLen = 0;
                    updateCounter = 0;
                    isPBOn = false;
                }
            }
        }
    }
}