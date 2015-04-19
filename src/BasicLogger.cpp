/* 
 * File:   BasicLogger.cpp
 * Author: zapreevis
 * 
 * Created on April 19, 2015, 9:11 AM
 */

#include "BasicLogger.hpp"
#include <iostream>     // std::cout
#include <cstdio>
#include <string>
#include <sstream>

//This macro is used to spare some space in printing functions bodies
#define PRINT(MSG_TYPE, ARG_TYPE, MSG_PREFIX, LAST_ARG, OSTREAM, ARG_VALUE)  \
    {                                                                        \
        va_list args;                                                        \
        va_start(args, LAST_ARG);                                            \
        print<MSG_TYPE, ARG_TYPE, MSG_PREFIX, OSTREAM>(ARG_VALUE, args);     \
        va_end(args);                                                        \
    }

const int BasicLogger::BUFFER_SIZE = 2*1024;

char BasicLogger::buffer[BUFFER_SIZE];

const string BasicLogger::RESULT_PREFIX_STR = "RESULT";
const string BasicLogger::USAGE_PREFIX_STR = "USAGE";
const string BasicLogger::ERROR_PREFIX_STR = "ERROR";
const string BasicLogger::WARNING_PREFIX_STR = "WARNING";
const string BasicLogger::INFO_PREFIX_STR = "INFO";
const string BasicLogger::DEBUG_PREFIX_STR = "DEBUG";

BasicLogger::DebugLevel BasicLogger::currLEvel = INFO;

void BasicLogger::printError( Exception const & data, ... ) {
    PRINT(ERROR, const string &, ERROR_PREFIX_STR, data, cerr, data.getMessage());
}

void BasicLogger::printError( const string & data, ... ) {
    PRINT(ERROR, const string &, ERROR_PREFIX_STR, data, cerr, data);
}

void BasicLogger::printError( const char * data, ... ) {
    PRINT(ERROR, const char *, ERROR_PREFIX_STR, data, cerr, data);
}

void BasicLogger::printWarning( const string & data, ... ) {
    PRINT(WARNING, const string &, WARNING_PREFIX_STR, data, cerr, data);
}

void BasicLogger::printWarning( const char * data, ... ) {
    PRINT(WARNING, const char *, WARNING_PREFIX_STR, data, cerr, data);
}

void BasicLogger::printResult( const string & data, ... ) {
    PRINT(RESULT, const string &, RESULT_PREFIX_STR, data, cout, data);
}

void BasicLogger::printResult( const char * data, ... ) {
    PRINT(RESULT, const char *, RESULT_PREFIX_STR, data, cout, data);
}

void BasicLogger::printUsage( const string & data, ... ) {
    PRINT(USAGE, const string &, USAGE_PREFIX_STR, data, cout, data);
}

void BasicLogger::printUsage( const char * data, ... ) {
    PRINT(USAGE, const char *, USAGE_PREFIX_STR, data, cout, data);
}

void BasicLogger::printInfo( const string & data, ... ) {
    PRINT(INFO, const string &, INFO_PREFIX_STR, data, cout, data);
}

void BasicLogger::printInfo( const char * data, ... ) {
    PRINT(INFO, const char *, INFO_PREFIX_STR, data, cout, data);
}

void BasicLogger::printUnsafeDebug( const string & data ) {
    print<DEBUG, const string &, DEBUG_PREFIX_STR, clog>(data);
}

void BasicLogger::printDebug( const string & data, ... ) {
    PRINT(DEBUG, const string &, DEBUG_PREFIX_STR, data, clog, data);
}

void BasicLogger::printDebug( const char * data, ... ) {
    PRINT(DEBUG, const char *, DEBUG_PREFIX_STR, data, clog, data);
}

template<BasicLogger::DebugLevel L, typename T, const string & P, ostream & S> void BasicLogger::print(T data, va_list &args) {
    if( L <= currLEvel ) {
        stringstream dataStream;
        dataStream << data;
        const char * dataCStr = dataStream.str().c_str();
        print<L,P,S>(dataCStr, args);
    }
}

template<BasicLogger::DebugLevel L, const string & P, ostream & S> void BasicLogger::print(const char * data, va_list &args) {
    if( L <= currLEvel ) {
        vsprintf(buffer, data, args);
        S << P << ": " << buffer << endl;
    }
}


template<BasicLogger::DebugLevel L, typename T, const string & P, ostream & S> void BasicLogger::print(T data) {
    if( L <= currLEvel ) {
        S << P << ": " << data << endl;
    }
}