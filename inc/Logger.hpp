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

#include <iostream>  // std::cout
#include <sstream>   // std::stringstream
#include <vector>    // std::vector

#include "Exceptions.hpp"

using namespace std;

#ifndef LOGGER_HPP
#define	LOGGER_HPP

//Defines the progress bar update period in CPU seconds
#define PROGRESS_UPDATE_PERIOD 0.1

//The logging macros to be used that allows for compile-time as well as runtime optimization
#ifndef LOGER_MAX_LEVEL
#define LOGER_MAX_LEVEL Logger::DEBUG2
#endif
#define LOGGER(level)                          \
  if (level > LOGER_MAX_LEVEL) ;               \
  else if (level > Logger::ReportingLevel()) ; \
       else Logger::Get(level)

/**
 * This is a trivial logging facility that exchibits a singleton
 * behavior and does output to stderr and stdout.
 */
class Logger {
public:

    //This enumeration stores all the available logging levels.
    enum DebugLevel {
        USAGE = 0, RESULT = USAGE + 1, ERROR = RESULT + 1, WARNING = ERROR + 1, INFO = WARNING + 1, DEBUG = INFO + 1, DEBUG1 = DEBUG + 1, DEBUG2 = DEBUG1 + 1
    };

    //Stores the the string representation of the sthe DebugLeven enumeration elements
    static const char * DebugLevelStr[];
    
    virtual ~Logger(){};

    /**
     * This methods allows to get the output stream for the given log-level
     * @param level the log level for the messages to print
     * @return the output stream object
     */
    static std::ostream& Get(DebugLevel level = INFO);
    
    /**
     * Returns the reference to the internal log level variable
     * @return the reference to the internal log level variable
     */
    static inline DebugLevel& ReportingLevel() { return currLEvel; };

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
    
private:
    //The class constructor, copy constructor and assign operator
    //are made private as they are not to be used.
    Logger(){};
    Logger(const Logger&) {};
    Logger& operator=(const Logger&) { return *this;};

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

#endif	/* LOGGER_HPP */

