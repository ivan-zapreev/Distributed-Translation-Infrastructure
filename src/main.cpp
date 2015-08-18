/* 
 * File:   main.cpp
 * Author: Dr. Ivan S. Zapreev
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
 * Created on April 18, 2015, 11:35 AM
 */

#include <string>       // std::string
#include <iostream>     // std::cout
#include <sstream>      // std::stringstream, std::stringbuf
#include <fstream>      // std::ifstream
#include <math.h>    //std::pow

#include "Exceptions.hpp"
#include "StatisticsMonitor.hpp"
#include "Logger.hpp"
#include "ATrie.hpp"
#include "ContextMultiHashMapTrie.hpp"
#include "ARPATrieBuilder.hpp"
#include "Globals.hpp"
#include "ARPAGramBuilder.hpp"
#include "StringUtils.hpp"
#include "MemoryMappedFileReader.hpp"

using namespace std;
using namespace uva::smt;
using namespace uva::smt::tries;
using namespace uva::smt::file;
using namespace uva::smt::tries::arpa;
using namespace uva::smt::logging;
using namespace uva::smt::utils::text;
using namespace uva::smt::exceptions;
using namespace uva::smt::monitore;

/**
 * This structure is needed to store the application parameters
 */
typedef struct {
    //The train file name
    string modelFileName;
    //The test file name
    string testFileName;
} TAppParams;

/**
 * This functions does nothing more but printing the program header information
 */
static void printInfo() {
    LOG_USAGE << " ------------------------------------------------------------------ " << END_LOG;
    LOG_USAGE << "|                 Back Off Language Model(s) for SMT     :)\\___/(: |" << END_LOG;
    LOG_USAGE << "|                       Software version 1.0             {(@)v(@)} |" << END_LOG;
    LOG_USAGE << "|                                                        {|~- -~|} |" << END_LOG;
    LOG_USAGE << "|             Copyright (C) Dr. Ivan S Zapreev, 2015     {/^'^'^\\} |" << END_LOG;
    LOG_USAGE << "|  ═════════════════════════════════════════════════════════m-m══  |" << END_LOG;
    LOG_USAGE << "|        This software is distributed under GPL 2.0 license        |" << END_LOG;
    LOG_USAGE << "|          (GPL stands for GNU General Public License)             |" << END_LOG;
    LOG_USAGE << "|          The product comes with ABSOLUTELY NO WARRANTY.          |" << END_LOG;
    LOG_USAGE << "|   This is a free software, you are welcome to redistribute it.   |" << END_LOG;
    LOG_USAGE << "|                     Running in " << sizeof (uint64_t) * 8 << " bit mode!                      |" << END_LOG;
    LOG_USAGE << " ------------------------------------------------------------------ " << END_LOG;
}

/**
 * This function prints the usage information for the software
 * @param name the absolute name of the application 
 */
static void printUsage(const string name) {
    //Since we do not want the absolute file name, we need to chop off the directory prefix
    const unsigned int lastSlashBeforeFileName = name.find_last_of(PATH_SEPARATION_SYMBOLS);
    const string shortName = name.substr(lastSlashBeforeFileName + 1);

    LOG_USAGE << "Running: " << END_LOG;
    LOG_USAGE << "  " << shortName.c_str() << " <model_file> <test_file> [debug-level]" << END_LOG;
    LOG_USAGE << "      <model_file> - a text file containing the back-off language model." << END_LOG;
    LOG_USAGE << "                     This file is supposed to be in ARPA format, see: " << END_LOG;
    LOG_USAGE << "                          http://www.speech.sri.com/projects/srilm/manpages/ngram-format.5.html" << END_LOG;
    LOG_USAGE << "                     for more details. We also allow doe tags listed here:" << END_LOG;
    LOG_USAGE << "                          https://msdn.microsoft.com/en-us/library/office/hh378460%28v=office.14%29.aspx" << END_LOG;
    LOG_USAGE << "      <test_file>  - a text file containing test data." << END_LOG;
    LOG_USAGE << "                     The test file consists of a number of N-grams," << END_LOG;
    LOG_USAGE << "                     where each line in the file consists of one N-gram." << END_LOG;
    LOG_USAGE << "     [debug-level] - the optional debug flag from " << Logger::getReportingLevels() << END_LOG;

    LOG_USAGE << "Output: " << END_LOG;
    LOG_USAGE << "    The program reads in the test lines from the <test_file>. " << END_LOG;
    LOG_USAGE << "    Each of these lines is a N-grams of the following form, e.g: " << END_LOG;
    LOG_USAGE << "       word1 word2 word3 word4 word5" << END_LOG;
    LOG_USAGE << "    For each of such N-grams the probability information is " << END_LOG;
    LOG_USAGE << "    computed, based on the data from the <model_file>. For" << END_LOG;
    LOG_USAGE << "    example, for a N-gram such as:" << END_LOG;
    LOG_USAGE << "       mortgages had lured borrowers and" << END_LOG;
    LOG_USAGE << "    the program may give the following output:" << END_LOG;
    LOG_USAGE << "        probability( mortgages had lured borrowers and ) = 0.00024" << END_LOG;
}

/**
 * This function tries to extract the 
 * @param argc the number of program arguments
 * @param argv the array of program arguments
 * @param params the structure that will be filled in with the parsed program arguments
 */
static void extractArguments(const uint argc, char const * const * const argv, TAppParams & params) {
    if (argc < EXPECTED_NUMBER_OF_ARGUMENTS) {
        stringstream msg;
        msg << "Incorrect number of arguments, expected >= " << EXPECTED_USER_NUMBER_OF_ARGUMENTS << ", got " << (argc - 1);
        throw Exception(msg.str());
    } else {
        params.modelFileName = argv[1];
        params.testFileName = argv[2];
        //Set the default reporting level information for the logger
        string errorLevelStr = RESULT_PARAM_VALUE;

        //The third argument should be a debug level, get it and try to set.
        if (argc > EXPECTED_NUMBER_OF_ARGUMENTS) {
            errorLevelStr = argv[3];
        }
        Logger::setReportingLevel(errorLevelStr);
    }
}

/**
 * THis method creates a string for the "file exists" information
 * @param fname the file name
 * @param isPresent true if the file is present
 * @return the resulting string to be print
 */
static const string getFileExistsString(string const & fname, bool isPresent) {
    string result = ((bool) isPresent ? "is present" : "is missing");
    return fname + " (" + result + ")";
}

/**
 * This function is meant to give the memory statistics information delta
 * @param action the monitored action
 * @param msStart the start memory usage statistics
 * @param msEnd the end memory usage statistics
 */
static void reportMemotyUsage(const char* action, TMemotyUsage msStart, TMemotyUsage msEnd) {
    LOG_USAGE << "Action: \'" << action << "\' memory increase:" << END_LOG;
    LOG_DEBUG << "memory before: vmsize=" << SSTR(msStart.vmsize) << " Kb, vmpeak="
            << SSTR(msStart.vmpeak) << " Kb, vmrss=" << SSTR(msStart.vmrss)
            << " Kb, vmhwm=" << SSTR(msStart.vmhwm) << " Kb" << END_LOG;
    LOG_DEBUG << "memory after: vmsize=" << SSTR(msEnd.vmsize) << " Kb, vmpeak="
            << SSTR(msEnd.vmpeak) << " Kb, vmrss=" << SSTR(msEnd.vmrss)
            << " Kb, vmhwm=" << SSTR(msEnd.vmhwm) << " Kb" << END_LOG;

    unsigned int vmsize = (msEnd.vmsize < msStart.vmsize) ? 0 : msEnd.vmsize - msStart.vmsize;
    unsigned int vmpeak = (msEnd.vmpeak < msStart.vmpeak) ? 0 : msEnd.vmpeak - msStart.vmpeak;
    unsigned int vmrss = (msEnd.vmrss < msStart.vmrss) ? 0 : msEnd.vmrss - msStart.vmrss;
    unsigned int vmhwm = (msEnd.vmhwm < msStart.vmhwm) ? 0 : msEnd.vmhwm - msStart.vmhwm;
    LOG_USAGE << "vmsize=" << SSTR(vmsize / BYTES_ONE_MB)
            << " Mb, vmpeak=" << SSTR(vmpeak / BYTES_ONE_MB)
            << " Mb, vmrss=" << SSTR(vmrss / BYTES_ONE_MB)
            << " Mb, vmhwm=" << SSTR(vmhwm / BYTES_ONE_MB) << " Mb" << END_LOG;

    LOG_INFO << "  vmsize - Virtual memory size; vmpeak - Peak virtual memory size" << END_LOG;
    LOG_INFO << "    Virtual memory size is how much virtual memory the process has in total (RAM+SWAP)" << END_LOG;
    LOG_INFO << "  vmrss  - Resident set size; vmhwm  - Peak resident set size" << END_LOG;
    LOG_INFO << "    Resident set size is how much memory this process currently has in main memory (RAM)" << END_LOG;
}

/**
 * This method is used to read from the corpus and initialize the Trie
 * @param fstr the file to read data from
 * @param trie the trie to put the data into
 */
template<TModelLevel N>
static void fillInTrie(MemoryMappedFileReader & fstr, ATrie<N> & trie) {
    //A trie container and the corps file stream are already instantiated and are given

    //A.1. Create the TrieBuilder and give the trie to it
    ARPATrieBuilder<N> builder(trie, fstr);

    //A.2. Build the trie
    builder.build();
}

/**
 * Allows to read and execute test queries from the given file on the given trie.
 * @param trie the given trie, filled in with some data
 * @param testFile the file containing the N-Gram (5-Gram queries)
 * @return the CPU seconds used to run the queries, without time needed to read the test file
 */
template<TModelLevel N>
static double readAndExecuteQueries(ATrie<N> & trie, ifstream &testFile) {
    //Declare time variables for CPU times in seconds
    double totalTime = 0.0, startTime = 0.0, endTime = 0.0;
    //Will store the read line (word1 word2 word3 word4 word5)
    string line;
    //Will store the N-gram [word1 word2 word3 word4 word5] corresponding to the line
    vector<string> ngram;
    //Will store the queue result for one N-Gram
    SProbResult result = {0,};

    //Read the test file line by line
    while (getline(testFile, line)) {
        //First get the complete N-gram
        buildNGram(line, N, TOKEN_DELIMITER_CHAR, ngram);

        LOG_DEBUG << line << ":" << END_LOG;

        //Second qury the Trie for the results
        startTime = StatisticsMonitor::getCPUTime();
        trie.queryNGram(ngram, result);
        endTime = StatisticsMonitor::getCPUTime();

        //Print the results:
        LOG_RESULT << "log_" << LOG_PROB_WEIGHT_BASE << "( Prob( '" << line << "' ) ) = " << result.prob << END_LOG;
        LOG_INFO << "Prob( '" << line << "' ) = " << pow(LOG_PROB_WEIGHT_BASE, result.prob) << END_LOG;
        LOG_RESULT << "CPU Time needed: " << (endTime - startTime) << " sec." << END_LOG;

        //update total time
        totalTime += (endTime - startTime);
    }

    return totalTime;
}

/**
 * This method will perform the main tasks of this application:
 * Read the text corpus and create a trie and then read the test
 * file and query the trie for frequencies.
 * @param params the runtime program parameters
 */
static void performTasks(const TAppParams& params) {
    //Declare time variables for CPU times in seconds
    double startTime, endTime;
    //Declare the statistics monitor and its data
    TMemotyUsage memStatStart = {}, memStatInterm = {};

    LOG_DEBUG << "Getting the memory statistics before opening the model file ..." << END_LOG;
    StatisticsMonitor::getMemoryStatistics(memStatStart);

    //Attempt to open the model file
    MemoryMappedFileReader modelFile(params.modelFileName.c_str());

    LOG_DEBUG << "Getting the memory statistics after opening the model file ..." << END_LOG;
    StatisticsMonitor::getMemoryStatistics(memStatInterm);

    LOG_DEBUG << "Reporting on the memory consumption" << END_LOG;
    reportMemotyUsage("Opening the Language Model file", memStatStart, memStatInterm);

    //Attempt to open the test file
    ifstream testFile(params.testFileName.c_str());

    //If the files could be opened then proceed with training and then testing
    if ((modelFile.is_open()) && (testFile.is_open())) {
        LOG_DEBUG << "Getting the memory statistics before creating the Trie ..." << END_LOG;
        StatisticsMonitor::getMemoryStatistics(memStatStart);

        //Create a trie and pass it to the algorithm method
        TFiveContextMultiHashMapTrie trie(
                __AHashMapTrie::UM_WORD_INDEX_MEMORY_FACTOR,
                __ContextMultiHashMapTrie::UM_O_GRAM_MEMORY_FACTOR,
                __ContextMultiHashMapTrie::UM_M_GRAM_MEMORY_FACTOR,
                __ContextMultiHashMapTrie::UM_N_GRAM_MEMORY_FACTOR);

        LOG_DEBUG << "Getting the time statistics before creating the Trie ..." << END_LOG;
        startTime = StatisticsMonitor::getCPUTime();
        fillInTrie(modelFile, trie);
        LOG_DEBUG << "Getting the time statistics after creating the Trie ..." << END_LOG;
        endTime = StatisticsMonitor::getCPUTime();
        LOG_USAGE << "Reading the Language Model is done, it took " << (endTime - startTime) << " CPU seconds." << END_LOG;

        LOG_DEBUG << "Getting the memory statistics after creating the Trie ..." << END_LOG;
        StatisticsMonitor::getMemoryStatistics(memStatInterm);

        LOG_DEBUG << "Closing the model file ..." << END_LOG;
        modelFile.close();

        LOG_DEBUG << "Reporting on the memory consumption" << END_LOG;
        reportMemotyUsage("Loading of the Language Model", memStatStart, memStatInterm);

        LOG_USAGE << "Reading and executing the test queries ..." << END_LOG;
        const double queryCPUTimes = readAndExecuteQueries(trie, testFile);
        LOG_USAGE << "Total query execution time is " << queryCPUTimes << " CPU seconds." << END_LOG;
        testFile.close();
    } else {
        stringstream msg;
        msg << "One of the input files does not exist: " +
                getFileExistsString(params.modelFileName, (bool)modelFile)
                + " , " +
                getFileExistsString(params.testFileName, (bool)testFile);
        throw Exception(msg.str());
    }

    LOG_INFO << "Done" << END_LOG;
}

/**
 * The main program entry point
 */
int main(int argc, char** argv) {
    //Declare the return code
    int returnCode = 0;

    //First print the program info
    printInfo();

    try {
        LOG_INFO << "Checking on the program arguments ..." << END_LOG;
        //Attempt to extract the program arguments
        TAppParams params = {};
        extractArguments(argc, argv, params);

        LOG_INFO << "Checking on the provided files \'"
                << params.modelFileName << "\' and \'"
                << params.testFileName << "\' ..." << END_LOG;

        //Do the actual work, read the text corpse, create trie and do queries
        performTasks(params);

    } catch (Exception & ex) {
        //The argument's extraction has failed, print the error message and quit
        LOG_ERROR << ex.getMessage() << END_LOG;
        printUsage((string) argv[0]);
        returnCode = 1;
    }

    return returnCode;
}
