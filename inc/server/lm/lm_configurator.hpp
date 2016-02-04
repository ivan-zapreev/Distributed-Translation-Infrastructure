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

#include "server/lm/lm_parameters.hpp"
#include "server/lm/trie_constants.hpp"

#include "common/utils/logging/logger.hpp"
#include "common/utils/exceptions.hpp"
#include "common/utils/monitore/statistics_monitore.hpp"

#include "server/lm/tries/C2DMapTrie.hpp"
#include "server/lm/tries/W2CHybridTrie.hpp"
#include "server/lm/tries/C2WArrayTrie.hpp"
#include "server/lm/tries/W2CArrayTrie.hpp"
#include "server/lm/tries/C2DHybridTrie.hpp"
#include "server/lm/tries/G2DMapTrie.hpp"
#include "server/lm/tries/H2DMapTrie.hpp"

using namespace uva::utils::monitore;
using namespace uva::utils::exceptions;
using namespace uva::utils::logging;

namespace uva {
    namespace smt {
        namespace translation {
            namespace server {
                namespace lm {

                    //Make a forward declaration of the the lm query executor
                    class lm_query_executor;

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
                        static void configure(const lm_parameters & params) {
                            m_params = params;

                            //Detect which trie configuration is to be used
                            detect_trie_identifier();

                            //Load the trie data from the model file
                            load_trie_data();
                        }

                        /**
                         * Allows to return an instance of the query executor,
                         * is to be destroyed by the client class.
                         * @return an instance of the query executor.
                         */
                        static lm_query_executor * get_query_executor() {
                            //ToDo: Implement return the query executor proxy class
                            return NULL;
                        }

                    protected:
                        
                        //static uint32_t compute_trie_id(const uint8_t max_m_gram_level, ){}

                        /**
                         * Allows to detect the requested trie type, including the needed the word index class
                         * In case the given combination is not supported an exception is thrown.
                         */
                        static void detect_trie_identifier() {
                            //Check that the trie maximum trie level is supported
                            ASSERT_CONDITION_THROW((m_params.m_max_trie_level != M_GRAM_LEVEL_MAX),
                                    string("Unsupported trie max level: ") + std::to_string(m_params.m_max_trie_level));
                            
                            //Perform a number of checks on the type of the word index
                            ASSERT_CONDITION_THROW(
                                    ((m_params.m_word_index_type <= WordIndexTypesEnum::UNDEFINED_WORD_INDEX) ||
                                    (m_params.m_word_index_type <= WordIndexTypesEnum::size_word_index)),
                                    string("Unsupported word index type: ") + std::to_string(m_params.m_word_index_type));
                            ASSERT_CONDITION_THROW((m_params.m_word_index_type == WordIndexTypesEnum::BASIC_WORD_INDEX),
                                    string("The basic word index type is not supported"));
                            ASSERT_CONDITION_THROW((m_params.m_word_index_type == WordIndexTypesEnum::COUNTING_WORD_INDEX),
                                    string("The counting word index type is not supported"));
                            
                            //Check that the trie type is well defined
                            ASSERT_CONDITION_THROW(
                                    ((m_params.m_trie_type <= TrieTypesEnum::UNDEFINED_TRIE) ||
                                    (m_params.m_trie_type <= TrieTypesEnum::size_trie)),
                                    string("Unsupported trie type: ") + std::to_string(m_params.m_trie_type));

                            //Perform a check that the Hashing based trie is only used with the hashing index
                            ASSERT_CONDITION_THROW(
                                    ((m_params.m_word_index_type != WordIndexTypesEnum::COUNTING_WORD_INDEX) &&
                                    (m_params.m_trie_type == TrieTypesEnum::H2DM_TRIE)),
                                    string("The h2dm trie is only designed to work with the Hashing Word Index!"));
                            
                            //ToDo: based on the parameters detect which trie configuration is to be used
                        }

                        /**
                         * Allows to load the trie model into the trie instance of the selected class with
                         * the given word index type
                         */
                        static void load_trie_data() {
                            //ToDo: load the trie data from the model file
                        }

                    private:
                        //Stores the copy of the configuration parameters
                        static lm_parameters m_params;
                        //Stores the trie identifier type computed based on the lm parameters
                        static uint32_t m_trie_id;

                        //Make instances of all the currently supported word index types
                        static OptimizingWordIndex < BasicWordIndex > m_opt_basic_wi;
                        static OptimizingWordIndex < CountingWordIndex > m_opt_count_wi;
                        static HashingWordIndex m_hashing_wi;

                        //Make instances of all the currently supported trie combinations
                    };
                }
            }
        }
    }
}

#endif /* CONFIGURATOR_HPP */

