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
#include "common/utils/monitor/statistics_monitor.hpp"

#include "server/server_configs.hpp"

#include "server/lm/lm_configs.hpp"
#include "server/lm/lm_consts.hpp"

#include "server/lm/proxy/lm_fast_query_proxy.hpp"
#include "server/lm/proxy/lm_fast_query_proxy_local.hpp"

#include "server/lm/proxy/lm_slow_query_proxy.hpp"
#include "server/lm/proxy/lm_slow_query_proxy_local.hpp"

#include "server/lm/builders/lm_basic_builder.hpp"

using namespace uva::utils::monitor;
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

                            /**
                             * The basic constructor of the trie proxy implementation class
                             * @param params the language model parameters
                             */
                            lm_proxy_local() : m_word_index(__AWordIndex::MEMORY_FACTOR), m_model(m_word_index), m_params(NULL) {
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
                            virtual void connect(const lm_parameters & params) {
                                //Store the parameters
                                m_params = &params;

                                //The whole purpose of this method connect here is
                                //just to load the language model into the memory.
                                load_model_data<lm_builder_type, lm_model_reader>("Language Model", params);

                                //Retrieve the unknown word probability
                                get_unk_word_prob();

                                //Retrieve the begin tag uid value
                                get_tag_uid(BEGIN_SENTENCE_TAG_STR, m_begin_tag_uid);
                                LOG_DEBUG << BEGIN_SENTENCE_TAG_STR << " tag uid: " << m_begin_tag_uid << END_LOG;
                                
                                //Retrieve the end tag uid value
                                get_tag_uid(END_SENTENCE_TAG_STR, m_end_tag_uid);
                                LOG_DEBUG << END_SENTENCE_TAG_STR << " tag uid: " << m_end_tag_uid << END_LOG;
                            }

                            /**
                             * @see lm_proxy
                             */
                            virtual void disconnect() {
                                //Nothing to be done at the moment, the word index
                                //and trie are stack allocated class data members 
                            }

                            /**
                             * \todo {In the future we should just use a number of stack 
                             * allocated objects in order to reduce the new/delete overhead}
                             * @see lm_proxy
                             */
                            virtual lm_fast_query_proxy & allocate_fast_query_proxy() {
                                return *(new lm_fast_query_proxy_local<lm_model_type>(*m_params, m_model, m_unk_word_prob, m_begin_tag_uid, m_end_tag_uid));
                            }

                            /**
                             * \todo {In the future we should just use a number of stack
                             * allocated objects in order to reduce the new/delete overhead}
                             * @see lm_proxy
                             */
                            virtual void dispose_fast_query_proxy(lm_fast_query_proxy & query) {
                                delete &query;
                            }

                            /**
                             * \todo {In the future we should just use a number of stack
                             * allocated objects in order to reduce the new/delete overhead}
                             * @see lm_proxy
                             */
                            virtual lm_slow_query_proxy & allocate_slow_query_proxy() {
                                return *(new lm_slow_query_proxy_local<lm_model_type>(m_model));
                            }

                            /**
                             * \todo {In the future we should just use a number of stack
                             * allocated objects in order to reduce the new/delete overhead}
                             * @see lm_proxy
                             */
                            virtual void dispose_slow_query_proxy(lm_slow_query_proxy & query) {
                                delete &query;
                            }

                        private:

                            /**
                             * Allows to retrieve the unknown word LM probability from the model for the sake of caching
                             */
                            void get_unk_word_prob() {
                                m_unk_word_prob = m_model.get_unk_word_prob();
                            }

                            /**
                             * Allows to retrieve the sentence tag uid from the model.
                             */
                            void get_tag_uid(const string & tag_str, word_uid & uid) {
                                //Create the text piece reader
                                text_piece_reader tag(tag_str.c_str(), tag_str.length());

                                //Get the end sentence tag word id
                                uid = m_model.get_word_index().get_word_id(tag);

                                //Check that the tag word uid is found!
                                ASSERT_CONDITION_THROW((uid == UNKNOWN_WORD_ID),
                                        string("The sentence tag '") + tag_str +
                                        string("' word uid is not found!"));
                            }

                            /**
                             * Allows to load the model into the instance of the selected container class
                             * \todo Add the possibility to choose between the file readers from the command line!
                             * @param the name of the model being loaded
                             * @params params the model parameters
                             */
                            template<typename lm_builder_type, typename file_reader_type>
                            void load_model_data(char const *model_name, const lm_parameters & params) {
                                const string & model_file_name = params.m_conn_string;

                                //Declare time variables for CPU times in seconds
                                double start_time, end_time;
                                //Declare the statistics monitor and its data
                                TMemotyUsage mem_stat_start = {}, mem_stat_end = {};

                                LOG_USAGE << "--------------------------------------------------------" << END_LOG;
                                LOG_USAGE << "Start creating and loading the " << model_name << " ..." << END_LOG;
                                LOG_USAGE << model_name << " is located in: " << model_file_name << END_LOG;

                                LOG_DEBUG << "Getting the memory statistics before opening the " << model_name << " file ..." << END_LOG;
                                stat_monitor::get_mem_stat(mem_stat_start);

                                //Attempt to open the model file
                                file_reader_type model_file(model_file_name.c_str());
                                model_file.log_reader_type_info();
                                LOG_DEBUG << "Getting the memory statistics after opening the " << model_name << " file ..." << END_LOG;
                                stat_monitor::get_mem_stat(mem_stat_end);

                                //Log the usage information
                                m_model.log_model_type_info();

                                LOG_DEBUG << "Getting the memory statistics before loading the " << model_name << " ..." << END_LOG;
                                stat_monitor::get_mem_stat(mem_stat_start);
                                LOG_DEBUG << "Getting the time statistics before creating the " << model_name << " ..." << END_LOG;
                                start_time = stat_monitor::get_cpu_time();

                                //Assert that the model file is opened
                                ASSERT_CONDITION_THROW(!model_file.is_open(), string("The ") + string(model_name) + string(" file: '")
                                        + model_file_name + string("' does not exist!"));

                                //Create the trie builder and give it the trie
                                lm_builder_type builder(params, m_model, model_file);
                                //Load the model from the file
                                builder.build();

                                LOG_DEBUG << "Getting the time statistics after loading the " << model_name << " ..." << END_LOG;
                                end_time = stat_monitor::get_cpu_time();
                                LOG_USAGE << "Reading the " << model_name << " took " << (end_time - start_time) << " CPU seconds." << END_LOG;
                                LOG_DEBUG << "Getting the memory statistics after loading the " << model_name << " ..." << END_LOG;
                                stat_monitor::get_mem_stat(mem_stat_end);
                                LOG_DEBUG << "Reporting on the memory consumption" << END_LOG;
                                const string action_name = string("Loading the ") + string(model_name);
                                report_memory_usage(action_name.c_str(), mem_stat_start, mem_stat_end, true);

                                LOG_DEBUG << "Getting the memory statistics before closing the " << model_name << " file ..." << END_LOG;
                                stat_monitor::get_mem_stat(mem_stat_start);
                                LOG_DEBUG << "Closing the model file ..." << END_LOG;
                                model_file.close();
                                LOG_DEBUG << "Getting the memory statistics after closing the " << model_name << " file ..." << END_LOG;
                                stat_monitor::get_mem_stat(mem_stat_end);
                            }

                        protected:
                            //Stores the word index
                            lm_word_index m_word_index;

                            //Stores the trie
                            lm_model_type m_model;

                            //Stores the cached unknown word probability from LM
                            prob_weight m_unk_word_prob;

                            //Sore the begin and end sentence tag word uids as retrieved from the LM word index.
                            word_uid m_begin_tag_uid;
                            word_uid m_end_tag_uid;

                            //Stores the pointer to the configuration parameters
                            const lm_parameters * m_params;
                        };
                    }
                }
            }
        }
    }
}

#endif /* TRIE_PROXY_IMPL_HPP */

