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
 * Created on July 26, 2015, 3:49 PM
 */

#ifndef LOGGER_HPP
#define	LOGGER_HPP

#include <iostream>  // std::cout
#include <sstream>   // std::stringstream
#include <vector>    // std::vector
#include <time.h>    // std::clock std::clock_t

#include "Globals.hpp"
#include "Exceptions.hpp"

using namespace std;

namespace uva {
    namespace smt {
        namespace logging {

            //This Macro is used to convert numerival values to proper strings!
#define SSTR( x ) std::dec << (x)

            //The macros needed to get the line sting for proper printing in cout
#define STRINGIZE(x) STRINGIZE2(x)
#define STRINGIZE2(x) #x
#define LINE_STRING STRINGIZE(__LINE__)

            //Defines the progress bar update period in CPU seconds
#define PROGRESS_UPDATE_PERIOD 0.05

#define LOGGER(level)                          \
  if (level > LOGER_MAX_LEVEL) ;               \
  else if (level > Logger::getReportingLevel()) ; \
       else Logger::get(level)

#define LOGGER_DEBUG(level)                    \
  if (level > LOGER_MAX_LEVEL) ;               \
  else if (level > Logger::getReportingLevel()) ; \
       else Logger::get(level, __FILE__, __FUNCTION__, LINE_STRING)

            //The Macro commands to be used for logging data with different log levels,
            //For example, to log a warning one can use:
            //      LOG_WARNING << "This is a warning message!" << END_LOG;
            //Here, the END_LOG is optional and is currently used for a new line only.
#define LOG_USAGE   LOGGER(DebugLevelsEnum::USAGE)
#define LOG_RESULT  LOGGER(DebugLevelsEnum::RESULT)
#define LOG_ERROR   LOGGER(DebugLevelsEnum::ERROR)
#define LOG_WARNING LOGGER(DebugLevelsEnum::WARNING)
#define LOG_INFO    LOGGER(DebugLevelsEnum::INFO)
#define LOG_INFO1    LOGGER(DebugLevelsEnum::INFO1)
#define LOG_INFO2    LOGGER(DebugLevelsEnum::INFO2)
#define LOG_INFO3    LOGGER(DebugLevelsEnum::INFO3)
#define LOG_DEBUG   LOGGER_DEBUG(DebugLevelsEnum::DEBUG)
#define LOG_DEBUG1  LOGGER_DEBUG(DebugLevelsEnum::DEBUG1)
#define LOG_DEBUG2  LOGGER_DEBUG(DebugLevelsEnum::DEBUG2)
#define LOG_DEBUG3  LOGGER_DEBUG(DebugLevelsEnum::DEBUG3)
#define LOG_DEBUG4  LOGGER_DEBUG(DebugLevelsEnum::DEBUG4)
#define END_LOG     endl << flush


            //The string representation values for debug levels
#define USAGE_PARAM_VALUE "USAGE"
#define ERROR_PARAM_VALUE "ERROR"
#define WARNING_PARAM_VALUE "WARN"
#define RESULT_PARAM_VALUE "RESULT"
#define INFO_PARAM_VALUE "INFO"
#define INFO1_PARAM_VALUE "INFO1"
#define INFO2_PARAM_VALUE "INFO2"
#define INFO3_PARAM_VALUE "INFO3"
#define DEBUG_PARAM_VALUE "DEBUG"
#define DEBUG1_PARAM_VALUE "DEBUG1"
#define DEBUG2_PARAM_VALUE "DEBUG2"
#define DEBUG3_PARAM_VALUE "DEBUG3"
#define DEBUG4_PARAM_VALUE "DEBUG4"

            /**
             * This is a trivial logging facility that exchibits a singleton
             * behavior and does output to stderr and stdout.
             */
            class Logger {
            public:

                virtual ~Logger() {
                };

                /**
                 * Returns a string containing all the possible reporting levels
                 * @return a string containing all the possible reporting levels
                 */
                static string getReportingLevels();

                /**
                 * Allows to set the logging level from a string, if not recognized - reports a warning!
                 * @param level the string level to set
                 */
                static void setReportingLevel(const string level);

                /**
                 * This methods allows to get the output stream for the given log-level
                 * @param level the log level for the messages to print
                 * @return the output stream object
                 */
                static inline std::ostream& get(DebugLevelsEnum level) {
                    return cout << _debugLevelStr[level] << ":\t";
                }

                /**
                 * This methods allows to get the output stream for the given log-level
                 * @param level the log level for the messages to print
                 * @return the output stream object
                 */
                static inline std::ostream& get(DebugLevelsEnum level, const char * file, const char * func, const char * line) {
                    return cout << _debugLevelStr[level] << " \t<" << file << "::" << func << "(...):" << line << ">:\t";
                }

                /**
                 * Checks if the current reporting level is higher or equal to the given
                 * @return the reporting level to check
                 * @return true if the given reporting level is smaller or equal to the current, otherwise false
                 */
                static inline bool isRelevantLevel(const DebugLevelsEnum& level) {
                    return level <= currLEvel;
                };

                /**
                 * Returns the reference to the internal log level variable
                 * @return the reference to the internal log level variable
                 */
                static inline DebugLevelsEnum& getReportingLevel() {
                    return currLEvel;
                };

                /**
                 * The function that start progress bar
                 * Works if the current debug level is <= INFO
                 * @param msg the message to display
                 */
                static void startProgressBar(const string & msg);

                /**
                 * The function that updates progress bar
                 * Works if the current debug level is <= INFO
                 */
                static void updateProgressBar();

                /**
                 * The function that stops progress bar
                 * Works if the current debug level is <= INFO
                 */
                static void stopProgressBar();

                /**
                 * The function allows to check if the progress bar is running or not
                 * @return true if the progress bar is running, otherwise case;
                 */
                static inline bool isProgressBarOn() {
                    return isPBOn;
                };

            private:
                //The class constructor, copy constructor and assign operator
                //are made private as they are not to be used.

                //Stores the the string representation of the the DebugLevel enumeration elements
                static const char * _debugLevelStr[];

                //Stores the flag indicating if the progress bar is running or not
                static bool isPBOn;

                //The action message to display
                static string prefix;

                //Stores the progress begin time
                static clock_t beginTime;
                //Stores the length of the previously output time
                static size_t timeStrLen;

                Logger() {
                };

                Logger(const Logger&) {
                };

                Logger& operator=(const Logger&) {
                    return *this;
                };

                /**
                 * Allow to compute the elapsed clock time string based on the given elapsed clock time
                 * @param elapsedClockTime the elapsed clock time
                 * @param timeStrLen the output parameter - the number of characters in the clock time string
                 * @return the clock time string
                 */
                static string computeTimeString(const clock_t elapsedClockTime, size_t & timeStrLen);

                /**
                 * Allows to compute the clear string with the given length
                 * @param length the length of the string to clear
                 * @return the clearing string
                 */
                static string computeTimeClearString(const size_t length);

                //Stores the current used message level
                static DebugLevelsEnum currLEvel;

                //Stores the progress bar characters
                static const vector<string> progressChars;

                //Stores the number of progress characters
                static const unsigned short int numProgChars;

                //Stores the current progress element idx
                static unsigned short int currProgCharIdx;

                //Stores the last progress update in CPU seconds
                static unsigned int updateCounter;

            };

        }
    }
}
#endif	/* LOGGER_HPP */

