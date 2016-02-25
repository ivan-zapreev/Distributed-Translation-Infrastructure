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

#include "server/lm/lm_consts.hpp"
#include "server/lm/lm_parameters.hpp"
#include "server/lm/lm_configurator.hpp"

#include "server/lm/proxy/lm_fast_query_proxy.hpp"

#include "server/lm/dictionaries/basic_word_index.hpp"
#include "server/lm/dictionaries/counting_word_index.hpp"
#include "server/lm/dictionaries/optimizing_word_index.hpp"
#include "server/lm/dictionaries/hashing_word_index.hpp"

#include "server/lm/builders/lm_basic_builder.hpp"
#include "server/lm/builders/lm_gram_builder.hpp"

#include "server/lm/models/m_gram_query.hpp"

using namespace std;
using namespace uva::utils::file;
using namespace uva::utils::logging;
using namespace uva::utils::exceptions;
using namespace uva::utils::monitore;

using namespace uva::smt::bpbd::server::lm;
using namespace uva::smt::bpbd::server::lm::dictionary;
using namespace uva::smt::bpbd::server::lm::arpa;
using namespace uva::smt::bpbd::server::lm::proxy;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace lm {
                    namespace __executor {

                        /**
                         * This structure is needed to store the language model (query application) parameters
                         */
                        typedef struct {
                            //Stores the language model specific parameters
                            lm_parameters m_lm_params;

                            //The test file name
                            string m_query_file_name;
                        } lm_exec_params;

                        /**
                         * Allows to read and execute test queries from the given file on the given trie.
                         * @param test_file the file containing the N-Gram (5-Gram queries)
                         */
                        template<typename TFileReaderQuery>
                        static void execute_queries(TFileReaderQuery &test_file) {
                            //Declare time variables for CPU times in seconds
                            double start_time = 0.0, end_time = 0.0;

                            //Will store the read line (word1 word2 word3 word4 word5)
                            TextPieceReader line;

                            //Get the query executor proxy object
                            lm_slow_query_proxy & query = lm_configurator::allocate_slow_query_proxy();

                            LOG_USAGE << "Start reading and executing the test queries ..." << END_LOG;

                            //Start the timer
                            start_time = StatisticsMonitor::getCPUTime();

                            //Read the test file line by line
                            while (test_file.get_first_line(line)) {
                                LOG_DEBUG << "Got query line [ " << line.str() << " ]" << END_LOG;

                                try {
                                    //Query the Trie for the results and log them
                                    query.execute(line);
                                } catch (Exception & ex) {
                                    //The query has failed! print an exception and proceed!
                                    LOG_ERROR << ex.get_message() << END_LOG;
                                }
                            }

                            //Stop the timer
                            end_time = StatisticsMonitor::getCPUTime();

                            //Dispose the query
                            lm_configurator::dispose_slow_query_proxy(query);

                            LOG_USAGE << "Total query execution time is " << (end_time - start_time) << " CPU seconds." << END_LOG;
                        }

                        /**
                         * This method will perform the main tasks of this application:
                         * Read the text corpus and create a trie and then read the test
                         * file and query the trie for frequencies.
                         * @param params the runtime program parameters
                         */
                        static void perform_tasks(const __executor::lm_exec_params & params) {
                            //Attempt to open the test file
                            MemoryMappedFileReader test_file(params.m_query_file_name.c_str());

                            //Assert that the query file is opened
                            ASSERT_CONDITION_THROW(!test_file.is_open(), string("The Test Queries file: '")
                                    + params.m_query_file_name + string("' does not exist!"));

                            //Connect to the language model
                            lm_configurator::connect(params.m_lm_params);

                            //Execute the queries
                            execute_queries(test_file);

                            //Deallocate the trie
                            LOG_USAGE << "Cleaning up memory ..." << END_LOG;

                            //Close the test file
                            test_file.close();

                            //Disconnect from the trie
                            lm_configurator::disconnect();
                        }
                    }
                }
            }
        }
    }
}

#endif /* QUERYEXECUTOR_HPP */

