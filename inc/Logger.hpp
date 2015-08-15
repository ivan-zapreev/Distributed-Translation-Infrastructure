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

#include "Exceptions.hpp"

using namespace std;

namespace uva {
    namespace smt {
        namespace logging {

            //This Macro is used to convert numerival values to proper strings!
#define SSTR( x ) dynamic_cast< std::ostringstream & >( \
        ( std::ostringstream() << std::dec << x ) ).str()

            //The macros needed to get the line sting for proper printing in cout
#define STRINGIZE(x) STRINGIZE2(x)
#define STRINGIZE2(x) #x
#define LINE_STRING STRINGIZE(__LINE__)

            //Defines the progress bar update period in CPU seconds
#define PROGRESS_UPDATE_PERIOD 0.05

            //The logging macros to be used that allows for compile-time as well as runtime optimization
#ifndef LOGER_MAX_LEVEL
#define LOGER_MAX_LEVEL Logger::DEBUG3
#endif

#define LOGGER(level)                          \
  if (level > LOGER_MAX_LEVEL) ;               \
  else if (level > Logger::ReportingLevel()) ; \
       else Logger::Get(level)


#define LOGGER_DEBUG(level)                    \
  if (level > LOGER_MAX_LEVEL) ;               \
  else if (level > Logger::ReportingLevel()) ; \
       else Logger::Get(level, __FILE__, __FUNCTION__, LINE_STRING)

            //The Macro commands to be used for logging data with different log levels,
            //For example, to log a warning one can use:
            //      LOG_WARNING << "This is a warning message!" << END_LOG;
            //Here, the END_LOG is optional and is currently used for a new line only.
#define LOG_USAGE   LOGGER(Logger::USAGE)
#define LOG_RESULT  LOGGER(Logger::RESULT)
#define LOG_ERROR   LOGGER(Logger::ERROR)
#define LOG_WARNING LOGGER(Logger::WARNING)
#define LOG_INFO    LOGGER(Logger::INFO)
#define LOG_DEBUG   LOGGER_DEBUG(Logger::DEBUG)
#define LOG_DEBUG1  LOGGER_DEBUG(Logger::DEBUG1)
#define LOG_DEBUG2  LOGGER_DEBUG(Logger::DEBUG2)
#define LOG_DEBUG3  LOGGER_DEBUG(Logger::DEBUG3)
#define END_LOG     endl


            //The string representation values for debug levels
#define USAGE_PARAM_VALUE "USAGE"
#define ERROR_PARAM_VALUE "ERROR"
#define WARNING_PARAM_VALUE "WARNING"
#define RESULT_PARAM_VALUE "RESULT"
#define INFO_PARAM_VALUE "INFO"
#define DEBUG_PARAM_VALUE "DEBUG"
#define DEBUG1_PARAM_VALUE "DEBUG1"
#define DEBUG2_PARAM_VALUE "DEBUG2"
#define DEBUG3_PARAM_VALUE "DEBUG3"

            /**
             * This is a trivial logging facility that exchibits a singleton
             * behavior and does output to stderr and stdout.
             */
            class Logger {
            public:

                //This enumeration stores all the available logging levels.

                enum DebugLevel {
                    USAGE = 0, ERROR = USAGE + 1, WARNING = ERROR + 1, RESULT = WARNING + 1, INFO = RESULT + 1,
                    DEBUG = INFO + 1, DEBUG1 = DEBUG + 1, DEBUG2 = DEBUG1 + 1, DEBUG3 = DEBUG2 + 1
                };

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
                static inline std::ostream& Get(DebugLevel level) {
                    return cout << _debugLevelStr[level] << ":\t";
                }

                /**
                 * This methods allows to get the output stream for the given log-level
                 * @param level the log level for the messages to print
                 * @return the output stream object
                 */
                static inline std::ostream& Get(DebugLevel level, const char * file, const char * func, const char * line) {
                    return cout << _debugLevelStr[level] << " \t<" << file << "::" << func << "(...):" << line << ">:\t";
                }

                /**
                 * Returns the reference to the internal log level variable
                 * @return the reference to the internal log level variable
                 */
                static inline DebugLevel& ReportingLevel() {
                    return currLEvel;
                };

                /**
                 * The function that start progress bar
                 * Works if the current debug level is <= INFO
                 */
                static void startProgressBar();

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
                static DebugLevel currLEvel;

                //Stores the progress bar characters
                static const vector<string> progressChars;

                //Stores the number of progress characters
                static const unsigned short int numProgChars;

                //Stores the current progress element idx
                static unsigned short int currProgCharIdx;

                //Stores the last progress update in CPU seconds
                static double lastProgressUpdate;

            };

        }
    }
}
#endif	/* LOGGER_HPP */

