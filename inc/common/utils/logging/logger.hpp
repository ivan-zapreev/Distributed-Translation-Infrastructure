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
                static inline recursive_mutex & m_mv() {
                    static recursive_mutex mv;
                    return mv;
                }
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
logging_synch::rec_scoped_lock lock(logging_synch::m_mv()); \
logger::get(level)
            
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
            
#define LOGGER_DEBUG(level)                                             \
if (level > MAXIMUM_LOGGING_LEVEL) ;                                 \
else if (level > logger::get_reporting_level()) ;                     \
else {                                                           \
logging_synch::rec_scoped_lock lock(logging_synch::m_mv());     \
logger::get(level, __FILENAME__, __FUNCTION__, LINE_STRING)
            
            //The Macro commands to be used for logging data with different log levels,
            //For example, to log a warning one can use:
            //      LOG_WARNING << "This is a warning message!" << END_LOG;
            //Here, the END_LOG is optional and is currently used for a new line only.
#define LOG_ERROR   LOGGER_DEBUG(debug_levels_enum::ERROR)
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
                static void get_reporting_levels(vector<string> * p_reporting_levels) {
                    for (size_t level_id = debug_levels_enum::ERROR; level_id <= MAXIMUM_LOGGING_LEVEL; ++level_id) {
                        string level = m_debug_level_str()[level_id];
                        transform(level.begin(), level.end(), level.begin(), ::tolower);
                        p_reporting_levels->push_back(level);
                    }
                }
                
                /**
                 * Allows to set the logging level from a string, if not
                 * recognized - reports a warning!
                 * \todo {This function is ugly improve it by using a map, or a
                 * similar so that we could just get an appropriate level
                 * for the string.}
                 * @param level the string level to set
                 */
                static void set_reporting_level(string level) {
                    bool isGoodLevel = true;
                    debug_levels_enum new_debug_level = debug_levels_enum::USAGE;
                    transform(level.begin(), level.end(), level.begin(), ::toupper);
                    
                    if (!level.compare(USAGE_PARAM_VALUE)) {
                        new_debug_level = debug_levels_enum::USAGE;
                    } else {
                        if (!level.compare(RESULT_PARAM_VALUE)) {
                            new_debug_level = debug_levels_enum::RESULT;
                        } else {
                            if (!level.compare(WARNING_PARAM_VALUE)) {
                                new_debug_level = debug_levels_enum::WARNING;
                            } else {
                                if (!level.compare(INFO_PARAM_VALUE)) {
                                    new_debug_level = debug_levels_enum::INFO;
                                } else {
                                    if (!level.compare(INFO1_PARAM_VALUE)) {
                                        new_debug_level = debug_levels_enum::INFO1;
                                    } else {
                                        if (!level.compare(INFO2_PARAM_VALUE)) {
                                            new_debug_level = debug_levels_enum::INFO2;
                                        } else {
                                            if (!level.compare(INFO3_PARAM_VALUE)) {
                                                new_debug_level = debug_levels_enum::INFO3;
                                            } else {
                                                if (!level.compare(DEBUG_PARAM_VALUE)) {
                                                    new_debug_level = debug_levels_enum::DEBUG;
                                                } else {
                                                    if (!level.compare(DEBUG1_PARAM_VALUE)) {
                                                        new_debug_level = debug_levels_enum::DEBUG1;
                                                    } else {
                                                        if (!level.compare(DEBUG2_PARAM_VALUE)) {
                                                            new_debug_level = debug_levels_enum::DEBUG2;
                                                        } else {
                                                            if (!level.compare(DEBUG3_PARAM_VALUE)) {
                                                                new_debug_level = debug_levels_enum::DEBUG3;
                                                            } else {
                                                                if (!level.compare(DEBUG4_PARAM_VALUE)) {
                                                                    new_debug_level = debug_levels_enum::DEBUG4;
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
                        if( new_debug_level <= MAXIMUM_LOGGING_LEVEL  ) {
                            //Set the new level
                            m_curr_level() = new_debug_level;
                            //Log the usage info
                            LOG_USAGE << "The requested debug level is: \'" << level
                            << "\', the maximum build level is '"
                            << m_debug_level_str()[MAXIMUM_LOGGING_LEVEL] << "'"
                            << " the set level is '" << m_debug_level_str()[m_curr_level()]
                            << "'" << END_LOG;
                        } else {
                            LOG_WARNING << "The requested debug level: '" << level
                            << "' is higher than the maximum build level: "
                            << m_debug_level_str()[MAXIMUM_LOGGING_LEVEL] << ", Ignoring!" << END_LOG;
                        }
                    } else {
                        LOG_WARNING << "Ignoring an unsupported value of [debug-level] parameter: '" << level << "'" << END_LOG;
                    }
                }
                
                /**
                 * This methods allows to get the output stream for the given log-level
                 * @param level the log level for the messages to print
                 * @return the output stream object
                 */
                static inline std::ostream& get(debug_levels_enum level) {
                    return cout << m_debug_level_str()[level] << ":" << WHITE_SPACE_SEPARATOR;
                }
                
                /**
                 * This methods allows to get the output stream for the given log-level
                 * @param level the log level for the messages to print
                 * @param file the file name from which the logging was called
                 * @param func the function from which the logging was called
                 * @param line the line of code from which the logging was called
                 * @return the output stream object
                 */
                static inline std::ostream& get(debug_levels_enum level, const char * file, const char * func, const char * line) {
                    return cout << m_debug_level_str()[level] << WHITE_SPACE_SEPARATOR << "<"
                    << file << "::" << func << "(...):" << line << ">:" << WHITE_SPACE_SEPARATOR;
                }
                
                /**
                 * Checks if the current reporting level is higher or equal to the given
                 * @return the reporting level to check
                 * @return true if the given reporting level is smaller or equal to the current, otherwise false
                 */
                static inline bool is_relevant_level(const debug_levels_enum& level) {
                    return level <= m_curr_level();
                };
                
                /**
                 * Returns the reference to the internal log level variable
                 * @return the reference to the internal log level variable
                 */
                static inline debug_levels_enum& get_reporting_level() {
                    return m_curr_level();
                };
                
                /**
                 * Allows to obtain the current reporting level string
                 * @return the current reporting level string
                 */
                static inline const string get_curr_level_str() {
                    return m_debug_level_str()[m_curr_level()];
                };
                
                //This macro is used to check if we need to do the progress indications
#define IS_ENOUGH_LOGGING_LEVEL(level) \
(( PROGRESS_ACTIVE_LEVEL <= MAXIMUM_LOGGING_LEVEL ) && ( PROGRESS_ACTIVE_LEVEL <= level ))
                
                /**
                 * The function that start progress bar
                 * Works if the current debug level is <= INFO
                 * @param msg the message to display
                 */
                static void start_progress_bar(const string & msg) {
                    if (IS_ENOUGH_LOGGING_LEVEL(m_curr_level())) {
                        if (!m_is_pb_on()) {
                            stringstream pref;
                            pref << m_debug_level_str()[PROGRESS_ACTIVE_LEVEL] << ":"
                            << WHITE_SPACE_SEPARATOR << msg << ":" << WHITE_SPACE_SEPARATOR;
                            m_prefix() = pref.str();
                            
                            //Output the time string
                            cout << compute_time_string(m_begin_time(), m_time_str_len());
                            cout.flush();
                            
                            //Store the current time
                            m_begin_time() = clock();
                            
                            //Update the update counter and set the progress bar on flag
                            m_update_counter() = 0;
                            m_is_pb_on() = true;
                        }
                    } else {
                        LOG_INFO << msg << END_LOG;
                    }
                };
                
                /**
                 * The function that updates progress bar
                 * Works if the current debug level is <= INFO
                 */
                static void update_progress_bar() {
                    if (IS_ENOUGH_LOGGING_LEVEL(m_curr_level())) {
                        //Do not update each time to save on computations
                        if (m_update_counter() > (CLOCKS_PER_SEC / 4)) {
                            
                            //Output the current time
                            cout << compute_time_clear_string(m_time_str_len())
                            << compute_time_string(clock() - m_begin_time(), m_time_str_len());
                            cout.flush();
                            
                            m_update_counter() = 0;
                        } else {
                            
                            m_update_counter()++;
                        }
                    }
                };
                
                /**
                 * The function that stops progress bar
                 * Works if the current debug level is <= INFO
                 */
                static void stop_progress_bar() {
                    if (IS_ENOUGH_LOGGING_LEVEL(m_curr_level()) && m_is_pb_on()) {
                        //Clear the progress
                        cout << compute_time_clear_string(m_time_str_len()) << "\n";
                        cout.flush();
                        
                        //Reset class variables
                        m_prefix() = "";
                        m_begin_time() = 0;
                        m_time_str_len() = 0;
                        m_update_counter() = 0;
                        m_is_pb_on() = false;
                    }
                };
                
                /**
                 * The function allows to check if the progress bar is running or not
                 * @return true if the progress bar is running, otherwise case;
                 */
                static inline bool is_progress_bar_on() {
                    return m_is_pb_on();
                };
                
            private:
                //Stores the the string representation of the the DebugLevel enumeration elements
                static inline const string * m_debug_level_str() {
                    static const string debug_level_str[debug_levels_enum::size] = {
                        string(ERROR_PARAM_VALUE), string(WARNING_PARAM_VALUE),
                        string(USAGE_PARAM_VALUE), string(RESULT_PARAM_VALUE),
                        string(INFO_PARAM_VALUE), string(INFO1_PARAM_VALUE),
                        string(INFO2_PARAM_VALUE), string(INFO3_PARAM_VALUE),
                        string(DEBUG_PARAM_VALUE), string(DEBUG1_PARAM_VALUE),
                        string(DEBUG2_PARAM_VALUE), string(DEBUG3_PARAM_VALUE),
                        string(DEBUG4_PARAM_VALUE)
                    };
                    return debug_level_str;
                }
                
                //Stores the flag indicating if the progress bar is running or not
                static bool & m_is_pb_on(){
                    //The progress bar is not running first
                    static bool is_pb_on = false;
                    return is_pb_on;
                }
                
                //The action message to display
                static string & m_prefix(){
                    //The action message to display
                    static string prefix = "";
                    return prefix;
                }
                
                //Stores the progress begin time
                static clock_t & m_begin_time(){
                    //Initialize the progress bar begin time
                    static clock_t begin_time = 0;
                    return begin_time;
                }
                
                //Stores the length of the previously output time
                static size_t & m_time_str_len(){
                    //Initialize the previous output time string length
                    static size_t time_str_len = 0;
                    return time_str_len;
                }
                
                //The class constructor, copy constructor and assign operator
                //are made private as they are not to be used.
                
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
                static string compute_time_string(const clock_t elapsedClockTime, size_t & timeStrLen) {
                    const float timeSec = (((float) elapsedClockTime) / CLOCKS_PER_SEC);
                    const uint minute = (((uint) timeSec) % 3600) / 60;
                    const uint hour = ((uint) timeSec) / 3600;
                    const float second = (float) (((uint) ((timeSec - minute * 60 - hour * 3600)* 100)) / 100);
                    stringstream msg;
                    msg << " " << SSTR(hour) << " hour(s) " << SSTR(minute) << " minute(s) " << SSTR(second) << " second(s) ";
                    string result = m_prefix() + msg.str();
                    timeStrLen = result.size();
                    return result;
                }
                
                /**
                 * Allows to compute the clear string with the given length
                 * @param length the length of the string to clear
                 * @return the clearing string
                 */
                static inline string compute_time_clear_string(const size_t length) {
                    string result = "";
                    for (size_t i = 0; i < length; i++) {
                        result += "\r";
                    }
                    return result;
                }
                
                //Stores the current used message level
                static inline debug_levels_enum & m_curr_level(){
                    static debug_levels_enum curr_level = debug_levels_enum::RESULT;
                    return curr_level;
                }
                
                //Stores the last progress update in CPU seconds
                static inline unsigned int & m_update_counter(){
                    //Set the initial update time to zero
                    static unsigned int update_counter = 0.0;
                    return update_counter;
                }
                
            };
        }
    }
}
#endif /* LOGGER_HPP */

