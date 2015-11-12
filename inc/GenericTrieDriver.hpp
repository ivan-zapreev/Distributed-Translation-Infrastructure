/* 
 * File:   ATrie.hpp
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
 * Created on September 3, 2015, 2:59 PM
 */

#ifndef GENERIC_TRIE_DRIVER_HPP
#define	GENERIC_TRIE_DRIVER_HPP

#include <string>       // std::string
#include <functional>   // std::function

#include "Globals.hpp"
#include "Exceptions.hpp"
#include "Logger.hpp"

#include "MathUtils.hpp"

#include "ModelMGram.hpp"
#include "TextPieceReader.hpp"

#include "BasicWordIndex.hpp"
#include "CountingWordIndex.hpp"
#include "OptimizingWordIndex.hpp"

#include "GenericTrieBase.hpp"

#include "C2DHashMapTrie.hpp"
#include "W2CHybridMemoryTrie.hpp"
#include "C2WOrderedArrayTrie.hpp"
#include "W2COrderedArrayTrie.hpp"
#include "C2DMapArrayTrie.hpp"
#include "G2DHashMapTrie.hpp"

using namespace std;
using namespace uva::smt::logging;
using namespace uva::smt::file;
using namespace uva::smt::tries::caching;
using namespace uva::smt::tries::dictionary;
using namespace uva::smt::tries::m_grams;
using namespace uva::utils::math::bits;

namespace uva {
    namespace smt {
        namespace tries {

            /**
             * This is the common generic trie base class.
             * @param N the maximum level of the considered N-gram, i.e. the N value
             * @param TrieType the type of word index to be used
             */
            template<typename TrieType>
            class GenericTrieDriver : public GenericTrieBase<TrieType::MAX_LEVEL, typename TrieType::WordIndexType, false> {
            public:
                static constexpr TModelLevel MAX_LEVEL = TrieType::MAX_LEVEL;
                typedef typename TrieType::WordIndexType WordIndexType;
                typedef GenericTrieBase<MAX_LEVEL, WordIndexType, false> BASE;

                /**
                 * The basic constructor
                 * @param word_index the word index to be used
                 */
                explicit GenericTrieDriver(WordIndexType & word_index)
                : GenericTrieBase<MAX_LEVEL, WordIndexType, false> (word_index),
                m_trie(word_index) {
                }

                /**
                 * @see GenericTrieBase
                 */
                void pre_allocate(const size_t counts[MAX_LEVEL]) {
                    m_trie.pre_allocate(counts);
                };

                /**
                 * @see GenericTrieBase
                 */
                template<TModelLevel CURR_LEVEL>
                inline void add_m_gram(const T_Model_M_Gram<WordIndexType> & gram) {
                    m_trie.template add_m_gram<CURR_LEVEL>(gram);
                };

                /**
                 * @see GenericTrieBase
                 */
                inline void log_trie_type_usage_info() {
                    m_trie.log_trie_type_usage_info();
                };

                /**
                 * @see GenericTrieBase
                 */
                template<TModelLevel CURR_LEVEL>
                inline bool is_post_grams() {
                    return m_trie.template is_post_grams<CURR_LEVEL>();
                };

                /**
                 * @see GenericTrieBase
                 */
                template<TModelLevel CURR_LEVEL>
                inline void post_grams() {
                    m_trie.template post_grams<CURR_LEVEL>();
                };

                /**
                 * @see GenericTrieBase
                 */
                template<bool DO_CUMULATIVE_PROBS>
                inline void execute(const T_Query_M_Gram<WordIndexType> & query, void * payloads[MAX_LEVEL][MAX_LEVEL], TLogProbBackOff probs[MAX_LEVEL]) const {
                    m_trie.template execute<DO_CUMULATIVE_PROBS>(query, payloads, probs);
                };

                /**
                 * The basic class destructor
                 */
                virtual ~GenericTrieDriver() {
                };

            protected:
                
                //Stores the trie
                TrieType m_trie;
            };

#define INSTANTIATE_TYPEDEF_TRIE_DRIVERS_TRIE_NAME_WORD_IDX_TYPE(PREFIX, TRIE_NAME, WORD_IDX_TYPE) \
            template class GenericTrieDriver<PREFIX##TRIE_NAME##WORD_IDX_TYPE>; \
            typedef GenericTrieDriver<PREFIX##TRIE_NAME##WORD_IDX_TYPE> TTrieDriver##TRIE_NAME##WORD_IDX_TYPE;

#define INSTANTIATE_TYPEDEF_TRIE_DRIVERS_PREFIX_NAME(PREFIX, TRIE_NAME) \
            INSTANTIATE_TYPEDEF_TRIE_DRIVERS_TRIE_NAME_WORD_IDX_TYPE(PREFIX, TRIE_NAME, Basic); \
            INSTANTIATE_TYPEDEF_TRIE_DRIVERS_TRIE_NAME_WORD_IDX_TYPE(PREFIX, TRIE_NAME, Count); \
            INSTANTIATE_TYPEDEF_TRIE_DRIVERS_TRIE_NAME_WORD_IDX_TYPE(PREFIX, TRIE_NAME, OptBasic); \
            INSTANTIATE_TYPEDEF_TRIE_DRIVERS_TRIE_NAME_WORD_IDX_TYPE(PREFIX, TRIE_NAME, OptCount);

#define INSTANTIATE_TYPEDEF_LAYERED_TRIE_DRIVERS_NAME(TRIE_NAME) \
INSTANTIATE_TYPEDEF_TRIE_DRIVERS_PREFIX_NAME( TLayeredTrieDriver, TRIE_NAME);         

#define INSTANTIATE_TYPEDEF_GENERIC_TRIE_DRIVERS_NAME(TRIE_NAME) \
INSTANTIATE_TYPEDEF_TRIE_DRIVERS_PREFIX_NAME( T, TRIE_NAME);         

            //Make sure that there will be templates instantiated, at least for the given parameter values
            INSTANTIATE_TYPEDEF_LAYERED_TRIE_DRIVERS_NAME(C2DMapTrie);
            INSTANTIATE_TYPEDEF_LAYERED_TRIE_DRIVERS_NAME(C2DHybridTrie);
            INSTANTIATE_TYPEDEF_LAYERED_TRIE_DRIVERS_NAME(C2WArrayTrie);
            INSTANTIATE_TYPEDEF_LAYERED_TRIE_DRIVERS_NAME(W2CHybridTrie);
            INSTANTIATE_TYPEDEF_LAYERED_TRIE_DRIVERS_NAME(W2CArrayTrie);
            INSTANTIATE_TYPEDEF_GENERIC_TRIE_DRIVERS_NAME(G2DMapTrie);
        }
    }
}

#endif	/* ATRIE_HPP */

