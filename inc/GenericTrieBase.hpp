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

#include "ModelMGram.hpp"
#include "QueryMGram.hpp"
#include "TextPieceReader.hpp"

#include "BasicWordIndex.hpp"
#include "CountingWordIndex.hpp"
#include "OptimizingWordIndex.hpp"

#include "WordIndexTrieBase.hpp"

#include "BitmapHashCache.hpp"

using namespace std;
using namespace uva::smt::logging;
using namespace uva::smt::file;
using namespace uva::smt::tries::dictionary;
using namespace uva::smt::tries::m_grams;
using namespace uva::utils::math::bits;
using namespace uva::smt::tries::caching;

namespace uva {
    namespace smt {
        namespace tries {

            //This macro is needed to report the collision detection warnings!
#define REPORT_COLLISION_WARNING(gram, word_id, contextId, prevProb, prevBackOff, newProb, newBackOff)   \
            LOG_WARNING << "The " << gram.get_m_gram_level() << "-Gram : " << (string) gram              \
                        << " has been already seen! Word Id: " << SSTR(word_id)                          \
                        << ", context Id: " << SSTR(contextId) << ". "                                   \
                        << "Changing the (prob,back-off) data from ("                                    \
                        << prevProb << "," << prevBackOff << ") to ("                                    \
                        << newProb << "," << newBackOff << ")" << END_LOG;

            /**
             * Stores the possible result value for the method that retrieves the m-gram payload
             */
            enum GPR_Enum {
                FAILED_GPR = 0, PAYLOAD_GPR = FAILED_GPR + 1, BACK_OFF_GPR = PAYLOAD_GPR + 1
            };

            /**
             * Contains the m-gram status values:
             * 0. UNDEFINED_MGS - the status is undefined
             * 1. BAD_END_WORD_UNKNOWN_MGS - the m-gram is definitely not present the end word is unknown
             * 2. BAD_NO_PAYLOAD_MGS - the m-gram is definitely not present, the m-gram hash is not cached,
             *                         or it is not found in the trie (the meaning depends on the context)
             * 3. GOOD_PRESENT_MGS   - the m-gram is potentially present, its hash is cached, or it is
             *                         found in the trie (the meaning depends on the context)
             */
            enum MGramStatusEnum {
                UNDEFINED_MGS = 0, BAD_END_WORD_UNKNOWN_MGS = 1, BAD_NO_PAYLOAD_MGS = 2, GOOD_PRESENT_MGS = 3
            };

            /**
             * This class defined the trie interface and functionality that is expected by the TrieDriver class
             */
            template<TModelLevel MAX_LEVEL, typename WordIndexType, bool NEEDS_BITMAP_HASH_CACHE>
            class GenericTrieBase : public WordIndexTrieBase<MAX_LEVEL, WordIndexType> {
            public:
                //Typedef the base class
                typedef WordIndexTrieBase<MAX_LEVEL, WordIndexType> BASE;

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
                 * @see WordIndexTrieBase
                 */
                inline void pre_allocate(const size_t counts[MAX_LEVEL]) {
                    BASE::pre_allocate(counts);

                    //Pre-allocate the bitmap cache for hashes if needed
                    if (NEEDS_BITMAP_HASH_CACHE) {
                        for (size_t idx = 0; idx < NUM_M_N_GRAM_LEVELS; ++idx) {
                            m_bitmap_hash_cach[idx].pre_allocate(counts[idx + 1]);
                            Logger::updateProgressBar();
                        }
                    }
                }

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
                 * Allows to log the information about the instantiated trie type
                 */
                inline void log_trie_type_usage_info() const {
                    THROW_MUST_OVERRIDE();
                };

                /**
                 * This method allows to get the payloads and compute the (cumulative) m-gram probabilities.
                 * @param DO_CUMULATIVE_PROBS true if we want cumulative probabilities per sum-m-gram, otherwise false (one conditional m-gram probability)
                 * @param gram the m-gram to work with
                 * @param payloads the matrix of pointers to payloads, has to contain NULL pointers in the first place
                 * @param probs the computed probabilities, should be initialized with zeros.
                 */
                template<bool DO_CUMULATIVE_PROBS>
                inline void execute(const T_Query_M_Gram<WordIndexType> & query, const void * payloads[MAX_LEVEL][MAX_LEVEL], TLogProbBackOff probs[MAX_LEVEL]) const {
                    THROW_MUST_OVERRIDE();
                };

                /**
                 * Allows to check if the given sub-m-gram, defined by the begin_word_idx
                 * and end_word_idx parameters, is potentially present in the trie.
                 * @param begin_word_idx the begin word index in the given m-gram
                 * @param end_word_idx the end word index in the given m-gram
                 * @param gram the m-gram to work with
                 * @param status [out] the resulting status of the operation
                 */
                inline void is_m_gram_potentially_present(const TModelLevel begin_word_idx,
                        const TModelLevel end_word_idx, const T_Query_M_Gram<WordIndexType> & gram,
                        MGramStatusEnum &status) const {
                    //Check if the end word is unknown
                    if (gram.get_end_word_id() != WordIndexType::UNKNOWN_WORD_ID) {
                        //Compute the model level
                        const TModelLevel curr_level = (end_word_idx - begin_word_idx) + 1;
                        //Check if the caching is enabled
                        if (NEEDS_BITMAP_HASH_CACHE && (curr_level > M_GRAM_LEVEL_1)) {
                            //If the caching is enabled, the higher sub-m-gram levels always require checking
                            const BitmapHashCache & ref = m_bitmap_hash_cach[curr_level - MGRAM_IDX_OFFSET];

                            //Get the m-gram's hash
                            const uint64_t hash = gram.template get_hash(begin_word_idx, end_word_idx);

                            if (ref.is_hash_cached(hash)) {
                                //The m-gram hash is cached, so potentially a payload data
                                status = MGramStatusEnum::GOOD_PRESENT_MGS;
                            } else {
                                //The m-gram hash is not in cache, so definitely no data
                                status = MGramStatusEnum::BAD_NO_PAYLOAD_MGS;
                            }
                        } else {
                            //If caching is not enabled then we always check the trie
                            status = MGramStatusEnum::GOOD_PRESENT_MGS;
                        }
                    } else {
                        //The end word is unknown, so definitely no m-gram data
                        status = MGramStatusEnum::BAD_END_WORD_UNKNOWN_MGS;
                    }
                }

            protected:

                /**
                 * Is to be used from the sub-classes from the add_X_gram methods.
                 * This method allows to register the given M-gram in internal high
                 * level caches if present.
                 * @param gram the M-gram to cache
                 */
                template<TModelLevel CURR_LEVEL>
                inline void register_m_gram_cache(const T_Model_M_Gram<WordIndexType> &gram) {
                    if (NEEDS_BITMAP_HASH_CACHE && (CURR_LEVEL > M_GRAM_LEVEL_1)) {
                        m_bitmap_hash_cach[CURR_LEVEL - MGRAM_IDX_OFFSET].template cache_m_gram_hash<WordIndexType, CURR_LEVEL>(gram);
                    }
                }

            private:

                //Stores the bitmap hash caches per M-gram level for 1 < M <= N
                BitmapHashCache m_bitmap_hash_cach[NUM_M_N_GRAM_LEVELS];
            };

            //Make sure that there will be templates instantiated, at least for the given parameter values
            template class GenericTrieBase<M_GRAM_LEVEL_MAX, BasicWordIndex, true >;
            template class GenericTrieBase<M_GRAM_LEVEL_MAX, CountingWordIndex, true >;
            template class GenericTrieBase<M_GRAM_LEVEL_MAX, OptimizingWordIndex<BasicWordIndex>, true >;
            template class GenericTrieBase<M_GRAM_LEVEL_MAX, OptimizingWordIndex<CountingWordIndex>, true >;
            template class GenericTrieBase<M_GRAM_LEVEL_MAX, BasicWordIndex, false >;
            template class GenericTrieBase<M_GRAM_LEVEL_MAX, CountingWordIndex, false >;
            template class GenericTrieBase<M_GRAM_LEVEL_MAX, OptimizingWordIndex<BasicWordIndex>, false >;
            template class GenericTrieBase<M_GRAM_LEVEL_MAX, OptimizingWordIndex<CountingWordIndex>, false >;

            //Make sure that there will be templates instantiated, at least for the given parameter values
#define INSTANTIATE_TRIE_EXECUTE(TRIE_TYPE_NAME, ...) \
            template void TRIE_TYPE_NAME<__VA_ARGS__>::execute<true>(const T_Query_M_Gram<WordIndexType> & query, const void * payloads[MAX_LEVEL][MAX_LEVEL], TLogProbBackOff probs[MAX_LEVEL]) const;\
            template void TRIE_TYPE_NAME<__VA_ARGS__>::execute<false>(const T_Query_M_Gram<WordIndexType> & query, const void * payloads[MAX_LEVEL][MAX_LEVEL], TLogProbBackOff probs[MAX_LEVEL]) const;

#define INSTANTIATE_TRIE_FUNCS_LEVEL(LEVEL, TRIE_TYPE_NAME, ...) \
            template void TRIE_TYPE_NAME<__VA_ARGS__>::add_m_gram<LEVEL>(const T_Model_M_Gram<TRIE_TYPE_NAME<__VA_ARGS__>::WordIndexType> & gram);

#define INSTANTIATE_TRIE_TEMPLATE_TYPE(TRIE_TYPE_NAME, ...) \
            template class TRIE_TYPE_NAME<__VA_ARGS__>; \
            INSTANTIATE_TRIE_EXECUTE(TRIE_TYPE_NAME, __VA_ARGS__) \
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

