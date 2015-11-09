/* 
 * File:   GenericTrieBase.hpp
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
 * Created on September 20, 2015, 5:21 PM
 */

#ifndef GENERICTRIEBASE_HPP
#define	GENERICTRIEBASE_HPP

#include <string>       // std::string

#include "Globals.hpp"
#include "Exceptions.hpp"
#include "Logger.hpp"

#include "MGrams.hpp"
#include "ModelMGram.hpp"
#include "QueryMGram.hpp"
#include "TextPieceReader.hpp"

#include "BasicWordIndex.hpp"
#include "CountingWordIndex.hpp"
#include "OptimizingWordIndex.hpp"

#include "WordIndexTrieBase.hpp"

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
             * Stores the possible result value for the method that retrieves the m-gram payload
             */
            enum GPR_Enum {
                FAILED_GPR = 0, PAYLOAD_GPR = FAILED_GPR + 1, BACK_OFF_GPR = PAYLOAD_GPR + 1
            };

            /**
             * This class defined the trie interface and functionality that is expected by the TrieDriver class
             */
            template<TModelLevel MAX_LEVEL, typename WordIndexType>
            class GenericTrieBase : public WordIndexTrieBase<MAX_LEVEL, WordIndexType> {
            public:
                //The offset, relative to the M-gram level M for the m-gram mapping array index
                const static TModelLevel MGRAM_IDX_OFFSET = 2;

                //Will store the the number of M levels such that 1 < M < N.
                const static TModelLevel NUM_M_GRAM_LEVELS = MAX_LEVEL - MGRAM_IDX_OFFSET;

                //Will store the the number of M levels such that 1 < M <= N.
                const static TModelLevel NUM_M_N_GRAM_LEVELS = MAX_LEVEL - 1;

                //Compute the N-gram index in in the arrays for M and N grams
                static const TModelLevel N_GRAM_IDX_IN_M_N_ARR = MAX_LEVEL - MGRAM_IDX_OFFSET;

                // Stores the undefined index array value
                static const TShortId UNDEFINED_ARR_IDX = 0;

                // Stores the undefined index array value
                static const TShortId FIRST_VALID_CTX_ID = UNDEFINED_ARR_IDX + 1;

                /**
                 * The basic constructor
                 * @param word_index the word index to be used
                 */
                explicit GenericTrieBase(WordIndexType & word_index) : WordIndexTrieBase<MAX_LEVEL, WordIndexType> (word_index) {
                }

                /**
                 * This method adds a 1-Gram (word) to the trie.
                 * It it snot guaranteed that the parameter will be checked to be a 1-Gram!
                 * @param gram the 1-Gram data
                 */
                inline void add_1_gram(const T_Model_M_Gram<WordIndexType> &gram) {
                    THROW_MUST_OVERRIDE();
                };

                /**
                 * This method adds a M-Gram (word) to the trie where 1 < M < N
                 * @param gram the M-Gram data
                 * @throws Exception if the level of this M-gram is not such that  1 < M < N
                 */
                template<TModelLevel CURR_LEVEL>
                inline void add_m_gram(const T_Model_M_Gram<WordIndexType> & gram) {
                    THROW_MUST_OVERRIDE();
                };

                /**
                 * This method adds a N-Gram (word) to the trie where
                 * It it snot guaranteed that the parameter will be checked to be a N-Gram!
                 * @param gram the N-Gram data
                 */
                inline void add_n_gram(const T_Model_M_Gram<WordIndexType> & gram) {
                    THROW_MUST_OVERRIDE();
                };

                /**
                 * Allows to log the information about the instantiated trie type
                 */
                inline void log_trie_type_usage_info() const {
                    THROW_MUST_OVERRIDE();
                };

                /**
                 * This method allows to get the probability and/or back off weight for the
                 * sub-m-gram defined by the BEGIN_WORD_IDX and END_WORD_IDX template parameters.
                 * @param BEGIN_WORD_IDX the begin word index in the given m-gram
                 * @param END_WORD_IDX the end word index in the given m-gram
                 * @param DO_BACK_OFF true if we should try to return the back-off m-gram payload
                 *                    in case the regular payload could not be retrieved
                 * @param gram the m-gram to work with
                 * @param payload the payload structure to put the values in
                 * @param bo_payload the reference to the back-off m-gram payload data structure
                 * @return GPR_Enum::PAYLOAD_GPR  if the payload has been found
                 *         GPR_Enum::BACK_OFF_GPR if the payload could not be found
                 *                                but the back-off payload was found
                 *         GPR_Enum::FAILED_GPR   if neither payload not the back-off
                 *                                payload was found
                 */
                template<TModelLevel BEGIN_WORD_IDX, TModelLevel END_WORD_IDX, bool DO_BACK_OFF>
                inline GPR_Enum get_payload(const T_Query_M_Gram<WordIndexType> & gram, T_M_Gram_Payload & payload, T_M_Gram_Payload & bo_payload) const {
                    THROW_MUST_OVERRIDE();
                };

                /**
                 * Allows to retrieve the probability and back-off weight of the unknown word
                 * @param payload the unknown word payload data
                 */
                inline void get_unk_word_payload(T_M_Gram_Payload & payload) const {
                    THROW_MUST_OVERRIDE();
                };

                /**
                 * Allows to check if the bitmap hash cache is to be used.
                 * By default returns false, needs to be re-implemented in the sub-class.
                 * @return true if the bitmap hash cache is to be used, otherwise false
                 */
                constexpr static inline bool needs_bitmap_hash_cache() {
                    return false;
                };

            };

            //Make sure that there will be templates instantiated, at least for the given parameter values
            template class GenericTrieBase<M_GRAM_LEVEL_MAX, BasicWordIndex >;
            template class GenericTrieBase<M_GRAM_LEVEL_MAX, CountingWordIndex>;
            template class GenericTrieBase<M_GRAM_LEVEL_MAX, OptimizingWordIndex<BasicWordIndex> >;
            template class GenericTrieBase<M_GRAM_LEVEL_MAX, OptimizingWordIndex<CountingWordIndex> >;

            //Make sure that there will be templates instantiated, at least for the given parameter values
#define INSTANTIATE_TRIE_GET_PAYLOAD_BEGIN_END_INDEX(BEGIN_IDX, END_IDX, TRIE_TYPE_NAME, ...) \
            template GPR_Enum TRIE_TYPE_NAME<__VA_ARGS__>::get_payload<BEGIN_IDX, END_IDX, true>(const T_Query_M_Gram<TRIE_TYPE_NAME<__VA_ARGS__>::WordIndexType> & gram, T_M_Gram_Payload & payload, T_M_Gram_Payload & bo_payload) const; \
            template GPR_Enum TRIE_TYPE_NAME<__VA_ARGS__>::get_payload<BEGIN_IDX, END_IDX, false>(const T_Query_M_Gram<TRIE_TYPE_NAME<__VA_ARGS__>::WordIndexType> & gram, T_M_Gram_Payload & payload, T_M_Gram_Payload & bo_payload) const;

#define INSTANTIATE_TRIE_GET_PAYLOAD(TRIE_TYPE_NAME, ...) \
            INSTANTIATE_TRIE_GET_PAYLOAD_BEGIN_END_INDEX(0, 6, TRIE_TYPE_NAME, __VA_ARGS__); \
            INSTANTIATE_TRIE_GET_PAYLOAD_BEGIN_END_INDEX(1, 6, TRIE_TYPE_NAME, __VA_ARGS__); \
            INSTANTIATE_TRIE_GET_PAYLOAD_BEGIN_END_INDEX(2, 6, TRIE_TYPE_NAME, __VA_ARGS__); \
            INSTANTIATE_TRIE_GET_PAYLOAD_BEGIN_END_INDEX(3, 6, TRIE_TYPE_NAME, __VA_ARGS__); \
            INSTANTIATE_TRIE_GET_PAYLOAD_BEGIN_END_INDEX(4, 6, TRIE_TYPE_NAME, __VA_ARGS__); \
            INSTANTIATE_TRIE_GET_PAYLOAD_BEGIN_END_INDEX(5, 6, TRIE_TYPE_NAME, __VA_ARGS__); \
            INSTANTIATE_TRIE_GET_PAYLOAD_BEGIN_END_INDEX(6, 6, TRIE_TYPE_NAME, __VA_ARGS__); \
            INSTANTIATE_TRIE_GET_PAYLOAD_BEGIN_END_INDEX(0, 5, TRIE_TYPE_NAME, __VA_ARGS__); \
            INSTANTIATE_TRIE_GET_PAYLOAD_BEGIN_END_INDEX(1, 5, TRIE_TYPE_NAME, __VA_ARGS__); \
            INSTANTIATE_TRIE_GET_PAYLOAD_BEGIN_END_INDEX(2, 5, TRIE_TYPE_NAME, __VA_ARGS__); \
            INSTANTIATE_TRIE_GET_PAYLOAD_BEGIN_END_INDEX(3, 5, TRIE_TYPE_NAME, __VA_ARGS__); \
            INSTANTIATE_TRIE_GET_PAYLOAD_BEGIN_END_INDEX(4, 5, TRIE_TYPE_NAME, __VA_ARGS__); \
            INSTANTIATE_TRIE_GET_PAYLOAD_BEGIN_END_INDEX(5, 5, TRIE_TYPE_NAME, __VA_ARGS__); \
            INSTANTIATE_TRIE_GET_PAYLOAD_BEGIN_END_INDEX(0, 4, TRIE_TYPE_NAME, __VA_ARGS__); \
            INSTANTIATE_TRIE_GET_PAYLOAD_BEGIN_END_INDEX(1, 4, TRIE_TYPE_NAME, __VA_ARGS__); \
            INSTANTIATE_TRIE_GET_PAYLOAD_BEGIN_END_INDEX(2, 4, TRIE_TYPE_NAME, __VA_ARGS__); \
            INSTANTIATE_TRIE_GET_PAYLOAD_BEGIN_END_INDEX(3, 4, TRIE_TYPE_NAME, __VA_ARGS__); \
            INSTANTIATE_TRIE_GET_PAYLOAD_BEGIN_END_INDEX(4, 4, TRIE_TYPE_NAME, __VA_ARGS__); \
            INSTANTIATE_TRIE_GET_PAYLOAD_BEGIN_END_INDEX(0, 3, TRIE_TYPE_NAME, __VA_ARGS__); \
            INSTANTIATE_TRIE_GET_PAYLOAD_BEGIN_END_INDEX(1, 3, TRIE_TYPE_NAME, __VA_ARGS__); \
            INSTANTIATE_TRIE_GET_PAYLOAD_BEGIN_END_INDEX(2, 3, TRIE_TYPE_NAME, __VA_ARGS__); \
            INSTANTIATE_TRIE_GET_PAYLOAD_BEGIN_END_INDEX(3, 3, TRIE_TYPE_NAME, __VA_ARGS__); \
            INSTANTIATE_TRIE_GET_PAYLOAD_BEGIN_END_INDEX(0, 2, TRIE_TYPE_NAME, __VA_ARGS__); \
            INSTANTIATE_TRIE_GET_PAYLOAD_BEGIN_END_INDEX(1, 2, TRIE_TYPE_NAME, __VA_ARGS__); \
            INSTANTIATE_TRIE_GET_PAYLOAD_BEGIN_END_INDEX(2, 2, TRIE_TYPE_NAME, __VA_ARGS__); \
            INSTANTIATE_TRIE_GET_PAYLOAD_BEGIN_END_INDEX(0, 1, TRIE_TYPE_NAME, __VA_ARGS__); \
            INSTANTIATE_TRIE_GET_PAYLOAD_BEGIN_END_INDEX(1, 1, TRIE_TYPE_NAME, __VA_ARGS__); \
            INSTANTIATE_TRIE_GET_PAYLOAD_BEGIN_END_INDEX(0, 0, TRIE_TYPE_NAME, __VA_ARGS__);

#define INSTANTIATE_TRIE_FUNCS_LEVEL(LEVEL, TRIE_TYPE_NAME, ...) \
            template void TRIE_TYPE_NAME<__VA_ARGS__>::add_m_gram<LEVEL>(const T_Model_M_Gram<TRIE_TYPE_NAME<__VA_ARGS__>::WordIndexType> & gram);

#define INSTANTIATE_TRIE_TEMPLATE_TYPE(TRIE_TYPE_NAME, ...) \
            template class TRIE_TYPE_NAME<__VA_ARGS__>; \
            INSTANTIATE_TRIE_GET_PAYLOAD(TRIE_TYPE_NAME, __VA_ARGS__) \
            INSTANTIATE_TRIE_FUNCS_LEVEL(M_GRAM_LEVEL_1, TRIE_TYPE_NAME, __VA_ARGS__); \
            INSTANTIATE_TRIE_FUNCS_LEVEL(M_GRAM_LEVEL_2, TRIE_TYPE_NAME, __VA_ARGS__); \
            INSTANTIATE_TRIE_FUNCS_LEVEL(M_GRAM_LEVEL_3, TRIE_TYPE_NAME, __VA_ARGS__); \
            INSTANTIATE_TRIE_FUNCS_LEVEL(M_GRAM_LEVEL_4, TRIE_TYPE_NAME, __VA_ARGS__); \
            INSTANTIATE_TRIE_FUNCS_LEVEL(M_GRAM_LEVEL_5, TRIE_TYPE_NAME, __VA_ARGS__); \
            INSTANTIATE_TRIE_FUNCS_LEVEL(M_GRAM_LEVEL_6, TRIE_TYPE_NAME, __VA_ARGS__); \
            INSTANTIATE_TRIE_FUNCS_LEVEL(M_GRAM_LEVEL_7, TRIE_TYPE_NAME, __VA_ARGS__);

        }
    }
}


#endif	/* GENERICTRIEBASE_HPP */

