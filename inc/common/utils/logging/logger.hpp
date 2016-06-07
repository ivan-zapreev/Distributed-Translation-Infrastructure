/* 
 * File:   logger.hpp
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
#define LOGGER_HPP

#include <mutex>
#include <iostream>  // std::cout
#include <sstream>   // std::stringstream
#include <vector>    // std::vector
#include <time.h>    // std::clock std::clock_t
#include <string.h>

using namespace std;

namespace uva {
    namespace utils {
        namespace logging {

            /**
             * This enumeration stores all the available logging levels.
             */
            enum debug_levels_enum {
                ERROR = 0, WARNING = ERROR + 1, USAGE = WARNING + 1, RESULT = USAGE + 1,
                INFO = RESULT + 1, INFO1 = INFO + 1, INFO2 = INFO1 + 1, INFO3 = INFO2 + 1,
                DEBUG = INFO3 + 1, DEBUG1 = DEBUG + 1, DEBUG2 = DEBUG1 + 1, DEBUG3 = DEBUG2 + 1,
                DEBUG4 = DEBUG3 + 1, size = DEBUG4 + 1
            };

            //Defines the maximum logging level
            static constexpr debug_levels_enum MAXIMUM_LOGGING_LEVEL = INFO3;

            //Defines the log level from which the detailed timing info is available
            static constexpr debug_levels_enum PROGRESS_ACTIVE_LEVEL = INFO1;

            /**
             * This structures stores the recursive synchronization mutex for logging.
             * The mutex is to be recursive as functions called when logging can do the own logging.
             */
            struct logging_synch {
                //Define the recursive lock quard
                typedef lock_guard<recursive_mutex> rec_scoped_lock;
                //The recursive mutex to be used for logging
                static recursive_mutex mv;
            };

            //This Macro is used to convert numerival values to proper strings!
#define SSTR( x ) std::dec << (x)

            //The macros needed to get the line sting for proper printing in cout
#define STRINGIZE(x) STRINGIZE2(x)
#define STRINGIZE2(x) #x
#define LINE_STRING STRINGIZE(__LINE__)

            //Defines the progress bar update period in CPU seconds
#define PROGRESS_UPDATE_PERIOD 0.05

#define LOGGER(level)                                               \
  if (level > MAXIMUM_LOGGING_LEVEL) ;                             \
  else if (level > logger::get_reporting_level()) ;                 \
       else {                                                       \
            logging_synch::rec_scoped_lock lock(logging_synch::mv); \
            logger::get(level)

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define LOGGER_DEBUG(level)                                             \
  if (level > MAXIMUM_LOGGING_LEVEL) ;                                 \
  else if (level > logger::get_reporting_level()) ;                     \
       else {                                                           \
            logging_synch::rec_scoped_lock lock(logging_synch::mv);     \
            logger::get(level, __FILENAME__, __FUNCTION__, LINE_STRING)

            //The Macro commands to be used for logging data with different log levels,
            //For example, to log a warning one can use:
            //      LOG_WARNING << "This is a warning message!" << END_LOG;
            //Here, the END_LOG is optional and is currently used for a new line only.
#define LOG_ERROR   LOGGER(debug_levels_enum::ERROR)
#define LOG_WARNING LOGGER(debug_levels_enum::WARNING)
#define LOG_USAGE   LOGGER(debug_levels_enum::USAGE)
#define LOG_RESULT  LOGGER(debug_levels_enum::RESULT)
#define LOG_INFO    LOGGER(debug_levels_enum::INFO)
#define LOG_INFO1    LOGGER(debug_levels_enum::INFO1)
#define LOG_INFO2    LOGGER(debug_levels_enum::INFO2)
#define LOG_INFO3    LOGGER(debug_levels_enum::INFO3)
#define LOG_DEBUG   LOGGER_DEBUG(debug_levels_enum::DEBUG)
#define LOG_DEBUG1  LOGGER_DEBUG(debug_levels_enum::DEBUG1)
#define LOG_DEBUG2  LOGGER_DEBUG(debug_levels_enum::DEBUG2)
#define LOG_DEBUG3  LOGGER_DEBUG(debug_levels_enum::DEBUG3)
#define LOG_DEBUG4  LOGGER_DEBUG(debug_levels_enum::DEBUG4)

#define END_LOG     endl << flush; \
}


            //The string representation values for debug levels
#define ERROR_PARAM_VALUE "ERROR"
#define WARNING_PARAM_VALUE "WARN"
#define USAGE_PARAM_VALUE "USAGE"
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

            //Define the white space separator used when logging
#define WHITE_SPACE_SEPARATOR " "

            std::ostream& operator<<(std::ostream& stream, const unsigned char & value);

            std::ostream& operator<<(std::ostream& stream, const signed char & value);

            /**
             * This is a trivial logging facility that exchibits a singleton
             * behavior and does output to stderr and stdout.
             */
            class logger {
            public:

                virtual ~logger() {
                };

                /**
                 * Allows to retrieve the list of supporter logging levels
                 * @param p_reporting_levels the pointer to the logging levels vector to be filled in
                 */
                static void get_reporting_levels(vector<string> * p_reporting_levels);

                /**
                 * Allows to set the logging level from a string, if not recognized - reports a warning!
                 * @param level the string level to set
                 */
                static void set_reporting_level(const string level);

                /**
                 * This methods allows to get the output stream for the given log-level
                 * @param level the log level for the messages to print
                 * @return the output stream object
                 */
                static inline std::ostream& get(debug_levels_enum level) {
                    return cout << m_debug_level_str[level] << ":" << WHITE_SPACE_SEPARATOR;
                }

                /**
                 * This methods allows to get the output stream for the given log-level
                 * @param level the log level for the messages to print
                 * @return the output stream object
                 */
                static inline std::ostream& get(debug_levels_enum level, const char * file, const char * func, const char * line) {
                    return cout << m_debug_level_str[level] << WHITE_SPACE_SEPARATOR << "<"
                            << file << "::" << func << "(...):" << line << ">:" << WHITE_SPACE_SEPARATOR;
                }

                /**
                 * Checks if the current reporting level is higher or equal to the given
                 * @return the reporting level to check
                 * @return true if the given reporting level is smaller or equal to the current, otherwise false
                 */
                static inline bool is_relevant_level(const debug_levels_enum& level) {
                    return level <= m_curr_level;
                };

                /**
                 * Returns the reference to the internal log level variable
                 * @return the reference to the internal log level variable
                 */
                static inline debug_levels_enum& get_reporting_level() {
                    return m_curr_level;
                };

                /**
                 * Allows to obtain the current reporting level string
                 * @return the current reporting level string
                 */
                static inline const string get_curr_level_str() {
                    return m_debug_level_str[m_curr_level];
                };

                /**
                 * The function that start progress bar
                 * Works if the current debug level is <= INFO
                 * @param msg the message to display
                 */
                static void start_progress_bar(const string & msg);

                /**
                 * The function that updates progress bar
                 * Works if the current debug level is <= INFO
                 */
                static void update_progress_bar();

                /**
                 * The function that stops progress bar
                 * Works if the current debug level is <= INFO
                 */
                static void stop_progress_bar();

                /**
                 * The function allows to check if the progress bar is running or not
                 * @return true if the progress bar is running, otherwise case;
                 */
                static inline bool is_progress_bar_on() {
                    return m_is_pb_on;
                };

            private:
                //The class constructor, copy constructor and assign operator
                //are made private as they are not to be used.

                //Stores the the string representation of the the DebugLevel enumeration elements
                static const char * m_debug_level_str[];

                //Stores the flag indicating if the progress bar is running or not
                static bool m_is_pb_on;

                //The action message to display
                static string m_prefix;

                //Stores the progress begin time
                static clock_t m_begin_time;
                //Stores the length of the previously output time
                static size_t m_time_str_len;

                logger();
                ;

                logger(const logger&) {
                };

                logger& operator=(const logger&) {
                    return *this;
                };

                /**
                 * Allow to compute the elapsed clock time string based on the given elapsed clock time
                 * @param elapsedClockTime the elapsed clock time
                 * @param timeStrLen the output parameter - the number of characters in the clock time string
                 * @return the clock time string
                 */
                static string compute_time_string(const clock_t elapsedClockTime, size_t & timeStrLen);

                /**
                 * Allows to compute the clear string with the given length
                 * @param length the length of the string to clear
                 * @return the clearing string
                 */
                static string compute_time_clear_string(const size_t length);

                //Stores the current used message level
                static debug_levels_enum m_curr_level;

                //Stores the progress bar characters
                static const vector<string> m_progress_chars;

                //Stores the number of progress characters
                static const unsigned short int m_num_prog_chars;

                //Stores the current progress element idx
                static unsigned short int m_curr_prog_char_idx;

                //Stores the last progress update in CPU seconds
                static unsigned int m_update_counter;

            };
        }
    }
}
#endif /* LOGGER_HPP */

