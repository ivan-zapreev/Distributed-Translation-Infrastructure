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

#include "MGrams.hpp"
#include "TextPieceReader.hpp"

#include "MGramQuery.hpp"

#include "BasicWordIndex.hpp"
#include "CountingWordIndex.hpp"
#include "OptimizingWordIndex.hpp"

#include "GenericTrieBase.hpp"

#include "LayeredTrieDriver.hpp"

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
             * This is the common generic trie base class.
             * @param N the maximum level of the considered N-gram, i.e. the N value
             * @param TrieType the type of word index to be used
             */
            template<typename TrieType>
            class GenericTrieDriver : public GenericTrieBase<TrieType::MAX_LEVEL, typename TrieType::WordIndexType> {
            public:
                static constexpr TModelLevel MAX_LEVEL = TrieType::MAX_LEVEL;
                typedef typename TrieType::WordIndexType WordIndexType;
                typedef typename TrieType::TMGramQuery TMGramQuery;
                typedef GenericTrieBase<MAX_LEVEL, WordIndexType> BASE;

                //The typedef for the retrieving function
                typedef function<void(const GenericTrieDriver&, TMGramQuery & query) > TRetrieveDataFunct;

                /**
                 * The basic constructor
                 * @param word_index the word index to be used
                 */
                explicit GenericTrieDriver(WordIndexType & word_index)
                : GenericTrieBase<MAX_LEVEL, WordIndexType> (word_index),
                m_trie(word_index), m_is_bitmap_hash_cache(m_trie.is_bitmap_hash_cache()) {
                }

                /**
                 * @see GenericTrieBase
                 */
                void pre_allocate(const size_t counts[MAX_LEVEL]) {
                    //Pre-allocate the bitmap-hash caches if needed
                    if (m_is_bitmap_hash_cache) {
                        for (size_t idx = 0; idx < BASE::NUM_M_N_GRAM_LEVELS; ++idx) {
                            m_bitmap_hash_cach[idx].pre_allocate(counts[idx + 1]);
                            Logger::updateProgressBar();
                        }
                    }

                    m_trie.pre_allocate(counts);
                };

                /**
                 * @see GenericTrieBase
                 */
                inline void add_1_gram(const T_M_Gram<WordIndexType> &gram) {
                    m_trie.add_1_gram(gram);
                };

                /**
                 * @see GenericTrieBase
                 */
                template<TModelLevel CURR_LEVEL>
                inline void add_m_gram(const T_M_Gram<WordIndexType> & gram) {
                    if (m_is_bitmap_hash_cache) {
                        //Call the super class first, is needed for caching
                        register_m_gram_cache<CURR_LEVEL>(gram);
                    }

                    m_trie.template add_m_gram<CURR_LEVEL>(gram);
                };

                /**
                 * @see GenericTrieBase
                 */
                inline void add_n_gram(const T_M_Gram<WordIndexType> & gram) {
                    if (m_is_bitmap_hash_cache) {
                        //Call the super class first, is needed for caching
                        register_m_gram_cache<MAX_LEVEL>(gram);
                    }

                    m_trie.add_n_gram(gram);
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
                 * Allows to check if the given sub-m-gram contains an unknown word
                 * @param curr_level the currently considered level of the m-gram
                 * @return true if the unknown word is present, otherwise false
                 */
                template<bool IS_BACK_OFF, TModelLevel CURR_LEVEL>
                inline bool is_bitmap_hash_cache(TMGramQuery & query) const {
                    if (m_is_bitmap_hash_cache) {
                        const BitmapHashCache & ref = m_bitmap_hash_cach[CURR_LEVEL - BASE::MGRAM_IDX_OFFSET];
                        return ref.is_m_gram<IS_BACK_OFF, CURR_LEVEL>(query.m_gram);
                    } else {
                        return true;
                    }
                }

                /**
                 * This method will get the N-gram in a form of a vector, e.g.:
                 *      [word1 word2 word3 word4 word5]
                 * and will compute and return the Language Model Probability for it
                 * @param query the given M-Gram query and its state
                 */
                void execute(TMGramQuery & query) {
                    LOG_DEBUG << "Starting to execute:" << (string) query.m_gram << END_LOG;

                    //Make sure that the query is prepared for execution
                    TModelLevel curr_level = query.prepare_query();

                    //Compute the probability in the loop fashion, should be faster that recursion.
                    while ((query.m_result.m_prob == ZERO_PROB_WEIGHT) && (!DO_SANITY_CHECKS || (curr_level != 0))) {
                        //Try to compute the next probability with decreased level
                        cache_check_get_prob_weight_func[curr_level](*this, query);
                        //Decrease the level
                        curr_level--;
                    }

                    //If the probability is log-zero or snaller then there is no
                    //need for a back-off as then we will only get smaller values.
                    if (query.m_result.m_prob > ZERO_LOG_PROB_WEIGHT) {
                        //If the curr_level is smaller than the original level then
                        //it means that we needed to back-off, add back-off weights
                        for (++curr_level; curr_level != query.m_gram.m_used_level; ++curr_level) {
                            //Get the back_off 
                            cache_check_add_back_off_weight_func[curr_level](*this, query);
                        }
                    }

                    LOG_DEBUG << "The computed log_" << LOG_PROB_WEIGHT_BASE
                            << " probability is: " << query.m_result.m_prob << END_LOG;
                }

                /**
                 * The basic class destructor
                 */
                virtual ~GenericTrieDriver() {
                };

            protected:
                //Stores the trie
                TrieType m_trie;

                //Stores a flag of whether we should use the bitmap hash cache
                const bool m_is_bitmap_hash_cache;

                //Stores the bitmap hash caches per M-gram level
                BitmapHashCache m_bitmap_hash_cach[BASE::NUM_M_N_GRAM_LEVELS];

                //Declare static arrays of pointers to the template function instances 
                static const TRetrieveDataFunct cache_check_get_prob_weight_func[];
                static const TRetrieveDataFunct cache_check_add_back_off_weight_func[];

                /**
                 * Is to be used from the sub-classes from the add_X_gram methods.
                 * This method allows to register the given M-gram in internal high
                 * level caches if present.
                 * @param gram the M-gram to cache
                 */
                template<TModelLevel CURR_LEVEL>
                inline void register_m_gram_cache(const T_M_Gram<WordIndexType> &gram) {
                    if (m_is_bitmap_hash_cache && (gram.m_used_level > M_GRAM_LEVEL_1)) {
                        m_bitmap_hash_cach[gram.m_used_level - BASE::MGRAM_IDX_OFFSET].template add_m_gram<WordIndexType, CURR_LEVEL>(gram);
                    }
                }

                /**
                 * Allows to get the probability value also by checking the cache.
                 * If the probability is not found then the prob value is to stay intact!
                 * @param CURR_LEVEL the currently considered level of the m-gram
                 * @param query the M-gram query for a specific current level
                 */
                template<TModelLevel CURR_LEVEL>
                void cache_check_get_prob_weight(TMGramQuery & query) const {
                    LOG_DEBUG << "cache_check_add_prob_value(" << CURR_LEVEL
                            << ") = " << query.m_result.m_prob << END_LOG;

                    //Try getting the probability value.
                    //1. If the level is one go on: we can get smth
                    //even if the 1-Gram consists of just an unknown word.
                    //2. If the context length is more then one and there is
                    //an unknown word in the gram then it makes no sense to do
                    //searching as there are no M-grams with <unk> in them
                    if ((CURR_LEVEL == M_GRAM_LEVEL_1) ||
                            ((CURR_LEVEL > M_GRAM_LEVEL_1)
                            && query.template has_no_unk_words<false, CURR_LEVEL>()
                            && is_bitmap_hash_cache<false, CURR_LEVEL>(query))) {
                        //Let's look further, may be we will find something!
                        LOG_DEBUG1 << "All pre-checks are passed, calling add_prob_value(level, prob)!" << END_LOG;
                        m_trie.template get_prob_weight<CURR_LEVEL>(query);
                    } else {
                        LOG_DEBUG << "Could try to get probs but it will not be "
                                << "successful due to the present unk words! "
                                << "Thus backing off right away!" << END_LOG;
                    }
                }

                /**
                 * Allows to get the back-off weight value also by checking the cache.
                 * If the back-off is not found then the probability value is to stay intact.
                 * Then the back-off weight is considered to be zero!
                 * @param CURR_LEVEL the currently considered level of the m-gram
                 * @param query the M-gram query for a specific current level
                 */
                template<TModelLevel CURR_LEVEL>
                void cache_check_add_back_off_weight(TMGramQuery & query) const {
                    LOG_DEBUG << "cache_check_add_back_off_weight(" << CURR_LEVEL
                            << ") = " << query.m_result.m_prob << END_LOG;

                    //Try getting the back-off weight.
                    //1. If the context length is one go on: we can get smth
                    //even if the 1-Gram consists of just an unknown word.
                    //2. If the context length is more then one and there is
                    //an unknown word in the gram then it makes no sense to do
                    //searching as there are no M-grams with <unk> in them
                    if ((CURR_LEVEL == M_GRAM_LEVEL_1) ||
                            ((CURR_LEVEL > M_GRAM_LEVEL_1)
                            && query.template has_no_unk_words<true, CURR_LEVEL>()
                            && is_bitmap_hash_cache<true, CURR_LEVEL>(query))) {
                        //Let's look further, we definitely get some back-off weight or zero!
                        LOG_DEBUG1 << "All pre-checks are passed, calling add_back_off_weight(level, prob)!" << END_LOG;
                        m_trie.template add_back_off_weight<CURR_LEVEL>(query);
                    } else {
                        LOG_DEBUG << "Could try to back off but it will not be "
                                << "successful due to the present unk words! Thus "
                                << "the back-off weight is zero!" << END_LOG;
                    }
                }
            };

            template<typename TrieType>
            const typename GenericTrieDriver<TrieType>::TRetrieveDataFunct GenericTrieDriver<TrieType>::cache_check_get_prob_weight_func[] = {
                NULL,
                &GenericTrieDriver::cache_check_get_prob_weight<M_GRAM_LEVEL_1>,
                &GenericTrieDriver::cache_check_get_prob_weight<M_GRAM_LEVEL_2>,
                &GenericTrieDriver::cache_check_get_prob_weight<M_GRAM_LEVEL_3>,
                &GenericTrieDriver::cache_check_get_prob_weight<M_GRAM_LEVEL_4>,
                &GenericTrieDriver::cache_check_get_prob_weight<M_GRAM_LEVEL_5>,
                &GenericTrieDriver::cache_check_get_prob_weight<M_GRAM_LEVEL_6>,
                &GenericTrieDriver::cache_check_get_prob_weight<M_GRAM_LEVEL_7>
            };

            template<typename TrieType>
            const typename GenericTrieDriver<TrieType>::TRetrieveDataFunct GenericTrieDriver<TrieType>::cache_check_add_back_off_weight_func[] = {
                NULL,
                &GenericTrieDriver::cache_check_add_back_off_weight<M_GRAM_LEVEL_1>,
                &GenericTrieDriver::cache_check_add_back_off_weight<M_GRAM_LEVEL_2>,
                &GenericTrieDriver::cache_check_add_back_off_weight<M_GRAM_LEVEL_3>,
                &GenericTrieDriver::cache_check_add_back_off_weight<M_GRAM_LEVEL_4>,
                &GenericTrieDriver::cache_check_add_back_off_weight<M_GRAM_LEVEL_5>,
                &GenericTrieDriver::cache_check_add_back_off_weight<M_GRAM_LEVEL_6>,
                &GenericTrieDriver::cache_check_add_back_off_weight<M_GRAM_LEVEL_7>
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

