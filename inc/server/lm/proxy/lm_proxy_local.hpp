/* 
 * File:   lm_proxy_local.hpp
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

#include "server/lm/lm_consts.hpp"

#include "server/lm/proxy/lm_query_proxy.hpp"
#include "server/lm/proxy/lm_query_proxy_local.hpp"

#include "server/lm/builders/arpa_trie_builder.hpp"

#include "server/lm/dictionaries/BasicWordIndex.hpp"
#include "server/lm/dictionaries/CountingWordIndex.hpp"
#include "server/lm/dictionaries/OptimizingWordIndex.hpp"
#include "server/lm/dictionaries/HashingWordIndex.hpp"

#include "server/lm/models/c2d_map_trie.hpp"
#include "server/lm/models/w2c_hybrid_trie.hpp"
#include "server/lm/models/c2w_array_trie.hpp"
#include "server/lm/models/w2c_array_trie.hpp"
#include "server/lm/models/c2d_hybrid_trie.hpp"
#include "server/lm/models/g2d_map_trie.hpp"
#include "server/lm/models/h2d_map_trie.hpp"

using namespace uva::utils::monitore;
using namespace uva::utils::exceptions;
using namespace uva::utils::logging;
using namespace uva::utils::file;
using namespace uva::smt::bpbd::server::lm;
using namespace uva::smt::bpbd::server::lm::proxy;
using namespace uva::smt::bpbd::server::lm::arpa;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace lm {
                    namespace proxy {

                        /**
                         * This is a local trie proxy implementation of the trie proxy interface.
                         * Here we do not connect to remote server or something but rather work
                         * with a locally loaded trie model.
                         */
                        class lm_proxy_local : public lm_proxy {
                        public:
                            //Here we have a default word index, see the lm_confgs for the recommended word index information!
                            typedef HashingWordIndex word_index_type;
                            
                            //Here we have a default trie type
                            typedef H2DMapTrie<M_GRAM_LEVEL_MAX, word_index_type> model_type;
                            
                            //Define the builder type 
                            typedef lm_basic_builder<model_type, CStyleFileReader> builder_type;

                            /**
                             * The basic constructor of the trie proxy implementation class
                             * @param params the language model parameters
                             */
                            lm_proxy_local() : m_word_index(__AWordIndex::MEMORY_FACTOR), m_model(m_word_index) {
                            }

                            /**
                             * @see lm_proxy
                             */
                            virtual ~lm_proxy_local() {
                                //Call the disconnect, just in case.
                                disconnect();
                            };

                            /**
                             * @see lm_proxy
                             */
                            virtual void connect(const string & conn_str) {
                                //The whole purpose of this method connect here is
                                //just to load the language model into the memory.
                                load_model_data<builder_type, CStyleFileReader>("Language Model", conn_str);
                            }

                            /**
                             * @see lm_proxy
                             */
                            virtual void disconnect() {
                                //Nothing to be done at the moment, the word index
                                //and trie are stack allocated class data members 
                            }

                            /**
                             * @see lm_proxy
                             */
                            virtual lm_query_proxy * get_query_proxy() {
                                return new lm_query_proxy_local<model_type>(m_model);
                            }

                        private:

                            /**
                             * Allows to load the model into the instance of the selected container class
                             * @param the name of the model being loaded
                             * @params model_file_name the model file name
                             */
                            template<typename builder_type, typename file_reader_type>
                            void load_model_data(char const *model_name, const string & model_file_name) {
                                //Declare time variables for CPU times in seconds
                                double start_time, end_time;
                                //Declare the statistics monitor and its data
                                TMemotyUsage mem_stat_start = {}, mem_stat_end = {};
                                
                                LOG_USAGE << "--------------------------------------------------------" << END_LOG;
                                LOG_USAGE << "Start creating and loading the " << model_name << " ..." << END_LOG;

                                //ToDo: Add the possibility to choose between the file readers from the command line!
                                LOG_DEBUG << "Getting the memory statistics before opening the " << model_name << " file ..." << END_LOG;
                                StatisticsMonitor::getMemoryStatistics(mem_stat_start);

                                //Attempt to open the model file
                                file_reader_type model_file(model_file_name.c_str());
                                model_file.log_reader_type_info();
                                LOG_DEBUG << "Getting the memory statistics after opening the " << model_name << " file ..." << END_LOG;
                                StatisticsMonitor::getMemoryStatistics(mem_stat_end);

                                //Log the usage information
                                m_model.log_model_type_info();

                                LOG_DEBUG << "Getting the memory statistics before loading the " << model_name << " ..." << END_LOG;
                                StatisticsMonitor::getMemoryStatistics(mem_stat_start);
                                LOG_DEBUG << "Getting the time statistics before creating the " << model_name << " ..." << END_LOG;
                                start_time = StatisticsMonitor::getCPUTime();

                                //Assert that the model file is opened
                                ASSERT_CONDITION_THROW(!model_file.is_open(), string("The ") + string(model_name) + string(" file: '")
                                        + model_file_name + string("' does not exist!"));

                                //Create the trie builder and give it the trie
                                builder_type builder(m_model, model_file);
                                //Load the model from the file
                                builder.build();

                                LOG_DEBUG << "Getting the time statistics after loading the " << model_name << " ..." << END_LOG;
                                end_time = StatisticsMonitor::getCPUTime();
                                LOG_USAGE << "Reading the " << model_name << " took " << (end_time - start_time) << " CPU seconds." << END_LOG;
                                LOG_DEBUG << "Getting the memory statistics after loading the " << model_name << " ..." << END_LOG;
                                StatisticsMonitor::getMemoryStatistics(mem_stat_end);
                                LOG_DEBUG << "Reporting on the memory consumption" << END_LOG;
                                const string action_name = string("Loading the ") + string(model_name);
                                report_memory_usage(action_name.c_str(), mem_stat_start, mem_stat_end, true);

                                LOG_DEBUG << "Getting the memory statistics before closing the " << model_name << " file ..." << END_LOG;
                                StatisticsMonitor::getMemoryStatistics(mem_stat_start);
                                LOG_DEBUG << "Closing the model file ..." << END_LOG;
                                model_file.close();
                                LOG_DEBUG << "Getting the memory statistics after closing the " << model_name << " file ..." << END_LOG;
                                StatisticsMonitor::getMemoryStatistics(mem_stat_end);
                            }

                        protected:
                            //Stores the word index
                            word_index_type m_word_index;

                            //Stores the trie
                            model_type m_model;
                        };
                    }
                }
            }
        }
    }
}

#endif /* TRIE_PROXY_IMPL_HPP */

