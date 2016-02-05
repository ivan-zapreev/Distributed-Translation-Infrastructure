/* 
 * File:   configurator.hpp
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
 * Created on February 4, 2016, 1:32 PM
 */

#ifndef LM_CONFIGURATOR_HPP
#define LM_CONFIGURATOR_HPP

#include "common/utils/logging/logger.hpp"
#include "common/utils/exceptions.hpp"

#include "server/lm/lm_parameters.hpp"

#include "server/lm/proxy/trie_proxy.hpp"
#include "server/lm/proxy/trie_proxy_local.hpp"
#include "server/lm/proxy/lm_query_proxy.hpp"

#include "server/lm/dictionaries/BasicWordIndex.hpp"
#include "server/lm/dictionaries/CountingWordIndex.hpp"
#include "server/lm/dictionaries/OptimizingWordIndex.hpp"
#include "server/lm/dictionaries/HashingWordIndex.hpp"

#include "server/lm/tries/C2DMapTrie.hpp"
#include "server/lm/tries/W2CHybridTrie.hpp"
#include "server/lm/tries/C2WArrayTrie.hpp"
#include "server/lm/tries/W2CArrayTrie.hpp"
#include "server/lm/tries/C2DHybridTrie.hpp"
#include "server/lm/tries/G2DMapTrie.hpp"
#include "server/lm/tries/H2DMapTrie.hpp"

using namespace uva::utils::exceptions;
using namespace uva::utils::logging;
using namespace uva::smt::translation::server::lm::proxy;

namespace uva {
    namespace smt {
        namespace translation {
            namespace server {
                namespace lm {

                    /**
                     * This class represents a singleton that allows to
                     * configure the language model and then issues.
                     * query proxy objects for performing the queries
                     * against the internally encapsulated language model(s).
                     */
                    class lm_configurator {
                    public:

                        /**
                         * This method allows to set the configuration parameters
                         * for the word index trie etc. This method is to be called
                         * only once! The latter is not checked but is a must.
                         * @param params the language model parameters to be set.
                         */
                        static void connect(const lm_parameters & params) {
                            //Store the parameters for future use
                            m_params = params;

                            //Allows to get the trie class proxy 
                            get_proxy__choose_word_index(m_params);

                            //Connect to the trie instance using the given parameters
                            m_trie_proxy->connect(m_params);
                        }

                        /**
                         * Allows to disconnect from the language model.
                         * Currently ony calls the destructor of the trie proxy object.
                         */
                        static void disconnect() {
                            if( m_trie_proxy != NULL) {
                                //Disconnect from the trie
                                m_trie_proxy->disconnect();
                                //Delete the object, free the resources
                                delete m_trie_proxy;
                                m_trie_proxy = NULL;
                            }
                        }

                        /**
                         * Allows to return an instance of the query executor,
                         * is to be destroyed by the client class.
                         * @return an instance of the query executor.
                         */
                        static lm_query_proxy * get_query_executor() {
                            //Return the query executor as given by the proxy class
                            return m_trie_proxy->get_query_executor();
                        }

                    protected:

                        /**
                         * Allows to the trie type.
                         * In the end this method call will ensure that a trie proxy is constructed.
                         * @param params the lm parameters reference
                         */
                        template<TModelLevel MAX_LEVEL, typename word_index_type>
                        static void get_proxy__choose_trie(const lm_parameters & params) {
                            switch (params.m_trie_type) {
                                case trie_types::C2DH_TRIE:
                                    m_trie_proxy = new trie_proxy_local < C2DHybridTrie<MAX_LEVEL, word_index_type >> ();
                                    break;
                                case trie_types::C2DM_TRIE:
                                    m_trie_proxy = new trie_proxy_local < C2DMapTrie<MAX_LEVEL, word_index_type >> ();
                                    break;
                                case trie_types::C2WA_TRIE:
                                    m_trie_proxy = new trie_proxy_local < C2WArrayTrie<MAX_LEVEL, word_index_type >> ();
                                    break;
                                case trie_types::W2CA_TRIE:
                                    m_trie_proxy = new trie_proxy_local < W2CArrayTrie<MAX_LEVEL, word_index_type >> ();
                                    break;
                                case trie_types::W2CH_TRIE:
                                    m_trie_proxy = new trie_proxy_local < W2CHybridTrie<MAX_LEVEL, word_index_type >> ();
                                    break;
                                case trie_types::G2DM_TRIE:
                                    m_trie_proxy = new trie_proxy_local < G2DMapTrie<MAX_LEVEL, word_index_type >> ();
                                    break;
                                case trie_types::H2DM_TRIE:
                                    m_trie_proxy = new trie_proxy_local < H2DMapTrie<MAX_LEVEL, word_index_type >> ();
                                    break;
                                default:
                                    THROW_EXCEPTION(string("Unrecognized trie type: ") + std::to_string(params.m_trie_type));
                            }
                        }

                        /**
                         * Allows to the maximum level and then the trie type and so forth.
                         * In the end this method call will ensure that a trie proxy is constructed.
                         * @param params the lm parameters reference
                         */
                        template<typename word_index_type>
                        static void get_proxy__choose_level(const lm_parameters & params) {
                            switch (params.m_max_trie_level) {
                                //ToDo: In order to allow for more m-gram levels we need to instantiate more templates in the Trie headers
                                //case M_GRAM_LEVEL_3:
                                //    get_proxy__choose_trie < M_GRAM_LEVEL_3, WordIndexType > (params);
                                //    break;
                                //case M_GRAM_LEVEL_4:
                                //    get_proxy__choose_trie < M_GRAM_LEVEL_4, WordIndexType > (params);
                                //    break;
                                case M_GRAM_LEVEL_5:
                                    get_proxy__choose_trie < M_GRAM_LEVEL_5, word_index_type > (params);
                                    break;
                                //case M_GRAM_LEVEL_6:
                                //    get_proxy__choose_trie < M_GRAM_LEVEL_6, WordIndexType > (params);
                                //    break;
                                default:
                                    THROW_EXCEPTION(string("Unsupported trie max level: ") + std::to_string(params.m_max_trie_level));
                            }
                        }

                        /**
                         * Allows to choose the word index type and then the maximum level and so forth.
                         * In the end this method call will ensure that a trie proxy is constructed.
                         * @param params the lm parameters reference
                         */
                        static void get_proxy__choose_word_index(const lm_parameters & params) {
                            LOG_DEBUG << "Choosing the appropriate Word index type" << END_LOG;

                            //Chose the word index type and then the trie type
                            switch (params.m_word_index_type) {
                                case word_index_types::BASIC_WORD_INDEX:
                                    get_proxy__choose_level<BasicWordIndex>(params);
                                    break;
                                case word_index_types::COUNTING_WORD_INDEX:
                                    get_proxy__choose_level<CountingWordIndex>(params);
                                    break;
                                case word_index_types::OPTIMIZING_BASIC_WORD_INDEX:
                                    get_proxy__choose_level<OptimizingWordIndex < BasicWordIndex >> (params);
                                    break;
                                case word_index_types::OPTIMIZING_COUNTING_WORD_INDEX:
                                    get_proxy__choose_level<OptimizingWordIndex < CountingWordIndex >> (params);
                                    break;
                                case word_index_types::HASHING_WORD_INDEX:
                                    get_proxy__choose_level<HashingWordIndex>(params);
                                    break;
                                default:
                                    THROW_EXCEPTION(string("Unrecognized word index type: ") + to_string(params.m_word_index_type));
                            }
                        }

                    private:
                        //Stores the copy of the configuration parameters
                        static lm_parameters m_params;
                        
                        //Store the trie proxy object
                        static trie_proxy * m_trie_proxy;
                    };
                }
            }
        }
    }
}

#endif /* CONFIGURATOR_HPP */

