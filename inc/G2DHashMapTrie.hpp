/* 
 * File:   G2DHashMapTrie.hpp
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
 * Created on September 3, 2015, 3:32 PM
 */

#ifndef G2DHASHMAPTRIE_HPP
#define	G2DHASHMAPTRIE_HPP

#include <string>       // std::string

#include "Globals.hpp"
#include "Exceptions.hpp"

#include "ATrie.hpp"
#include "AWordIndex.hpp"
#include "MGrams.hpp"

#include "TextPieceReader.hpp"

#include "ArrayUtils.hpp"
#include "DynamicMemoryArrays.hpp"

using namespace std;

using namespace uva::smt::logging;
using namespace uva::smt::file;
using namespace uva::smt::tries::alloc;
using namespace uva::smt::tries::dictionary;
using namespace uva::smt::tries::mgrams;
using namespace uva::smt::tries::__G2DHashMapTrie;

namespace uva {
    namespace smt {
        namespace tries {

            namespace __G2DHashMapTrie {

                /**
                 * This template structure is used for storing trie hash map elements
                 * Each element contains and id of the m-gram and its payload -
                 * the probability/back-off data, the latter is the template parameter
                 */
                template<typename M_GRAM_ID_TYPE, typename PAYLOAD_TYPE>
                struct S_M_GramData {
                    M_GRAM_ID_TYPE m_gram_id;
                    PAYLOAD_TYPE payload;

                    //Stores the memory increase strategy object
                    const static MemIncreaseStrategy m_mem_strat;

                    typedef M_GRAM_ID_TYPE TMGramIdType;
                };

                typedef S_M_GramData<T_Comp_M_Gram_Id_Ptr, TProbBackOffEntry> T_M_Gram_Prob_Back_Off_Entry;
                typedef S_M_GramData<T_Comp_M_Gram_Id_Ptr, TLogProbBackOff> T_M_Gram_Prob_Entry;
                
                template<typename ELEMENT_TYPE>
                void destroy_Comp_M_Gram_Id(ELEMENT_TYPE & elem) {
                    Comp_M_Gram_Id::destroy(elem.m_gram_id);
                };
                
                template void destroy_Comp_M_Gram_Id<T_M_Gram_Prob_Back_Off_Entry>(T_M_Gram_Prob_Back_Off_Entry &elem);
                template void destroy_Comp_M_Gram_Id<T_M_Gram_Prob_Entry>(T_M_Gram_Prob_Entry &elem);

                template<typename M_GRAM_ID_TYPE, typename PAYLOAD_TYPE>
                const MemIncreaseStrategy S_M_GramData<M_GRAM_ID_TYPE, PAYLOAD_TYPE>::m_mem_strat =
                get_mem_incr_strat(__G2DHashMapTrie::MEM_INC_TYPE,
                        __G2DHashMapTrie::MIN_MEM_INC_NUM, __G2DHashMapTrie::MEM_INC_FACTOR);
            }

            /**
             * This is a Gram to Data trie that is implemented as a HashMap.
             * @param N - the maximum level of the considered N-gram, i.e. the N value
             */
            template<TModelLevel N>
            class G2DHashMapTrie : public ATrie<N> {
            public:

                /**
                 * The basic constructor
                 * @param _wordIndex the word index to be used
                 */
                explicit G2DHashMapTrie(AWordIndex * const _pWordIndex);

                /**
                 * This method can be used to provide the N-gram count information
                 * That should allow for pre-allocation of the memory
                 * @see ATrie
                 */
                virtual void pre_allocate(const size_t counts[N]);

                /**
                 * This method adds a 1-Gram (word) to the trie.
                 * It it snot guaranteed that the parameter will be checked to be a 1-Gram!
                 * @see ATrie
                 */
                virtual void add_1_gram(const T_M_Gram &oGram);

                /**
                 * This method adds a M-Gram (word) to the trie where 1 < M < N
                 * @see ATrie
                 */
                virtual void add_m_gram(const T_M_Gram &mGram);

                /**
                 * This method adds a N-Gram (word) to the trie where
                 * It it not guaranteed that the parameter will be checked to be a N-Gram!
                 * @see ATrie
                 */
                virtual void add_n_gram(const T_M_Gram &nGram);

                /**
                 * This method allows to check if post processing should be called after
                 * all the X level grams are read. This method is virtual.
                 * @see ATrie
                 */
                virtual bool is_post_grams(const TModelLevel level) {
                    //Check the base class and we need to do post actions
                    //for all the M-grams with 1 < M <= N. The M-grams level
                    //data has to be ordered per bucket per id, see
                    //post_M_Grams, and post_N_Grams methods below.

                    return (level > M_GRAM_LEVEL_1) || ATrie<N>::is_post_grams(level);
                }

                /**
                 * The basic class destructor
                 */
                virtual ~G2DHashMapTrie();

            protected:

                /**
                 * This method will be called after all the M-grams are read.
                 * The default implementation of this method is present.
                 * If overridden must be called from the child method.
                 * @see ATrie
                 */
                virtual void post_m_grams(const TModelLevel level);
                
                /**
                 * This method will be called after all the N-grams are read.
                 * The default implementation of this method is present.
                 * If overridden must be called from the child method.
                 * @see ATrie
                 */
                virtual void post_n_grams();

                /**
                 * This recursive function implements the computation of the
                 * N-Gram probabilities in the Back-Off Language Model.
                 * @see ATrie
                 */
                virtual void get_probability(const TModelLevel level, TLogProbBackOff & prob);

                /**
                 * This function allows to get the back-off weight for the current context.
                 * @see ATrie
                 */
                virtual bool get_back_off_weight(const TModelLevel level, TLogProbBackOff & back_off);

                /**
                 * Allows to get the bucket index for the given M-gram
                 * @param gram the M-gram to compute the bucked index for
                 * @param bucket_idx the resulting bucket index
                 */
                inline void get_bucket_id(const T_M_Gram &gram, TShortId & bucket_idx) {
                    //Compute the hash value for the given M-gram, it must
                    //be the M-Gram id in the M-Gram data storage
                    const TShortId gramHash = gram.hash();
                    //Compute the index in the array of bucket sizes
                    const TModelLevel buckes_size_idx = gram.level - 1;
                    //Compute the bucket Id from the M-Gram hash
                    bucket_idx = gramHash % num_buckets[buckes_size_idx];

                    LOG_DEBUG3 << "Getting bucket for " << tokensToString(gram) << " idx: " << SSTR(bucket_idx) << END_LOG;

                    //If the sanity check is on then check on that the id is within the range
                    if (DO_SANITY_CHECKS && ((bucket_idx < 0) || (bucket_idx >= num_buckets[buckes_size_idx]))) {
                        stringstream msg;
                        msg << "The " << SSTR(gram.level) << "-gram: " << tokensToString<N>(gram)
                                << " was given an incorrect hash: " << SSTR(bucket_idx)
                                << ", must be within [0, " << SSTR(num_buckets[buckes_size_idx]) << "]";
                        throw Exception(msg.str());
                    }
                }

                /**
                 * Performs the post-processing actions on the buckets in the given M-gram level
                 * @param BUCKET_TYPE the sort of buckets we should work with
                 * @param buckets the pointer to the array of buckets to process
                 * @param level the M-gram level value M
                 */
                template<typename BUCKET_TYPE>
                void post_M_N_Grams(BUCKET_TYPE * buckets, const TModelLevel level) {
                    //Iterate through all buckets and shrink/sort sub arrays
                    for (TShortId bucket_idx = 0; bucket_idx < num_buckets[level - 1]; ++bucket_idx) {
                        //First get the sub-array reference. 
                        BUCKET_TYPE & ref = buckets[bucket_idx];

                        LOG_DEBUG3 << "Shrinking the " << SSTR(level) << "-gram level bucket idx: " << SSTR(bucket_idx) << " ..." << END_LOG;
                        //Reduce capacity if there is unused memory
                        ref.shrink();
                        LOG_DEBUG3 << "Shrinking the " << SSTR(level) << "-gram level bucket idx: " << SSTR(bucket_idx) << " is done" << END_LOG;

                        LOG_DEBUG3 << "Sorting the " << SSTR(level) << "-gram level bucket idx: " << SSTR(bucket_idx) << " ..." << END_LOG;
                        //Order the N-gram array as it is unordered and we will binary search it later!
                        ref.sort([&] (const typename BUCKET_TYPE::TElemType & first, const typename BUCKET_TYPE::TElemType & second) -> bool {
                            LOG_DEBUG3 << "Comparing " << SSTR((void*) first.m_gram_id) << " with " << SSTR((void*) second.m_gram_id) << END_LOG;
                            //Update the progress bar status
                            Logger::updateProgressBar();
                                    //Return the result
                            return Comp_M_Gram_Id::is_less_m_grams_id(first.m_gram_id, second.m_gram_id, level);
                        });
                        LOG_DEBUG3 << "Sorting the " << SSTR(level) << "-gram level bucket idx: " << SSTR(bucket_idx) << " is done" << END_LOG;
                    }
                }

            private:

                //Stores the 1-gram data
                TProbBackOffEntry * m_1_gram_data;

                //These are arrays of buckets for M-Gram levels with 1 < M < N
                typedef ADynamicStackArray<T_M_Gram_Prob_Back_Off_Entry, uint8_t, &__G2DHashMapTrie::destroy_Comp_M_Gram_Id<T_M_Gram_Prob_Back_Off_Entry> > TProbBackOffBucket;
                TProbBackOffBucket * m_M_gram_data[ATrie<N>::NUM_M_GRAM_LEVELS];

                //This is an array of buckets for the N-Gram level
                typedef ADynamicStackArray<T_M_Gram_Prob_Entry, uint8_t, &__G2DHashMapTrie::destroy_Comp_M_Gram_Id<T_M_Gram_Prob_Entry> > TProbBucket;
                TProbBucket * m_N_gram_data;

                //Stores the number of gram ids/buckets per level
                TShortId num_buckets[N];
            };
        }
    }
}


#endif	/* G2DHASHMAPTRIE_HPP */

