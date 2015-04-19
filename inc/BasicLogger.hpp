/* 
 * File:   BasicLogger.hpp
 * Author: zapreevis
 *
 * Created on April 19, 2015, 9:11 AM
 */

#ifndef BASICLOGGER_HPP
#define	BASICLOGGER_HPP

#include "Exceptions.hpp"
#include <cstdarg>

/**
 * This is a trivial logging facility that exchibits a singleton
 * behavior and does output to stderr and stdout.
 */
class BasicLogger {
public:
    enum DebugLevel{USAGE=0, RESULT=USAGE+1, ERROR=RESULT+1, WARNING=ERROR+1, INFO=WARNING+1, DEBUG=INFO+1};

    /**
     * Prints a usage message to the stdout
     * @param data the message to be print
     */
    static void printUsage( const string & data, ... );
    
    /**
     * Prints a usage message to the stdout
     * @param data the message to be print
     */
    static void printUsage( const char * data, ... );
    
    /**
     * Prints a result message to the stdout
     * @param data the message to be print
     */
    static void printResult( const string & data, ... );
    
    /**
     * Prints a result message to the stdout
     * @param data the message to be print
     */
    static void printResult( const char * data, ... );
    
    /**
     * This function prints an error message obtained from the exception into stderr
     * @param data the exception object
     */
    static void printError( Exception const & data, ... );
    
    /**
     * Prints an error message to the stderr
     * @param data the message to be print
     */
    static void printError( const string & data, ... );
    
    /**
     * Prints an error message to the stderr
     * @param data the message to be print
     */
    static void printError( const char * data, ... );

    /**
     * Prints a warning message to the stderr
     * @param data the message to be print
     */
    static void printWarning( const string & data, ... );
    
    /**
     * Prints a warning message to the stderr
     * @param data the message to be print
     */
    static void printWarning( const char * data, ... );
    
    /**
     * Prints an info message to the stdout
     * @param data the message to be print
     */
    static void printInfo( const string & data, ... );
    
    /**
     * Prints an info message to the stdout
     * @param data the message to be print
     */
    static void printInfo( const char * data, ... );

    /**
     * Prints an debug message to the stdlog
     * @param data the message to be print
     */
    static void printUnsafeDebug( const string & data );
    
    /**
     * Prints an debug message to the stdlog
     * @param data the message to be print
     */
    static void printDebug( const string & data, ... );
    
    /**
     * Prints an debug message to the stdlog
     * @param data the message to be print
     */
    static void printDebug( const char * data, ... );
    
    static void setLoggingLevel(DebugLevel level) { currLEvel = level; };
    
    static DebugLevel getLoggingLevel() { return currLEvel; };
    
private:
    
    //The buffer size for printing parameterized strings
    static const int BUFFER_SIZE;
    
    //The message prefixes
    static const string RESULT_PREFIX_STR;
    static const string USAGE_PREFIX_STR;
    static const string ERROR_PREFIX_STR;
    static const string WARNING_PREFIX_STR;
    static const string INFO_PREFIX_STR;
    static const string DEBUG_PREFIX_STR;
    
    //Stores the current debug level
    static DebugLevel currLEvel;
    
    //Stores the buffer for printing
    static char buffer[];

    BasicLogger(){}
    BasicLogger(const BasicLogger& orig){}
    virtual ~BasicLogger(){}

    /**
     * This template function allows to print a message with a given prefix to a given output stream
     * @param message the message to be print
     * @param arg the message arguments to be print
     */
    template<BasicLogger::DebugLevel L, typename T, const string & P, ostream & S> static void print(T data, va_list &args);
    
    /**
     * This template function allows to print a message with a given prefix to a given output stream
     * @param message the message to be print
     * @param arg the message arguments to be print
     */
    template<BasicLogger::DebugLevel L, const string & P, ostream & S> static void print(const char * data, va_list &args);

    /**
     * This template function allows to print a message with a given prefix to a given output stream
     * @param message the message to be print
     */
    template<BasicLogger::DebugLevel L, typename T, const string & P, ostream & S> static void print(T data);
};

#endif	/* BASICLOGGER_HPP */

