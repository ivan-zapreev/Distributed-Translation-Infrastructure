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

#include "MGrams.hpp"
#include "TextPieceReader.hpp"

#include "MGramQuery.hpp"

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
             * This is a function type for the function that should be able to
             * provide a new (next) context id for a word id and a previous context.
             * 
             * WARNING: Must only be called for the M-gram level 1 < M <= N!
             * 
             * @param wordId the word id
             * @param ctxId the in/out parameter that is a context id, the input is the previous context id, the output is the next context id
             * @param level the M-gram level we are working with, must have 1 < M <= N or UNDEF_NGRAM_LEVEL!
             * @result true if the next context id could be computed, otherwise false
             * @throw nothign
             */
            typedef std::function<bool (const TShortId wordId, TLongId & ctxId, const TModelLevel level) > TGetCtxIdFunct;

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
                 * Allows to retrieve the data storage structure for the One gram with the given Id.
                 * If the storage structure does not exist, return a new one.
                 * @param wordId the One-gram id
                 * @return the reference to the storage structure
                 */
                inline TProbBackOffEntry & make_1_gram_data_ref(const TShortId wordId) {
                    THROW_MUST_OVERRIDE();
                };

                /**
                 * Allows to retrieve the data storage structure for the One gram with the given Id.
                 * If the storage structure does not exist, throws an exception.
                 * @param wordId the One-gram id
                 * @param ppData[out] the pointer to a pointer to the found data
                 * @return true if the element was found, otherwise false
                 * @throw nothing
                 */
                inline bool get_1_gram_data_ref(const TShortId wordId,
                        const TProbBackOffEntry ** ppData) const {
                    THROW_MUST_OVERRIDE();
                };

                /**
                 * Allows to retrieve the data storage structure for the M gram
                 * with the given M-gram level Id. M-gram context and last word Id.
                 * If the storage structure does not exist, return a new one.
                 * @param level the currently considered m-gram level
                 * @param wordId the id of the M-gram's last word
                 * @param ctxId the M-gram context (the M-gram's prefix) id
                 * @return the reference to the storage structure
                 */
                template<TModelLevel level>
                inline TProbBackOffEntry& make_m_gram_data_ref(const TShortId wordId, TLongId ctxId) {
                    THROW_MUST_OVERRIDE();
                };

                /**
                 * Allows to retrieve the data storage structure for the M gram
                 * with the given M-gram level Id. M-gram context and last word Id.
                 * If the storage structure does not exist, throws an exception.
                 * @param level the currently considered m-gram level
                 * @param wordId the id of the M-gram's last word
                 * @param ctxId the M-gram context (the M-gram's prefix) id
                 * @param ppData[out] the pointer to a pointer to the found data
                 * @return true if the element was found, otherwise false
                 * @throw nothing
                 */
                template<TModelLevel level>
                inline bool get_m_gram_data_ref(const TShortId wordId,
                        TLongId ctxId, const TProbBackOffEntry **ppData) const {
                    THROW_MUST_OVERRIDE();
                };

                /**
                 * Allows to retrieve the data storage structure for the N gram.
                 * Given the N-gram context and last word Id.
                 * If the storage structure does not exist, return a new one.
                 * @param wordId the id of the N-gram's last word
                 * @param ctxId the N-gram context (the N-gram's prefix) id
                 * @return the reference to the storage structure
                 */
                inline TLogProbBackOff& make_n_gram_data_ref(const TShortId wordId, const TLongId ctxId) {
                    THROW_MUST_OVERRIDE();
                };

                /**
                 * Allows to retrieve the probability value for the N gram defined by the end wordId and ctxId.
                 * @param wordId the id of the N-gram's last word
                 * @param ctxId the N-gram context (the N-gram's prefix) id
                 * @param ppData[out] the pointer to a pointer to the found data
                 * @return true if the probability was found, otherwise false
                 * @throw nothing
                 */
                inline bool get_n_gram_data_ref(const TShortId wordId, const TLongId ctxId,
                        TLogProbBackOff & prob) const {
                    THROW_MUST_OVERRIDE();
                };

                /**
                 * Allows to get the the new context id for the word and previous context id given the level
                 * @param level the currently considered m-gram level
                 * @param wordId the word id on this level
                 * @param ctxId the previous level context id
                 * @param level the level for which the context id is to be computed
                 * @return true if computation of the next context is succeeded
                 */
                template<TModelLevel level>
                inline bool get_ctx_id(const TShortId wordId, TLongId & ctxId) const {
                    THROW_MUST_OVERRIDE();
                }

            protected:

                /**
                 * Needs to become inaccessible from outside, as this method is only relevant for generic tries
                 */
                inline void get_prob_weight(MGramQuery<WordIndexType> & query) const {
                    THROW_MUST_OVERRIDE();
                };

                /**
                 * Needs to become inaccessible from outside, as this method is only relevant for generic tries
                 */
                inline void add_back_off_weight(MGramQuery<WordIndexType> & query) const {
                    THROW_MUST_OVERRIDE();
                };
            };

            //Make sure that there will be templates instantiated, at least for the given parameter values
            template class LayeredTrieBase<M_GRAM_LEVEL_MAX, BasicWordIndex >;
            template class LayeredTrieBase<M_GRAM_LEVEL_MAX, CountingWordIndex >;
            template class LayeredTrieBase<M_GRAM_LEVEL_MAX, TOptBasicWordIndex >;
            template class LayeredTrieBase<M_GRAM_LEVEL_MAX, TOptCountWordIndex >;

#define INSTANTIATE_LAYERED_TRIE_TEMPLATES_NAME_TYPE(CLASS_NAME, WORD_IDX_TYPE) \
            template class CLASS_NAME<M_GRAM_LEVEL_MAX, WORD_IDX_TYPE >; \
            template TProbBackOffEntry& CLASS_NAME<M_GRAM_LEVEL_MAX, WORD_IDX_TYPE >::make_m_gram_data_ref<M_GRAM_LEVEL_2>(const TShortId wordId, const TLongId ctxId); \
            template TProbBackOffEntry& CLASS_NAME<M_GRAM_LEVEL_MAX, WORD_IDX_TYPE >::make_m_gram_data_ref<M_GRAM_LEVEL_3>(const TShortId wordId, const TLongId ctxId); \
            template TProbBackOffEntry& CLASS_NAME<M_GRAM_LEVEL_MAX, WORD_IDX_TYPE >::make_m_gram_data_ref<M_GRAM_LEVEL_4>(const TShortId wordId, const TLongId ctxId); \
            template TProbBackOffEntry& CLASS_NAME<M_GRAM_LEVEL_MAX, WORD_IDX_TYPE >::make_m_gram_data_ref<M_GRAM_LEVEL_5>(const TShortId wordId, const TLongId ctxId); \
            template TProbBackOffEntry& CLASS_NAME<M_GRAM_LEVEL_MAX, WORD_IDX_TYPE >::make_m_gram_data_ref<M_GRAM_LEVEL_6>(const TShortId wordId, const TLongId ctxId); \
            template TProbBackOffEntry& CLASS_NAME<M_GRAM_LEVEL_MAX, WORD_IDX_TYPE >::make_m_gram_data_ref<M_GRAM_LEVEL_7>(const TShortId wordId, const TLongId ctxId); \
            template bool CLASS_NAME<M_GRAM_LEVEL_MAX, WORD_IDX_TYPE >::get_m_gram_data_ref<M_GRAM_LEVEL_2>(const TShortId wordId, TLongId ctxId, const TProbBackOffEntry **ppData) const; \
            template bool CLASS_NAME<M_GRAM_LEVEL_MAX, WORD_IDX_TYPE >::get_m_gram_data_ref<M_GRAM_LEVEL_3>(const TShortId wordId, TLongId ctxId, const TProbBackOffEntry **ppData) const; \
            template bool CLASS_NAME<M_GRAM_LEVEL_MAX, WORD_IDX_TYPE >::get_m_gram_data_ref<M_GRAM_LEVEL_4>(const TShortId wordId, TLongId ctxId, const TProbBackOffEntry **ppData) const; \
            template bool CLASS_NAME<M_GRAM_LEVEL_MAX, WORD_IDX_TYPE >::get_m_gram_data_ref<M_GRAM_LEVEL_5>(const TShortId wordId, TLongId ctxId, const TProbBackOffEntry **ppData) const; \
            template bool CLASS_NAME<M_GRAM_LEVEL_MAX, WORD_IDX_TYPE >::get_m_gram_data_ref<M_GRAM_LEVEL_6>(const TShortId wordId, TLongId ctxId, const TProbBackOffEntry **ppData) const; \
            template bool CLASS_NAME<M_GRAM_LEVEL_MAX, WORD_IDX_TYPE >::get_m_gram_data_ref<M_GRAM_LEVEL_7>(const TShortId wordId, TLongId ctxId, const TProbBackOffEntry **ppData) const; \
            template bool CLASS_NAME<M_GRAM_LEVEL_MAX, WORD_IDX_TYPE >::get_ctx_id<M_GRAM_LEVEL_1>(const TShortId wordId, TLongId & ctxId) const; \
            template bool CLASS_NAME<M_GRAM_LEVEL_MAX, WORD_IDX_TYPE >::get_ctx_id<M_GRAM_LEVEL_2>(const TShortId wordId, TLongId & ctxId) const; \
            template bool CLASS_NAME<M_GRAM_LEVEL_MAX, WORD_IDX_TYPE >::get_ctx_id<M_GRAM_LEVEL_3>(const TShortId wordId, TLongId & ctxId) const; \
            template bool CLASS_NAME<M_GRAM_LEVEL_MAX, WORD_IDX_TYPE >::get_ctx_id<M_GRAM_LEVEL_4>(const TShortId wordId, TLongId & ctxId) const; \
            template bool CLASS_NAME<M_GRAM_LEVEL_MAX, WORD_IDX_TYPE >::get_ctx_id<M_GRAM_LEVEL_5>(const TShortId wordId, TLongId & ctxId) const; \
            template bool CLASS_NAME<M_GRAM_LEVEL_MAX, WORD_IDX_TYPE >::get_ctx_id<M_GRAM_LEVEL_6>(const TShortId wordId, TLongId & ctxId) const; \
            template bool CLASS_NAME<M_GRAM_LEVEL_MAX, WORD_IDX_TYPE >::get_ctx_id<M_GRAM_LEVEL_7>(const TShortId wordId, TLongId & ctxId) const;

        }
    }
}

#endif	/* LAYEREDTRIEBASE_HPP */

