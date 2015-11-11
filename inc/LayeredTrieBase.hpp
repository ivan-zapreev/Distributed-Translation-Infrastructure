/* 
 * File:   LayeredTrieBase.hpp
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
 * Created on September 20, 2015, 5:39 PM
 */

#ifndef LAYEREDTRIEBASE_HPP
#define	LAYEREDTRIEBASE_HPP


#include <string>       // std::string

#include "Globals.hpp"
#include "Exceptions.hpp"
#include "Logger.hpp"

#include "TextPieceReader.hpp"

#include "BasicWordIndex.hpp"
#include "CountingWordIndex.hpp"
#include "OptimizingWordIndex.hpp"

#include "GenericTrieBase.hpp"

using namespace std;
using namespace uva::smt::logging;
using namespace uva::smt::file;
using namespace uva::smt::tries::dictionary;
using namespace uva::smt::tries::m_grams;
using namespace uva::utils::math::bits;

namespace uva {
    namespace smt {
        namespace tries {

            /**
             * This class defined the trie interface and functionality that is expected by the TrieDriver class
             */
            template<TModelLevel MAX_LEVEL, typename WordIndexType>
            class LayeredTrieBase : public GenericTrieBase<MAX_LEVEL, WordIndexType> {
            public:

                /**
                 * The basic constructor
                 * @param word_index the word index to be used
                 */
                explicit LayeredTrieBase(WordIndexType & word_index) : GenericTrieBase<MAX_LEVEL, WordIndexType> (word_index) {
                }

                /**
                 * Allows to retrieve the payload for the One gram with the given Id.
                 * @param word_id the One-gram id
                 * @param payload[out] the reference to the data that is to be set with the stored one.
                 * @throw nothing
                 */
                inline void get_1_gram_payload(const TShortId word_id,
                        T_M_Gram_Payload & payload) const {
                    THROW_MUST_OVERRIDE();
                };

                /**
                 * Allows to add the m-gram data under the given context.
                 * @param CURR_LEVEL the currently considered m-gram level
                 * @param gram the m-gram to be added
                 * @param ctx_id the M-gram context (the M-gram's prefix) id
                 */
                template<TModelLevel CURR_LEVEL>
                inline void add_m_gram_to_ctx(const T_Model_M_Gram<WordIndexType> & gram, TLongId ctx_id) {
                    THROW_MUST_OVERRIDE();
                };

                /**
                 * Allows to retrieve the payload for the M-gram defined by the end word_id and ctx_id.
                 * @param CURR_LEVEL the currently considered m-gram level
                 * @param word_id the id of the M-gram's last word
                 * @param ctx_id the M-gram context (the M-gram's prefix) id
                 * @param payload[out] the reference to the data that is to be set with the stored one.
                 * @return true if the element was found, otherwise false
                 * @throw nothing
                 */
                template<TModelLevel CURR_LEVEL>
                inline GPR_Enum get_m_gram_payload(const TShortId word_id, TLongId ctx_id,
                        T_M_Gram_Payload &payload) const {
                    THROW_MUST_OVERRIDE();
                };

                /**
                 * Allows to retrieve the payload for the N gram defined by the end word_id and ctx_id.
                 * @param word_id the id of the N-gram's last word
                 * @param ctx_id the N-gram context (the N-gram's prefix) id
                 * @param payload[out] the reference to the data that is to be set with the stored one.
                 * @return true if the probability was found, otherwise false
                 * @throw nothing
                 */
                inline GPR_Enum get_n_gram_payload(const TShortId word_id, const TLongId ctx_id,
                        T_M_Gram_Payload &payload) const {
                    THROW_MUST_OVERRIDE();
                };

                /**
                 * Allows to get the the new context id for the word and previous context id given the level
                 * @param CURR_LEVEL the currently considered m-gram level
                 * @param word_id the word id on this level
                 * @param ctx_id the previous level context id
                 * @return true if computation of the next context is succeeded
                 */
                template<TModelLevel CURR_LEVEL>
                inline bool get_ctx_id(const TShortId word_id, TLongId & ctx_id) const {
                    THROW_MUST_OVERRIDE();
                }
            };

            //Make sure that there will be templates instantiated, at least for the given parameter values
            template class LayeredTrieBase<M_GRAM_LEVEL_MAX, BasicWordIndex >;
            template class LayeredTrieBase<M_GRAM_LEVEL_MAX, CountingWordIndex >;
            template class LayeredTrieBase<M_GRAM_LEVEL_MAX, TOptBasicWordIndex >;
            template class LayeredTrieBase<M_GRAM_LEVEL_MAX, TOptCountWordIndex >;

#define INSTANTIATE_LAYERED_TRIE_TEMPLATES_NAME_TYPE(CLASS_NAME, WORD_IDX_TYPE) \
            template class CLASS_NAME<M_GRAM_LEVEL_MAX, WORD_IDX_TYPE >; \
            template void CLASS_NAME<M_GRAM_LEVEL_MAX, WORD_IDX_TYPE >::add_m_gram_to_ctx<M_GRAM_LEVEL_1>(const T_Model_M_Gram<WORD_IDX_TYPE> & gram, TLongId ctx_id); \
            template void CLASS_NAME<M_GRAM_LEVEL_MAX, WORD_IDX_TYPE >::add_m_gram_to_ctx<M_GRAM_LEVEL_2>(const T_Model_M_Gram<WORD_IDX_TYPE> & gram, TLongId ctx_id); \
            template void CLASS_NAME<M_GRAM_LEVEL_MAX, WORD_IDX_TYPE >::add_m_gram_to_ctx<M_GRAM_LEVEL_3>(const T_Model_M_Gram<WORD_IDX_TYPE> & gram, TLongId ctx_id); \
            template void CLASS_NAME<M_GRAM_LEVEL_MAX, WORD_IDX_TYPE >::add_m_gram_to_ctx<M_GRAM_LEVEL_4>(const T_Model_M_Gram<WORD_IDX_TYPE> & gram, TLongId ctx_id); \
            template void CLASS_NAME<M_GRAM_LEVEL_MAX, WORD_IDX_TYPE >::add_m_gram_to_ctx<M_GRAM_LEVEL_5>(const T_Model_M_Gram<WORD_IDX_TYPE> & gram, TLongId ctx_id); \
            template void CLASS_NAME<M_GRAM_LEVEL_MAX, WORD_IDX_TYPE >::add_m_gram_to_ctx<M_GRAM_LEVEL_6>(const T_Model_M_Gram<WORD_IDX_TYPE> & gram, TLongId ctx_id); \
            template void CLASS_NAME<M_GRAM_LEVEL_MAX, WORD_IDX_TYPE >::add_m_gram_to_ctx<M_GRAM_LEVEL_7>(const T_Model_M_Gram<WORD_IDX_TYPE> & gram, TLongId ctx_id); \
            template bool CLASS_NAME<M_GRAM_LEVEL_MAX, WORD_IDX_TYPE >::get_ctx_id<M_GRAM_LEVEL_1>(const TShortId word_id, TLongId & ctx_id) const; \
            template bool CLASS_NAME<M_GRAM_LEVEL_MAX, WORD_IDX_TYPE >::get_ctx_id<M_GRAM_LEVEL_2>(const TShortId word_id, TLongId & ctx_id) const; \
            template bool CLASS_NAME<M_GRAM_LEVEL_MAX, WORD_IDX_TYPE >::get_ctx_id<M_GRAM_LEVEL_3>(const TShortId word_id, TLongId & ctx_id) const; \
            template bool CLASS_NAME<M_GRAM_LEVEL_MAX, WORD_IDX_TYPE >::get_ctx_id<M_GRAM_LEVEL_4>(const TShortId word_id, TLongId & ctx_id) const; \
            template bool CLASS_NAME<M_GRAM_LEVEL_MAX, WORD_IDX_TYPE >::get_ctx_id<M_GRAM_LEVEL_5>(const TShortId word_id, TLongId & ctx_id) const; \
            template bool CLASS_NAME<M_GRAM_LEVEL_MAX, WORD_IDX_TYPE >::get_ctx_id<M_GRAM_LEVEL_6>(const TShortId word_id, TLongId & ctx_id) const; \
            template bool CLASS_NAME<M_GRAM_LEVEL_MAX, WORD_IDX_TYPE >::get_ctx_id<M_GRAM_LEVEL_7>(const TShortId word_id, TLongId & ctx_id) const;

        }
    }
}

#endif	/* LAYEREDTRIEBASE_HPP */

