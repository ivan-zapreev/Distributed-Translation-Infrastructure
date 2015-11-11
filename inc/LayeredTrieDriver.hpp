/* 
 * File:   LayeredTrieDriver.hpp
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
 * Created on April 18, 2015, 11:38 AM
 */

#ifndef ALAYEREDTRIE_HPP
#define	ALAYEREDTRIE_HPP

#include <string>       // std::string
#include <functional>   // std::function 

#include "Globals.hpp"
#include "Exceptions.hpp"

#include "TextPieceReader.hpp"
#include "AWordIndex.hpp"

#include "GenericTrieBase.hpp"
#include "LayeredTrieBase.hpp"

#include "G2DHashMapTrie.hpp"
#include "C2DHashMapTrie.hpp"
#include "W2CHybridMemoryTrie.hpp"
#include "C2WOrderedArrayTrie.hpp"
#include "W2COrderedArrayTrie.hpp"
#include "C2DMapArrayTrie.hpp"

using namespace std;
using namespace uva::smt::exceptions;
using namespace uva::smt::file;
using namespace uva::smt::hashing;
using namespace uva::smt::tries;
using namespace uva::smt::tries::dictionary;

namespace uva {
    namespace smt {
        namespace tries {

            /**
             * This is the common generic trie base class for layered tries.
             * @param N the maximum level of the considered N-gram, i.e. the N value
             * @param TrieType the type of word index to be used
             */
            template<typename TrieType >
            class LayeredTrieDriver : public GenericTrieBase<TrieType::MAX_LEVEL, typename TrieType::WordIndexType, false> {
            public:
                static constexpr TModelLevel MAX_LEVEL = TrieType::MAX_LEVEL;
                typedef typename TrieType::WordIndexType WordIndexType;
                typedef typename WordIndexType::TWordIdType TWordIdType;
                typedef GenericTrieBase<MAX_LEVEL, WordIndexType, false> BASE;
                //This is the function pointer type for the function that computes the m-gram context id
                typedef function<bool (const TrieType&, const TShortId, TLongId &) > TGetCtxIdFunct;

                /**
                 * The basic constructor
                 * @param _wordIndex the word index to be used
                 */
                explicit LayeredTrieDriver(WordIndexType & word_index)
                : GenericTrieBase<MAX_LEVEL, WordIndexType, false>(word_index), m_trie(word_index) {
                };

                /**
                 * @see GenericTrieBase
                 */
                inline void pre_allocate(const size_t counts[MAX_LEVEL]) {
                    m_trie.pre_allocate(counts);
                };

                /**
                 * @see GenericTrieBase
                 */
                template<TModelLevel CURR_LEVEL>
                inline void add_m_gram(const T_Model_M_Gram<WordIndexType> & gram) {
                    m_trie.template add_m_gram<CURR_LEVEL>(gram);
                }

                /**
                 * @see GenericTrieBase
                 */
                template<TModelLevel BEGIN_WORD_IDX, TModelLevel END_WORD_IDX, bool DO_BACK_OFF>
                GPR_Enum get_payload(const T_Query_M_Gram<WordIndexType> & gram, T_M_Gram_Payload & payload, T_M_Gram_Payload & bo_payload) const;

                /**
                 * @see GenericTrieBase
                 */
                inline void log_trie_type_usage_info() const {
                    m_trie.log_trie_type_usage_info();
                };

                /**
                 * @see GenericTrieBase
                 */
                template<TModelLevel CURR_LEVEL>
                bool is_post_grams() const {
                    return m_trie.template is_post_grams<CURR_LEVEL>();
                };
                
                /**
                 * @see GenericTrieBase
                 */
                template<TModelLevel BEGIN_WORD_IDX, TModelLevel END_WORD_IDX>
                inline bool is_m_gram_hash_cached(const T_Query_M_Gram<WordIndexType> & gram) const {
                    return m_trie.template is_m_gram_hash_cached<BEGIN_WORD_IDX, END_WORD_IDX>(gram);
                }
                
                /**
                 * @see GenericTrieBase
                 */
                template<TModelLevel CURR_LEVEL>
                inline void post_grams() {
                    m_trie.template post_grams<CURR_LEVEL>();
                };

                /**
                 * Allows to retrieve the probability and back-off weight of the unknown word
                 * @param payload the unknown word payload data
                 */
                inline void get_unk_word_payload(T_M_Gram_Payload & payload) const {
                    m_trie.get_unk_word_payload(payload);
                };

                /**
                 * The basic class destructor
                 */
                virtual ~LayeredTrieDriver() {
                };

            protected:

                /**
                 * The copy constructor, is made private as we do not intend to copy this class objects
                 * @param orig the object to copy from
                 */
                LayeredTrieDriver(const LayeredTrieDriver & orig)
                : GenericTrieBase<MAX_LEVEL, WordIndexType, false>(orig.get_word_index()),
                m_trie(orig.get_word_index()) {
                    throw Exception("ATrie copy constructor is not to be used, unless implemented!");
                };

            private:
                //Stores the trie
                TrieType m_trie;
            };

            template<typename TrieType>
            constexpr TModelLevel LayeredTrieDriver<TrieType>::MAX_LEVEL;

#define TYPEDEF_LAYERED_DRIVER_TEMPLATES_NAME_TYPE(TRIE_NAME, TYPE) \
            typedef LayeredTrieDriver< T##TRIE_NAME##TYPE > TLayeredTrieDriver##TRIE_NAME##TYPE;

#define TYPEDEF_LAYERED_DRIVER_TEMPLATES_NAME(TRIE_NAME) \
            TYPEDEF_LAYERED_DRIVER_TEMPLATES_NAME_TYPE(TRIE_NAME, Basic); \
            TYPEDEF_LAYERED_DRIVER_TEMPLATES_NAME_TYPE(TRIE_NAME, Count); \
            TYPEDEF_LAYERED_DRIVER_TEMPLATES_NAME_TYPE(TRIE_NAME, OptBasic); \
            TYPEDEF_LAYERED_DRIVER_TEMPLATES_NAME_TYPE(TRIE_NAME, OptCount);

            /**************************************************************************/
            TYPEDEF_LAYERED_DRIVER_TEMPLATES_NAME(C2DMapTrie);
            TYPEDEF_LAYERED_DRIVER_TEMPLATES_NAME(C2WArrayTrie);
            TYPEDEF_LAYERED_DRIVER_TEMPLATES_NAME(W2CArrayTrie);
            TYPEDEF_LAYERED_DRIVER_TEMPLATES_NAME(W2CHybridTrie);
            TYPEDEF_LAYERED_DRIVER_TEMPLATES_NAME(C2DHybridTrie);
            /**************************************************************************/
        }
    }
}
#endif	/* ITRIES_HPP */

