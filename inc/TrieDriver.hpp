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

#ifndef TRIE_DRIVER_HPP
#define	TRIE_DRIVER_HPP

#include <string>       // std::string

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

#include "G2DHashMapTrie.hpp"
#include "C2DHashMapTrie.hpp"
#include "W2CHybridMemoryTrie.hpp"
#include "C2WOrderedArrayTrie.hpp"
#include "W2COrderedArrayTrie.hpp"
#include "C2DMapArrayTrie.hpp"

using namespace std;
using namespace uva::smt::logging;
using namespace uva::smt::file;
using namespace uva::smt::tries::dictionary;
using namespace uva::smt::tries::mgrams;
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
            class TrieDriver : public GenericTrieBase<TrieType::max_level, typename TrieType::WordIndexType> {
            public:
                typedef typename TrieType::WordIndexType WordIndexType;
                typedef typename TrieType::TMGramQuery TMGramQuery;

                /**
                 * The basic constructor
                 * @param word_index the word index to be used
                 */
                explicit TrieDriver(WordIndexType & word_index)
                : GenericTrieBase<TrieType::max_level, WordIndexType> (word_index), m_trie(word_index) {
                }

                /**
                 * @see GenericTrieBase
                 */
                void pre_allocate(const size_t counts[TrieType::max_level]) {
                    m_trie.pre_allocate(counts);
                };

                /**
                 * @see GenericTrieBase
                 */
                inline void add_1_gram(const T_M_Gram &gram) {
                    m_trie.add_1_gram(gram);
                };

                /**
                 * @see GenericTrieBase
                 */
                inline void add_m_gram(const T_M_Gram & gram) {
                    m_trie.add_m_gram(gram);
                };

                /**
                 * @see GenericTrieBase
                 */
                inline void add_n_gram(const T_M_Gram & gram) {
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
                inline bool is_post_grams(const TModelLevel level) {
                    return m_trie.is_post_grams(level);
                };

                /**
                 * @see GenericTrieBase
                 */
                inline void post_grams(const TModelLevel level) {
                    m_trie.post_grams(level);
                };

                /**
                 * This method will get the N-gram in a form of a vector, e.g.:
                 *      [word1 word2 word3 word4 word5]
                 * and will compute and return the Language Model Probability for it
                 * @param query the given M-Gram query and its state
                 */
                void execute(TMGramQuery & query) {
                    LOG_DEBUG << "Starting to execute:" << tokensToString(query.m_gram) << END_LOG;

                    //Make sure that the query is prepared for execution
                    query.prepare_query();

                    //Compute the probability in the loop fashion, should be faster that recursion.
                    while ((query.result.prob == ZERO_PROB_WEIGHT) && (!DO_SANITY_CHECKS || (query.curr_level != 0))) {
                        //Try to compute the next probability with decreased level
                        cache_check_get_prob_weight(query);
                        //Decrease the level
                        query.curr_level--;
                    }

                    //If the probability is log-zero or snaller then there is no
                    //need for a back-off as then we will only get smaller values.
                    if (query.result.prob > ZERO_LOG_PROB_WEIGHT) {
                        //If the curr_level is smaller than the original level then
                        //it means that we needed to back-off, add back-off weights
                        for (++query.curr_level; query.curr_level != query.m_gram.level; ++query.curr_level) {
                            //Get the back_off 
                            cache_check_add_back_off_weight(query);
                        }
                    }

                    LOG_DEBUG << "The computed log_" << LOG_PROB_WEIGHT_BASE
                            << " probability is: " << query.result.prob << END_LOG;
                }

                /**
                 * The basic class destructor
                 */
                virtual ~TrieDriver() {
                };

            protected:

                /**
                 * Allows to get the probability value also by checking the cache.
                 * If the probability is not found then the prob value is to stay intact!
                 * @param query the M-gram query for a specific current level
                 */
                void cache_check_get_prob_weight(TMGramQuery & query) {
                    LOG_DEBUG << "cache_check_add_prob_value(" << query.curr_level
                            << ") = " << query.result.prob << END_LOG;

                    //Try getting the probability value.
                    //1. If the level is one go on: we can get smth
                    //even if the 1-Gram consists of just an unknown word.
                    //2. If the context length is more then one and there is
                    //an unknown word in the gram then it makes no sense to do
                    //searching as there are no M-grams with <unk> in them
                    if ((query.curr_level == M_GRAM_LEVEL_1) ||
                            ((query.curr_level > M_GRAM_LEVEL_1)
                            && query.template has_no_unk_words<false>()
                            && m_trie.template is_bitmap_hash_cache<false>(query))) {
                        //Let's look further, may be we will find something!
                        LOG_DEBUG1 << "All pre-checks are passed, calling add_prob_value(level, prob)!" << END_LOG;
                        m_trie.get_prob_weight(query);
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
                 * @param query the M-gram query for a specific current level
                 */
                void cache_check_add_back_off_weight(TMGramQuery & query) {
                    LOG_DEBUG << "cache_check_add_back_off_weight(" << query.curr_level
                            << ") = " << query.result.prob << END_LOG;

                    //Try getting the back-off weight.
                    //1. If the context length is one go on: we can get smth
                    //even if the 1-Gram consists of just an unknown word.
                    //2. If the context length is more then one and there is
                    //an unknown word in the gram then it makes no sense to do
                    //searching as there are no M-grams with <unk> in them
                    if ((query.curr_level == M_GRAM_LEVEL_1) ||
                            ((query.curr_level > M_GRAM_LEVEL_1)
                            && query.template has_no_unk_words<true>()
                            && m_trie.template is_bitmap_hash_cache<true>(query))) {
                        //Let's look further, we definitely get some back-off weight or zero!
                        LOG_DEBUG1 << "All pre-checks are passed, calling add_back_off_weight(level, prob)!" << END_LOG;
                        m_trie.add_back_off_weight(query);
                    } else {
                        LOG_DEBUG << "Could try to back off but it will not be "
                                << "successful due to the present unk words! Thus "
                                << "the back-off weight is zero!" << END_LOG;
                    }
                }

            private:
                //Stores the trie
                TrieType m_trie;
            };

            //Make sure that there will be templates instantiated, at least for the given parameter values
            template class TrieDriver<LayeredTrieDriver< C2DMapTrie<M_GRAM_LEVEL_MAX, BasicWordIndex > > >;
            template class TrieDriver<LayeredTrieDriver< C2DMapTrie<M_GRAM_LEVEL_MAX, CountingWordIndex> > >;
            template class TrieDriver<LayeredTrieDriver< C2DMapTrie<M_GRAM_LEVEL_MAX, OptimizingWordIndex<BasicWordIndex> > > >;
            template class TrieDriver<LayeredTrieDriver< C2DMapTrie<M_GRAM_LEVEL_MAX, OptimizingWordIndex<CountingWordIndex> > > >;

            template class TrieDriver<LayeredTrieDriver< C2DHybridTrie<M_GRAM_LEVEL_MAX, BasicWordIndex > > >;
            template class TrieDriver<LayeredTrieDriver< C2DHybridTrie<M_GRAM_LEVEL_MAX, CountingWordIndex> > >;
            template class TrieDriver<LayeredTrieDriver< C2DHybridTrie<M_GRAM_LEVEL_MAX, OptimizingWordIndex<BasicWordIndex> > > >;
            template class TrieDriver<LayeredTrieDriver< C2DHybridTrie<M_GRAM_LEVEL_MAX, OptimizingWordIndex<CountingWordIndex> > > >;

            template class TrieDriver<LayeredTrieDriver< C2WArrayTrie<M_GRAM_LEVEL_MAX, BasicWordIndex > > >;
            template class TrieDriver<LayeredTrieDriver< C2WArrayTrie<M_GRAM_LEVEL_MAX, CountingWordIndex> > >;
            template class TrieDriver<LayeredTrieDriver< C2WArrayTrie<M_GRAM_LEVEL_MAX, OptimizingWordIndex<BasicWordIndex> > > >;
            template class TrieDriver<LayeredTrieDriver< C2WArrayTrie<M_GRAM_LEVEL_MAX, OptimizingWordIndex<CountingWordIndex> > > >;

            template class TrieDriver<LayeredTrieDriver< typename TW2CHybridTrie<M_GRAM_LEVEL_MAX, BasicWordIndex >::type > >;
            template class TrieDriver<LayeredTrieDriver< typename TW2CHybridTrie<M_GRAM_LEVEL_MAX, CountingWordIndex>::type > >;
            template class TrieDriver<LayeredTrieDriver< typename TW2CHybridTrie<M_GRAM_LEVEL_MAX, OptimizingWordIndex<BasicWordIndex> >::type > >;
            template class TrieDriver<LayeredTrieDriver< typename TW2CHybridTrie<M_GRAM_LEVEL_MAX, OptimizingWordIndex<CountingWordIndex> >::type > >;

            template class TrieDriver<LayeredTrieDriver< W2CArrayTrie<M_GRAM_LEVEL_MAX, BasicWordIndex > > >;
            template class TrieDriver<LayeredTrieDriver< W2CArrayTrie<M_GRAM_LEVEL_MAX, CountingWordIndex> > >;
            template class TrieDriver<LayeredTrieDriver< W2CArrayTrie<M_GRAM_LEVEL_MAX, OptimizingWordIndex<BasicWordIndex> > > >;
            template class TrieDriver<LayeredTrieDriver< W2CArrayTrie<M_GRAM_LEVEL_MAX, OptimizingWordIndex<CountingWordIndex> > > >;

            template class TrieDriver<G2DMapTrie< M_GRAM_LEVEL_MAX, BasicWordIndex > >;
            template class TrieDriver<G2DMapTrie< M_GRAM_LEVEL_MAX, CountingWordIndex > >;
            template class TrieDriver<G2DMapTrie< M_GRAM_LEVEL_MAX, OptimizingWordIndex<BasicWordIndex> > >;
            template class TrieDriver<G2DMapTrie< M_GRAM_LEVEL_MAX, OptimizingWordIndex<CountingWordIndex> > >;
        }
    }
}

#endif	/* ATRIE_HPP */

