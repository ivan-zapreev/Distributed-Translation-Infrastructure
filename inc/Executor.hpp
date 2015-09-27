/* 
 * File:   TrieExecutor.hpp
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
 * Created on September 18, 2015, 7:10 PM
 */

#ifndef EXECUTOR_HPP
#define	EXECUTOR_HPP
#include <string>

#include "Globals.hpp"
#include "Logger.hpp"
#include "Exceptions.hpp"

#include "GenericTrieDriver.hpp"
#include "BasicWordIndex.hpp"
#include "CountingWordIndex.hpp"
#include "OptimizingWordIndex.hpp"

#include "ARPATrieBuilder.hpp"
#include "ARPAGramBuilder.hpp"

#include "C2DHashMapTrie.hpp"
#include "W2CHybridMemoryTrie.hpp"
#include "C2WOrderedArrayTrie.hpp"
#include "W2COrderedArrayTrie.hpp"
#include "C2DMapArrayTrie.hpp"
#include "G2DHashMapTrie.hpp"

using namespace std;
using namespace uva::smt::file;
using namespace uva::smt::logging;
using namespace uva::smt::exceptions;
using namespace uva::smt::tries::dictionary;
using namespace uva::smt::tries::arpa;
using namespace uva::smt::monitore;

namespace uva {
    namespace smt {
        namespace tries {

            namespace __Executor {

                //Initialize constants
                static const string TC2DMapTrie_STR = string("c2dm");
                static const string TW2CHybridTrie_STR = string("w2ch");
                static const string TC2WArrayTrie_STR = string("c2wa");
                static const string TW2CArrayTrie_STR = string("w2ca");
                static const string C2DHybridTrie_STR = string("c2dh");
                static const string G2DMapTrie_STR = string("g2dm");

                /**
                 * Allows to get a string with all available (known to the factory) trie types
                 * @return the string with trie types, to be use from command line
                 */
                static inline string get_trie_types_str() {
                    stringstream text;
                    text << "{" << TC2DMapTrie_STR << ", "
                            << TW2CHybridTrie_STR << ", "
                            << TC2WArrayTrie_STR << ", "
                            << TW2CArrayTrie_STR << ", "
                            << C2DHybridTrie_STR << ", "
                            << G2DMapTrie_STR << "}";
                    return text.str();
                }

                /**
                 * This structure is needed to store the application parameters
                 */
                typedef struct {
                    //The train file name
                    string m_model_file_name;
                    //The test file name
                    string m_queries_file_name;
                    //The Trie type name
                    string m_trie_type_name;
                    //The word index type to be used with the trie
                    WordIndexTypesEnum m_word_index_type;
                    //Stores the word index memory factor
                    size_t m_word_index_mem_fact;
                    //The trie type 
                    TrieTypesEnum m_trie_type;
                } TExecutionParams;

                /**
                 * THis method creates a string for the "file exists" information
                 * @param fname the file name
                 * @param isPresent true if the file is present
                 * @return the resulting string to be print
                 */
                static const string get_file_exists_string(string const & fname, bool isPresent) {
                    string result = ((bool) isPresent ? "is present" : "is missing");
                    return fname + " (" + result + ")";
                }

                /**
                 * This function is meant to give the memory statistics information delta
                 * @param action the monitored action
                 * @param msStart the start memory usage statistics
                 * @param msEnd the end memory usage statistics
                 * @param isDoInfo true if the memory info may be print
                 */
                static void report_memory_usage(const char* action, TMemotyUsage msStart, TMemotyUsage msEnd, const bool isDoInfo) {
                    LOG_USAGE << "Action: \'" << action << "\' memory change:" << END_LOG;
                    LOG_DEBUG << "\tmemory before: vmsize=" << SSTR(msStart.vmsize) << " Kb, vmpeak="
                            << SSTR(msStart.vmpeak) << " Kb, vmrss=" << SSTR(msStart.vmrss)
                            << " Kb, vmhwm=" << SSTR(msStart.vmhwm) << " Kb" << END_LOG;
                    LOG_DEBUG << "memory after: vmsize=" << SSTR(msEnd.vmsize) << " Kb, vmpeak="
                            << SSTR(msEnd.vmpeak) << " Kb, vmrss=" << SSTR(msEnd.vmrss)
                            << " Kb, vmhwm=" << SSTR(msEnd.vmhwm) << " Kb" << END_LOG;

                    int vmsize = ((msEnd.vmsize < msStart.vmsize) ? 0 : msEnd.vmsize - msStart.vmsize) / BYTES_ONE_MB;
                    int vmpeak = ((msEnd.vmpeak < msStart.vmpeak) ? 0 : msEnd.vmpeak - msStart.vmpeak) / BYTES_ONE_MB;
                    int vmrss = ((msEnd.vmrss < msStart.vmrss) ? 0 : msEnd.vmrss - msStart.vmrss) / BYTES_ONE_MB;
                    int vmhwm = ((msEnd.vmhwm < msStart.vmhwm) ? 0 : msEnd.vmhwm - msStart.vmhwm) / BYTES_ONE_MB;
                    LOG_USAGE << showpos << "vmsize=" << vmsize << " Mb, vmpeak=" << vmpeak
                            << " Mb, vmrss=" << vmrss << " Mb, vmhwm=" << vmhwm
                            << " Mb" << noshowpos << END_LOG;
                    if (isDoInfo) {
                        LOG_INFO << "  vmsize - Virtual memory size; vmpeak - Peak virtual memory size" << END_LOG;
                        LOG_INFO << "    Virtual memory size is how much virtual memory the process has in total (RAM+SWAP)" << END_LOG;
                        LOG_INFO << "  vmrss  - Resident set size; vmhwm  - Peak resident set size" << END_LOG;
                        LOG_INFO << "    Resident set size is how much memory this process currently has in main memory (RAM)" << END_LOG;
                    }
                }

                /**
                 * This method is used to read from the corpus and initialize the Trie
                 * @param fstr the file to read data from
                 * @param trie the trie to put the data into
                 */
                template<typename TrieType>
                static void fill_in_trie(AFileReader & fstr, TrieType & trie) {
                    //A trie container and the corps file stream are already instantiated and are given

                    //A.1. Create the TrieBuilder and give the trie to it
                    ARPATrieBuilder<TrieType> builder(trie, fstr);

                    LOG_INFO3 << "Collision detections are: "
                            << (DO_SANITY_CHECKS ? "ON" : "OFF")
                            << " !" << END_LOG;

                    //A.2. Build the trie
                    builder.build();
                }

                /**
                 * For the given N-gram allows to give the string of the object
                 * for which the probability is computed, e.g.:
                 * N-gram = "word1" -> result = "word1"
                 * N-gram = "word1 word2 word3" -> result = "word3 | word1  word2"
                 * @param ngram the n-gram to transform
                 * @return the resulting string
                 */
                template<typename WordIndexType>
                static string get_mgram_prob_str(const T_M_Gram<WordIndexType> & mgram) {
                    if (mgram.m_used_level == 1) {
                        return mgram.m_tokens[0].str().empty() ? "<empty>" : mgram.m_tokens[0].str();
                    } else {
                        if (mgram.m_used_level > 1) {
                            string result = mgram.m_tokens[mgram.m_used_level - 1].str() + " |";
                            for (TModelLevel idx = 0; idx < (mgram.m_used_level - 1); idx++) {
                                result += string(" ") + mgram.m_tokens[idx].str();
                            }
                            return result;
                        } else {
                            return "<none>";
                        }
                    }
                }

                /**
                 * Allows to read and execute test queries from the given file on the given trie.
                 * @param trie the given trie, filled in with some data
                 * @param testFile the file containing the N-Gram (5-Gram queries)
                 * @return the CPU seconds used to run the queries, without time needed to read the test file
                 */
                template<typename TrieType>
                static void read_and_execute_queries(TrieType & trie, AFileReader &testFile) {
                    //Declare time variables for CPU times in seconds
                    double startTime = 0.0, endTime = 0.0;
                    //Will store the read line (word1 word2 word3 word4 word5)
                    TextPieceReader line;
                    //Will store the M-gram query and its internal state
                    MGramQuery <typename TrieType::WordIndexType > query(trie.get_word_index());

                    //Start the timer
                    startTime = StatisticsMonitor::getCPUTime();

                    //Read the test file line by line
                    while (testFile.getLine(line)) {
                        LOG_DEBUG << "Got query line [ " << line.str() << " ]" << END_LOG;

                        //Parse the line into an N-Gram
                        ARPAGramBuilder<typename TrieType::WordIndexType>::gram_to_tokens(line, query.m_gram);

                        //There can be an empty or "unreadable" line in the text file, just skip it ...
                        if (query.m_gram.m_used_level > 0) {

                            //Query the Trie for the results
                            trie.execute(query);

                            //Print the results:
                            LOG_RESULT << "log_" << LOG_PROB_WEIGHT_BASE << "( Prob( " << get_mgram_prob_str(query.m_gram) << " ) ) = " << SSTR(query.m_result.m_prob) << END_LOG;
                            LOG_INFO << "Prob( " << get_mgram_prob_str(query.m_gram) << " ) = " << SSTR(pow(LOG_PROB_WEIGHT_BASE, query.m_result.m_prob)) << END_LOG;
                        }
                    }

                    //Stop the timer
                    endTime = StatisticsMonitor::getCPUTime();

                    LOG_USAGE << "Total query execution time is " << (endTime - startTime) << " CPU seconds." << END_LOG;
                }

                template<typename TrieType>
                void execute(const __Executor::TExecutionParams& params, AFileReader &modelFile, AFileReader &testFile) {
                    //Get the word index type and make an instance of the word index
                    typename TrieType::WordIndexType word_index(params.m_word_index_mem_fact);
                    //Make an instance of the trie
                    TrieType trie(word_index);
                    //Declare time variables for CPU times in seconds
                    double startTime, endTime;
                    //Declare the statistics monitor and its data
                    TMemotyUsage memStatStart = {}, memStatEnd = {};

                    //Log the usage information
                    trie.log_trie_type_usage_info();

                    LOG_USAGE << "Start creating and loading the Trie ..." << END_LOG;
                    LOG_DEBUG << "Getting the memory statistics before creating the Trie ..." << END_LOG;
                    StatisticsMonitor::getMemoryStatistics(memStatStart);
                    LOG_DEBUG << "Getting the time statistics before creating the Trie ..." << END_LOG;
                    startTime = StatisticsMonitor::getCPUTime();
                    fill_in_trie(modelFile, trie);
                    LOG_DEBUG << "Getting the time statistics after creating the Trie ..." << END_LOG;
                    endTime = StatisticsMonitor::getCPUTime();
                    LOG_USAGE << "Reading the Language Model took " << (endTime - startTime) << " CPU seconds." << END_LOG;
                    LOG_DEBUG << "Getting the memory statistics after creating the Trie ..." << END_LOG;
                    StatisticsMonitor::getMemoryStatistics(memStatEnd);
                    LOG_DEBUG << "Reporting on the memory consumption" << END_LOG;
                    report_memory_usage("Creating the Language Model Trie", memStatStart, memStatEnd, true);

                    LOG_DEBUG << "Getting the memory statistics before closing the Model file ..." << END_LOG;
                    StatisticsMonitor::getMemoryStatistics(memStatStart);
                    LOG_DEBUG << "Closing the model file ..." << END_LOG;
                    modelFile.close();
                    LOG_DEBUG << "Getting the memory statistics after closing the Model file ..." << END_LOG;
                    StatisticsMonitor::getMemoryStatistics(memStatEnd);
                    //LOG_DEBUG << "Reporting on the memory consumption" << END_LOG;
                    //report_memory_usage("Closing the Language Model file", memStatStart, memStatEnd, true);

                    LOG_USAGE << "Start reading and executing the test queries ..." << END_LOG;
                    read_and_execute_queries(trie, testFile);
                    testFile.close();

                    //Deallocate the trie
                    LOG_USAGE << "Cleaning up memory ..." << END_LOG;
                }

                template<typename WordIndexType>
                static void choose_trie_type_and_execute(const __Executor::TExecutionParams& params,
                        AFileReader &modelFile, AFileReader &testFile) {
                    switch (params.m_trie_type) {
                        case TrieTypesEnum::C2DH_TRIE:
                            execute < GenericTrieDriver<LayeredTrieDriver<C2DHybridTrie<M_GRAM_LEVEL_MAX, WordIndexType>>> >(params, modelFile, testFile);
                            break;
                        case TrieTypesEnum::C2DM_TRIE:
                            execute < GenericTrieDriver<LayeredTrieDriver<C2DMapTrie<M_GRAM_LEVEL_MAX, WordIndexType>>> >(params, modelFile, testFile);
                            break;
                        case TrieTypesEnum::C2WA_TRIE:
                            execute < GenericTrieDriver<LayeredTrieDriver<C2WArrayTrie<M_GRAM_LEVEL_MAX, WordIndexType>>> >(params, modelFile, testFile);
                            break;
                        case TrieTypesEnum::G2DM_TRIE:
                            execute < GenericTrieDriver<G2DMapTrie<M_GRAM_LEVEL_MAX, WordIndexType>> >(params, modelFile, testFile);
                            break;
                        case TrieTypesEnum::W2CA_TRIE:
                            execute < GenericTrieDriver<LayeredTrieDriver<W2CArrayTrie<M_GRAM_LEVEL_MAX, WordIndexType>>> >(params, modelFile, testFile);
                            break;
                        case TrieTypesEnum::W2CH_TRIE:
                            execute < GenericTrieDriver < LayeredTrieDriver<W2CHybridTrie<M_GRAM_LEVEL_MAX, WordIndexType>>> > (params, modelFile, testFile);
                            break;
                        default:
                            stringstream msg;
                            msg << "Unrecognized trie type: " << params.m_trie_type;
                            throw Exception(msg.str());
                    }
                }

                /**
                 * Allows to execute the trie tasks: create/fill/execute queries
                 * @param params the execution parameters
                 * @param modelFile the model file existing and opened, will be closed by this function
                 * @param testFile the model file existing and opened, will be closed by this function
                 */
                static void choose_word_index_and_execute(
                        __Executor::TExecutionParams& params,
                        AFileReader &modelFile, AFileReader &testFile) {
                    LOG_DEBUG << "Choosing the appropriate Word index type" << END_LOG;

                    //Chose the word index type and then the trie type
                    params.m_word_index_mem_fact = __HashMapWordIndex::MEMORY_FACTOR;
                    switch (params.m_word_index_type) {
                        case WordIndexTypesEnum::BASIC_WORD_INDEX:
                            choose_trie_type_and_execute<BasicWordIndex>(params, modelFile, testFile);
                            break;
                        case WordIndexTypesEnum::COUNTING_WORD_INDEX:
                            choose_trie_type_and_execute<CountingWordIndex>(params, modelFile, testFile);
                            break;
                        case WordIndexTypesEnum::OPTIMIZING_BASIC_WORD_INDEX:
                            choose_trie_type_and_execute<OptimizingWordIndex<BasicWordIndex> >(params, modelFile, testFile);
                            break;
                        case WordIndexTypesEnum::OPTIMIZING_COUNTING_WORD_INDEX:
                            choose_trie_type_and_execute<OptimizingWordIndex<CountingWordIndex> >(params, modelFile, testFile);
                            break;
                        default:
                            stringstream msg;
                            msg << "Unrecognized word index type: " << params.m_word_index_type;
                            throw Exception(msg.str());
                    }
                }

                /**
                 * Allows to specify the remaining parameters and then to construct and trie, fill it with data and execute queries
                 * @param params the run parameters
                 * @param modelFile the open model file, will be closed within this call stack
                 * @param testFile the open queries file, will be closed within this call stack
                 */
                static void choose_and_execute(__Executor::TExecutionParams& params,
                        AFileReader &modelFile, AFileReader &testFile) {
                    if (params.m_trie_type_name == TC2DMapTrie_STR) {
                        params.m_word_index_type = __C2DMapTrie::WORD_INDEX_TYPE;
                        params.m_trie_type = TrieTypesEnum::C2DM_TRIE;
                    } else {
                        if (params.m_trie_type_name == TW2CHybridTrie_STR) {
                            params.m_word_index_type = __W2CHybridTrie::WORD_INDEX_TYPE;
                            params.m_trie_type = TrieTypesEnum::W2CH_TRIE;
                        } else {
                            if (params.m_trie_type_name == TC2WArrayTrie_STR) {
                                params.m_word_index_type = __C2WArrayTrie::WORD_INDEX_TYPE;
                                params.m_trie_type = TrieTypesEnum::C2WA_TRIE;
                            } else {
                                if (params.m_trie_type_name == TW2CArrayTrie_STR) {
                                    params.m_word_index_type = __W2CArrayTrie::WORD_INDEX_TYPE;
                                    params.m_trie_type = TrieTypesEnum::W2CA_TRIE;
                                } else {
                                    if (params.m_trie_type_name == C2DHybridTrie_STR) {
                                        params.m_word_index_type = __C2DHybridTrie::WORD_INDEX_TYPE;
                                        params.m_trie_type = TrieTypesEnum::C2DH_TRIE;
                                    } else {
                                        if (params.m_trie_type_name == G2DMapTrie_STR) {
                                            params.m_word_index_type = __G2DMapTrie::WORD_INDEX_TYPE;
                                            params.m_trie_type = TrieTypesEnum::G2DM_TRIE;
                                        } else {
                                            stringstream msg;
                                            msg << "Unrecognized trie type: " + params.m_trie_type_name;
                                            throw Exception(msg.str());
                                        }
                                    }
                                }
                            }
                        }
                    }
                    //Choose the word index and trie types and do all the actions
                    choose_word_index_and_execute(params, modelFile, testFile);
                }

                /**
                 * This method will perform the main tasks of this application:
                 * Read the text corpus and create a trie and then read the test
                 * file and query the trie for frequencies.
                 * @param params the runtime program parameters
                 */
                static void perform_tasks(__Executor::TExecutionParams& params) {
                    //Declare the statistics monitor and its data
                    TMemotyUsage memStatStart = {}, memStatEnd = {};

                    //ToDo: Add the possibility to choose between the file readers from the command line!
                    LOG_DEBUG << "Getting the memory statistics before opening the model file ..." << END_LOG;
                    StatisticsMonitor::getMemoryStatistics(memStatStart);
                    //Attempt to open the model file
                    //MemoryMappedFileReader modelFile(params.modelFileName.c_str());
                    //FileStreamReader modelFile(params.modelFileName.c_str());
                    CStyleFileReader modelFile(params.m_model_file_name.c_str());
                    modelFile.log_reader_type_usage_info();
                    LOG_DEBUG << "Getting the memory statistics after opening the model file ..." << END_LOG;
                    StatisticsMonitor::getMemoryStatistics(memStatEnd);
                    //LOG_DEBUG << "Reporting on the memory consumption" << END_LOG;
                    //report_memory_usage("Opening the Language Model file", memStatStart, memStatEnd, false);

                    //ToDo: Add the possibility to choose between the file readers from the command line!
                    //Attempt to open the test file
                    //FileStreamReader testFile(params.testFileName.c_str());
                    CStyleFileReader testFile(params.m_queries_file_name.c_str());

                    //If the files could be opened then proceed with training and then testing
                    if ((modelFile.is_open()) && (testFile.is_open())) {
                        //Choose needed class types and execute the trie tasks: create/fill/execute queries
                        choose_and_execute(params, modelFile, testFile);
                    } else {
                        stringstream msg;
                        msg << "One of the input files does not exist: " +
                                get_file_exists_string(params.m_model_file_name, (bool)modelFile)
                                + " , " +
                                get_file_exists_string(params.m_queries_file_name, (bool)testFile);
                        throw Exception(msg.str());
                    }

                    LOG_INFO << "Done" << END_LOG;
                }

            }
        }
    }
}

#endif	/* QUERYEXECUTOR_HPP */

