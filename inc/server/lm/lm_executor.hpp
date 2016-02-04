/* 
 * File:   Executor.hpp
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

#ifndef LM_EXECUTOR_HPP
#define LM_EXECUTOR_HPP

#include <string>
#include <vector>

#include "common/utils/logging/logger.hpp"
#include "common/utils/exceptions.hpp"

#include "server/lm/lm_configurator.hpp"
#include "server/lm/lm_parameters.hpp"
#include "server/lm/trie_constants.hpp"

#include "server/lm/dictionaries/BasicWordIndex.hpp"
#include "server/lm/dictionaries/CountingWordIndex.hpp"
#include "server/lm/dictionaries/OptimizingWordIndex.hpp"
#include "server/lm/dictionaries/HashingWordIndex.hpp"

#include "server/lm/builders/ARPATrieBuilder.hpp"
#include "server/lm/builders/ARPAGramBuilder.hpp"

#include "server/lm/mgrams/QueryMGram.hpp"
#include "server/lm/tries/queries/MGramCumulativeQuery.hpp"
#include "server/lm/tries/queries/MGramSingleQuery.hpp"

using namespace std;
using namespace uva::utils::file;
using namespace uva::utils::logging;
using namespace uva::utils::exceptions;
using namespace uva::utils::monitore;
using namespace uva::smt::translation::server::lm::dictionary;
using namespace uva::smt::translation::server::lm::arpa;

namespace uva {
    namespace smt {
        namespace translation {
            namespace server {
                namespace lm {
                    namespace __executor {

                        /**
                         * This structure is needed to store the language model (query application) parameters
                         */
                        typedef struct {
                            //Stores the language model specific parameters
                            lm_parameters lm_params;
                            
                            //Stores true if the cumulative probability is to be computed
                            //for each M-gram, otherwise false, and then we only compute
                            //one conditional probability for this M-gram
                            bool is_cumulative_prob;
                            
                            //The test file name
                            string m_queries_file_name;
                        }lm_exec_params;

                        /**
                         * This method is used to read from the corpus and initialize the Trie
                         * @param fstr the file to read data from
                         * @param trie the trie to put the data into
                         */
                        template<typename TrieType, typename TFileReaderModel>
                        static void fill_in_trie(TFileReaderModel & fstr, TrieType & trie) {
                            //A trie container and the corps file stream are already instantiated and are given

                            //A.1. Create the TrieBuilder and give the trie to it
                            ARPATrieBuilder<TrieType, TFileReaderModel> builder(trie, fstr);

                            LOG_INFO3 << "Collision detections are: "
                                    << (DO_SANITY_CHECKS ? "ON" : "OFF")
                                    << " !" << END_LOG;

                            //A.2. Build the trie
                            builder.build();
                        }

                        /**
                         * Allows to read and execute test queries from the given file on the given trie.
                         * @param trie the given trie, filled in with some data
                         * @param testFile the file containing the N-Gram (5-Gram queries)
                         * @return the CPU seconds used to run the queries, without time needed to read the test file
                         */
                        template<typename TrieType, typename TrieQueryType, typename TFileReaderQuery>
                        static void read_and_execute_queries(TrieType & trie, TFileReaderQuery &testFile) {
                            //Declare time variables for CPU times in seconds
                            double startTime = 0.0, endTime = 0.0;
                            //Will store the read line (word1 word2 word3 word4 word5)
                            TextPieceReader line;
                            //Will store the M-gram query and its internal state
                            TrieQueryType query(trie);

                            //Start the timer
                            startTime = StatisticsMonitor::getCPUTime();

                            //Enable the next line for the pin-point debugging of the querying process
                            //Logger::get_reporting_level() = DebugLevelsEnum::DEBUG3;

                            //Read the test file line by line
                            while (testFile.get_first_line(line)) {
                                LOG_DEBUG << "Got query line [ " << line.str() << " ]" << END_LOG;

                                //Query the Trie for the results
                                query.execute(line);

                                //Print the results:
                                query.log_results();
                            }

                            //Stop the timer
                            endTime = StatisticsMonitor::getCPUTime();

                            LOG_USAGE << "Total query execution time is " << (endTime - startTime) << " CPU seconds." << END_LOG;
                        }

                        template<typename TrieType, bool IS_CUM_QUERY, typename TFileReaderModel, typename TFileReaderQuery>
                        void execute(const __executor::lm_exec_params& params, TFileReaderModel &modelFile, TFileReaderQuery &testFile) {
                            //Get the word index type and make an instance of the word index
                            typename TrieType::WordIndexType word_index(params.lm_params.m_word_index_mem_fact);
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
                            __configurator::report_memory_usage("Creating the Language Model Trie", memStatStart, memStatEnd, true);

                            LOG_DEBUG << "Getting the memory statistics before closing the Model file ..." << END_LOG;
                            StatisticsMonitor::getMemoryStatistics(memStatStart);
                            LOG_DEBUG << "Closing the model file ..." << END_LOG;
                            modelFile.close();
                            LOG_DEBUG << "Getting the memory statistics after closing the Model file ..." << END_LOG;
                            StatisticsMonitor::getMemoryStatistics(memStatEnd);

                            LOG_USAGE << "Start reading and executing the test queries ..." << END_LOG;
                            if (IS_CUM_QUERY) {
                                read_and_execute_queries<TrieType, T_M_Gram_Cumulative_Query < TrieType >> (trie, testFile);
                            } else {
                                read_and_execute_queries<TrieType, T_M_Gram_Single_Query < TrieType >> (trie, testFile);
                            }
                            testFile.close();

                            //Deallocate the trie
                            LOG_USAGE << "Cleaning up memory ..." << END_LOG;
                        }

                        template<typename WordIndexType, bool IS_CUM_QUERY, typename TFileReaderModel, typename TFileReaderQuery>
                        static void choose_trie_type_and_execute(const __executor::lm_exec_params& params,
                                TFileReaderModel &modelFile, TFileReaderQuery &testFile) {
                            switch (params.lm_params.m_trie_type) {
                                case TrieTypesEnum::C2DH_TRIE:
                                    execute < C2DHybridTrie<M_GRAM_LEVEL_MAX, WordIndexType>, IS_CUM_QUERY>(params, modelFile, testFile);
                                    break;
                                case TrieTypesEnum::C2DM_TRIE:
                                    execute < C2DMapTrie<M_GRAM_LEVEL_MAX, WordIndexType>, IS_CUM_QUERY>(params, modelFile, testFile);
                                    break;
                                case TrieTypesEnum::C2WA_TRIE:
                                    execute < C2WArrayTrie<M_GRAM_LEVEL_MAX, WordIndexType>, IS_CUM_QUERY>(params, modelFile, testFile);
                                    break;
                                case TrieTypesEnum::W2CA_TRIE:
                                    execute < W2CArrayTrie<M_GRAM_LEVEL_MAX, WordIndexType>, IS_CUM_QUERY>(params, modelFile, testFile);
                                    break;
                                case TrieTypesEnum::W2CH_TRIE:
                                    execute < W2CHybridTrie<M_GRAM_LEVEL_MAX, WordIndexType>, IS_CUM_QUERY> (params, modelFile, testFile);
                                    break;
                                case TrieTypesEnum::G2DM_TRIE:
                                    execute < G2DMapTrie<M_GRAM_LEVEL_MAX, WordIndexType>, IS_CUM_QUERY>(params, modelFile, testFile);
                                    break;
                                case TrieTypesEnum::H2DM_TRIE:
                                    execute < H2DMapTrie<M_GRAM_LEVEL_MAX, WordIndexType>, IS_CUM_QUERY>(params, modelFile, testFile);
                                    break;
                                default:
                                    THROW_EXCEPTION(string("Unrecognized trie type: ") + std::to_string(params.lm_params.m_trie_type));
                            }
                        }

                        /**
                         * Allows to execute the trie tasks: create/fill/execute queries
                         * @param params the execution parameters
                         * @param modelFile the model file existing and opened, will be closed by this function
                         * @param testFile the model file existing and opened, will be closed by this function
                         */
                        template<bool IS_CUM_QUERY, typename TFileReaderModel, typename TFileReaderQuery>
                        static void choose_word_index_and_execute(
                                __executor::lm_exec_params& params,
                                TFileReaderModel &modelFile, TFileReaderQuery &testFile) {
                            LOG_DEBUG << "Choosing the appropriate Word index type" << END_LOG;

                            //Chose the word index type and then the trie type
                            params.lm_params.m_word_index_mem_fact = __AWordIndex::MEMORY_FACTOR;
                            switch (params.lm_params.m_word_index_type) {
                                case WordIndexTypesEnum::BASIC_WORD_INDEX:
                                    choose_trie_type_and_execute<BasicWordIndex, IS_CUM_QUERY>(params, modelFile, testFile);
                                    break;
                                case WordIndexTypesEnum::COUNTING_WORD_INDEX:
                                    choose_trie_type_and_execute<CountingWordIndex, IS_CUM_QUERY>(params, modelFile, testFile);
                                    break;
                                case WordIndexTypesEnum::OPTIMIZING_BASIC_WORD_INDEX:
                                    choose_trie_type_and_execute<OptimizingWordIndex<BasicWordIndex>, IS_CUM_QUERY>(params, modelFile, testFile);
                                    break;
                                case WordIndexTypesEnum::OPTIMIZING_COUNTING_WORD_INDEX:
                                    choose_trie_type_and_execute<OptimizingWordIndex<CountingWordIndex>, IS_CUM_QUERY>(params, modelFile, testFile);
                                    break;
                                case WordIndexTypesEnum::HASHING_WORD_INDEX:
                                    choose_trie_type_and_execute<HashingWordIndex, IS_CUM_QUERY>(params, modelFile, testFile);
                                    break;
                                default:
                                    stringstream msg;
                                    msg << "Unrecognized word index type: " << params.lm_params.m_word_index_type;
                                    throw Exception(msg.str());
                            }
                        }

                        /**
                         * Allows to specify the remaining parameters and then to construct and trie, fill it with data and execute queries
                         * @param params the run parameters
                         * @param modelFile the open model file, will be closed within this call stack
                         * @param testFile the open queries file, will be closed within this call stack
                         */
                        template<typename TFileReaderModel, typename TFileReaderQuery>
                        static void choose_and_execute(__executor::lm_exec_params& params,
                                TFileReaderModel &modelFile, TFileReaderQuery &testFile) {
                            //First set the trie and word index type
                            __configurator::get_trie_and_word_index_types(params.lm_params);

                            //Instantiate the the proper templates and execute queries
                            if (params.is_cumulative_prob) {
                                choose_word_index_and_execute<true>(params, modelFile, testFile);
                            } else {
                                choose_word_index_and_execute<false>(params, modelFile, testFile);
                            }
                        }

                        /**
                         * This method will perform the main tasks of this application:
                         * Read the text corpus and create a trie and then read the test
                         * file and query the trie for frequencies.
                         * @param params the runtime program parameters
                         */
                        static void perform_tasks(__executor::lm_exec_params& params) {
                            //Declare the statistics monitor and its data
                            TMemotyUsage memStatStart = {}, memStatEnd = {};

                            //ToDo: Add the possibility to choose between the file readers from the command line!
                            LOG_DEBUG << "Getting the memory statistics before opening the model file ..." << END_LOG;
                            StatisticsMonitor::getMemoryStatistics(memStatStart);

                            //Attempt to open the model file
                            //ToDo: Add the possibility to choose between the file readers from the command line!
                            //MemoryMappedFileReader modelFile(params.m_model_file_name.c_str());
                            //FileStreamReader modelFile(params.m_model_file_name.c_str());
                            CStyleFileReader modelFile(params.lm_params.m_model_file_name.c_str());
                            modelFile.log_reader_type_usage_info();
                            LOG_DEBUG << "Getting the memory statistics after opening the model file ..." << END_LOG;
                            StatisticsMonitor::getMemoryStatistics(memStatEnd);
                            //LOG_DEBUG << "Reporting on the memory consumption" << END_LOG;
                            //report_memory_usage("Opening the Language Model file", memStatStart, memStatEnd, false);

                            //ToDo: Add the possibility to choose between the file readers from the command line!
                            //Attempt to open the test file
                            MemoryMappedFileReader testFile(params.m_queries_file_name.c_str());
                            //FileStreamReader testFile(params.m_queries_file_name.c_str());
                            //CStyleFileReader testFile(params.m_queries_file_name.c_str());

                            //If the files could be opened then proceed with training and then testing
                            if ((modelFile.is_open()) && (testFile.is_open())) {
                                //Choose needed class types and execute the trie tasks: create/fill/execute queries
                                choose_and_execute(params, modelFile, testFile);
                            } else {
                                stringstream msg;
                                msg << "One of the input files does not exist: " +
                                        modelFile.get_file_exists_string(params.lm_params.m_model_file_name)
                                        + " , " +
                                        testFile.get_file_exists_string(params.m_queries_file_name);
                                throw Exception(msg.str());
                            }

                            LOG_INFO << "Done" << END_LOG;
                        }

                    }
                }
            }
        }
    }
}

#endif /* QUERYEXECUTOR_HPP */

