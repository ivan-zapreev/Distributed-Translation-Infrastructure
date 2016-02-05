/* 
 * File:   trie_proxy_impl.hpp
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
 * Created on February 5, 2016, 8:45 AM
 */

#ifndef TRIE_PROXY_IMPL_HPP
#define TRIE_PROXY_IMPL_HPP

#include "common/utils/logging/logger.hpp"
#include "common/utils/exceptions.hpp"
#include "common/utils/monitore/statistics_monitore.hpp"
#include "common/utils/file/cstyle_file_reader.hpp"

#include "server/lm/trie_constants.hpp"
#include "server/lm/lm_config_utils.hpp"

#include "server/lm/proxy/lm_query_proxy.hpp"
#include "server/lm/proxy/lm_query_proxy_local.hpp"

#include "server/lm/builders/arpa_trie_builder.hpp"

using namespace uva::utils::monitore;
using namespace uva::utils::exceptions;
using namespace uva::utils::logging;
using namespace uva::utils::file;
using namespace uva::smt::translation::server::lm;
using namespace uva::smt::translation::server::lm::proxy;
using namespace uva::smt::translation::server::lm::arpa;

namespace uva {
    namespace smt {
        namespace translation {
            namespace server {
                namespace lm {
                    namespace proxy {

                        /**
                         * This is a local trie proxy implementation of the trie proxy interface.
                         * Here we do not connect to remote server or something but rather work
                         * with a locally loaded trie model.
                         */
                        template<typename trie_type>
                        class trie_proxy_local : public trie_proxy {
                        public:
                            //Make a local declaration of the word index type for convenience
                            typedef typename trie_type::WordIndexType word_index_type;

                            /**
                             * The basic constructor of the trie proxy implementation class
                             * @param params the language model parameters
                             */
                            trie_proxy_local() : m_word_index(__AWordIndex::MEMORY_FACTOR), m_trie(m_word_index) {
                            }

                            /**
                             * @see trie_proxy
                             */
                            virtual ~trie_proxy_local() {
                                //Call the disconnect, just in case.
                                disconnect();
                            };

                            /**
                             * @see trie_proxy
                             */
                            virtual void connect(const lm_parameters & params) {
                                //The whole purpose of this method connect here is
                                //just to load the language model into the memory.
                                load_trie_data(params);
                            }

                            /**
                             * @see trie_proxy
                             */
                            virtual void disconnect() {
                                //Nothing to be done at the moment, the word index
                                //and trie are stack allocated class data members 
                            }

                            /**
                             * @see trie_proxy
                             */
                            virtual lm_query_proxy * get_query_executor() {
                                return new lm_query_proxy_local<trie_type>(m_trie);
                            }

                            /**
                             * @see trie_proxy
                             */
                            virtual void log_trie_type_usage_info() {
                                m_trie.log_trie_type_usage_info();
                            }
                        private:

                            /**
                             * Allows to load the trie model into the trie instance of the selected class with
                             * the given word index type
                             * @params params the parameters needed to load the language model
                             */
                            void load_trie_data(const lm_parameters & params) {
                                //Declare time variables for CPU times in seconds
                                double start_time, end_time;
                                //Declare the statistics monitor and its data
                                TMemotyUsage mem_stat_start = {}, mem_stat_end = {};

                                //ToDo: Add the possibility to choose between the file readers from the command line!
                                LOG_DEBUG << "Getting the memory statistics before opening the model file ..." << END_LOG;
                                StatisticsMonitor::getMemoryStatistics(mem_stat_start);

                                //Attempt to open the model file
                                CStyleFileReader model_file(params.m_model_file_name.c_str());
                                model_file.log_reader_type_usage_info();
                                LOG_DEBUG << "Getting the memory statistics after opening the model file ..." << END_LOG;
                                StatisticsMonitor::getMemoryStatistics(mem_stat_end);

                                //Log the usage information
                                m_trie.log_trie_type_usage_info();

                                LOG_USAGE << "Start creating and loading the Trie ..." << END_LOG;
                                LOG_DEBUG << "Getting the memory statistics before creating the Trie ..." << END_LOG;
                                StatisticsMonitor::getMemoryStatistics(mem_stat_start);
                                LOG_DEBUG << "Getting the time statistics before creating the Trie ..." << END_LOG;
                                start_time = StatisticsMonitor::getCPUTime();

                                //Assert that the model file is opened
                                ASSERT_CONDITION_THROW(!model_file.is_open(), string("The Language Model file: '")
                                        + params.m_model_file_name + string("' does not exist!"));

                                //Create the trie builder and give it the trie
                                ARPATrieBuilder<trie_type, CStyleFileReader> builder(m_trie, model_file);
                                LOG_INFO3 << "Collision detections are: " << (DO_SANITY_CHECKS ? "ON" : "OFF") << " !" << END_LOG;
                                //Build the trie, from the model file
                                builder.build();

                                LOG_DEBUG << "Getting the time statistics after creating the Trie ..." << END_LOG;
                                end_time = StatisticsMonitor::getCPUTime();
                                LOG_USAGE << "Reading the Language Model took " << (end_time - start_time) << " CPU seconds." << END_LOG;
                                LOG_DEBUG << "Getting the memory statistics after creating the Trie ..." << END_LOG;
                                StatisticsMonitor::getMemoryStatistics(mem_stat_end);
                                LOG_DEBUG << "Reporting on the memory consumption" << END_LOG;
                                __configurator::report_memory_usage("Creating the Language Model Trie", mem_stat_start, mem_stat_end, true);

                                LOG_DEBUG << "Getting the memory statistics before closing the Model file ..." << END_LOG;
                                StatisticsMonitor::getMemoryStatistics(mem_stat_start);
                                LOG_DEBUG << "Closing the model file ..." << END_LOG;
                                model_file.close();
                                LOG_DEBUG << "Getting the memory statistics after closing the Model file ..." << END_LOG;
                                StatisticsMonitor::getMemoryStatistics(mem_stat_end);
                            }

                        protected:
                            //Stores the word index
                            word_index_type m_word_index;

                            //Stores the trie
                            trie_type m_trie;
                        };
                    }
                }
            }
        }
    }
}

#endif /* TRIE_PROXY_IMPL_HPP */

