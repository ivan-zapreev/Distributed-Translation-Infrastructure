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
#include "BitMGramId.hpp"
#include "ByteMGramId.hpp"

#include "TextPieceReader.hpp"

#include "ArrayUtils.hpp"
#include "DynamicMemoryArrays.hpp"

using namespace std;

using namespace uva::smt::logging;
using namespace uva::smt::file;
using namespace uva::smt::tries::alloc;
using namespace uva::smt::tries::dictionary;
using namespace uva::smt::tries::mgrams;
using namespace uva::smt::tries::__G2DMapTrie;

namespace uva {
    namespace smt {
        namespace tries {

            namespace __G2DMapTrie {

                /**
                 * This template structure is used for storing trie hash map elements
                 * Each element contains and id of the m-gram and its payload -
                 * the probability/back-off data, the latter is the template parameter
                 * @param id stores the M-gram id
                 * @param payload stores the payload which is either probability or probability with back-off
                 */
                template<typename M_GRAM_ID_TYPE, typename PAYLOAD_TYPE>
                struct S_M_GramData {
                    M_GRAM_ID_TYPE id;
                    PAYLOAD_TYPE payload;

                    //Stores the memory increase strategy object
                    const static MemIncreaseStrategy m_mem_strat;

                    typedef M_GRAM_ID_TYPE TMGramIdType;
                    typedef PAYLOAD_TYPE TPayloadType;
                };

                typedef S_M_GramData<T_Gram_Id_Storage_Ptr, TProbBackOffEntry> T_M_Gram_PB_Entry;
                typedef S_M_GramData<T_Gram_Id_Storage_Ptr, TLogProbBackOff> T_M_Gram_Prob_Entry;

                template<typename ELEMENT_TYPE>
                void destroy_Comp_M_Gram_Id(ELEMENT_TYPE & elem) {
                    M_Gram_Id::destroy(elem.id);
                };

                template void destroy_Comp_M_Gram_Id<T_M_Gram_PB_Entry>(T_M_Gram_PB_Entry &elem);
                template void destroy_Comp_M_Gram_Id<T_M_Gram_Prob_Entry>(T_M_Gram_Prob_Entry &elem);

                template<typename M_GRAM_ID_TYPE, typename PAYLOAD_TYPE>
                const MemIncreaseStrategy S_M_GramData<M_GRAM_ID_TYPE, PAYLOAD_TYPE>::m_mem_strat =
                get_mem_incr_strat(__G2DMapTrie::MEM_INC_TYPE,
                        __G2DMapTrie::MIN_MEM_INC_NUM, __G2DMapTrie::MEM_INC_FACTOR);
            }

            /**
             * This is a Gram to Data trie that is implemented as a HashMap.
             * @param N - the maximum level of the considered N-gram, i.e. the N value
             */
            template<TModelLevel N, typename WordIndexType>
            class G2DMapTrie : public ATrie<N, WordIndexType> {
            public:

                /**
                 * The basic constructor
                 * @param _wordIndex the word index to be used
                 */
                explicit G2DMapTrie(WordIndexType & word_index);

                /**
                 * Allows to log the information about the instantiated trie type
                 */
                virtual void log_trie_type_usage_info() {
                    LOG_USAGE << "Using the <" << __FILE__ << "> model." << END_LOG;
                    LOG_INFO << "Using the #buckets divider: "
                            << SSTR(__G2DMapTrie::WORDS_PER_BUCKET_FACTOR) << END_LOG;
                    LOG_INFO << "Using  and the " << T_M_Gram_PB_Entry::m_mem_strat.get_strategy_info()
                            << " memory allocation strategy." << END_LOG;
                }

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

                    return (level > M_GRAM_LEVEL_1) || ATrie<N, WordIndexType>::is_post_grams(level);
                }

                /**
                 * The basic class destructor
                 */
                virtual ~G2DMapTrie();

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
                 * This function allows to retrieve the probability stored for the given M-gram level.
                 * If the value is found then it must be set to the prob parameter of the function.
                 * If the value is not found then the prob parameter of the function must not be changed.
                 * @see ATrie
                 */
                virtual void get_prob_weight(MGramQuery<N, WordIndexType> & query);

                /**
                 * This function allows to retrieve the back-off stored for the given M-gram level.
                 * If the value is found then it must be added to the prob parameter of the function.
                 * If the value is not found then the prob parameter of the function must not be changed.
                 * In that case the back-off weight is just zero.
                 * @see ATrie
                 */
                virtual void add_back_off_weight(MGramQuery<N, WordIndexType> & query);

                /**
                 * Allows to get the bucket index for the given M-gram
                 * @param gram the M-gram to compute the bucked index for
                 * @param bucket_idx the resulting bucket index
                 */
                inline uint32_t get_bucket_id(const uint64_t gram_hash, const TModelLevel level) {
                    //Compute the index in the array of bucket sizes
                    const TModelLevel buckes_size_idx = level - 1;

                    //Compute the bucket Id from the M-Gram hash
                    return gram_hash % num_buckets[buckes_size_idx];
                }

                /**
                 * Allows to get the bucket index for the given M-gram
                 * @param gram the M-gram to compute the bucked index for
                 * @param bucket_idx the resulting bucket index
                 */
                inline void get_bucket_id(const T_M_Gram &gram, TShortId & bucket_idx) {
                    //Compute the hash value for the given M-gram, it must
                    //be the M-Gram id in the M-Gram data storage
                    const uint64_t gram_hash = gram.hash();
                    LOG_DEBUG1 << "The " << gram.level << "-gram: " << tokensToString(gram)
                            << " hash is " << gram_hash << END_LOG;

                    bucket_idx = get_bucket_id(gram_hash, gram.level);

                    LOG_DEBUG1 << "Getting bucket for " << tokensToString(gram) << " bucket_idx: " << SSTR(bucket_idx) << END_LOG;

                    //If the sanity check is on then check on that the id is within the range
                    if (DO_SANITY_CHECKS && ((bucket_idx < 0) || (bucket_idx >= num_buckets[gram.level - 1]))) {
                        stringstream msg;
                        msg << "The " << SSTR(gram.level) << "-gram: " << tokensToString<N>(gram)
                                << " was given an incorrect hash: " << SSTR(bucket_idx)
                                << ", must be within [0, " << SSTR(num_buckets[gram.level - 1]) << "]";
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

                        LOG_DEBUG1 << "Shrinking the " << SSTR(level) << "-gram level bucket idx: " << SSTR(bucket_idx) << " ..." << END_LOG;
                        //Reduce capacity if there is unused memory
                        ref.shrink();
                        LOG_DEBUG1 << "Shrinking the " << SSTR(level) << "-gram level bucket idx: " << SSTR(bucket_idx) << " is done" << END_LOG;

                        LOG_DEBUG1 << "Sorting the " << SSTR(level) << "-gram level bucket idx: " << SSTR(bucket_idx) << " ..." << END_LOG;
                        //Order the N-gram array as it is unordered and we will binary search it later!
                        ref.sort([&] (const typename BUCKET_TYPE::TElemType & first, const typename BUCKET_TYPE::TElemType & second) -> bool {
                            LOG_DEBUG1 << "Comparing " << SSTR((void*) first.id) << " with " << SSTR((void*) second.id) << END_LOG;
                            //Update the progress bar status
                            Logger::updateProgressBar();
                                    //Return the result
                            return Byte_M_Gram_Id::is_less_m_grams_id(first.id, second.id, level);
                        });
                        LOG_DEBUG1 << "Sorting the " << SSTR(level) << "-gram level bucket idx: " << SSTR(bucket_idx) << " is done" << END_LOG;
                    }
                }

            private:
                //Stores the pointer to the temporary re-usable M-gram id for queries
                T_Gram_Id_Storage_Ptr m_tmp_gram_id;
                        
                //Stores the 1-gram data
                TProbBackOffEntry * m_1_gram_data;

                //These are arrays of buckets for M-Gram levels with 1 < M < N
                typedef ADynamicStackArray<T_M_Gram_PB_Entry, uint8_t, &__G2DMapTrie::destroy_Comp_M_Gram_Id<T_M_Gram_PB_Entry> > TProbBackOffBucket;
                TProbBackOffBucket * m_M_gram_data[ATrie<N, WordIndexType>::NUM_M_GRAM_LEVELS];

                //This is an array of buckets for the N-Gram level
                typedef ADynamicStackArray<T_M_Gram_Prob_Entry, uint8_t, &__G2DMapTrie::destroy_Comp_M_Gram_Id<T_M_Gram_Prob_Entry> > TProbBucket;
                TProbBucket * m_N_gram_data;

                //Stores the number of gram ids/buckets per level
                TShortId num_buckets[N];

                /**
                 * Allows to perform search in the bucket for the given M-gram id
                 * @param ref the reference to the bucket
                 * @param found_idx the found index
                 * @return true if the M-gram id was found and otherwise false
                 */
                template<typename BUCKET_TYPE, TModelLevel M_GRAM_LEVEL>
                inline bool search_gram(const BUCKET_TYPE & ref, typename BUCKET_TYPE::TIndexType &found_idx) {
                    return my_bsearch_id< typename BUCKET_TYPE::TElemType,
                            typename BUCKET_TYPE::TIndexType,
                            typename BUCKET_TYPE::TElemType::TMGramIdType,
                            Byte_M_Gram_Id::compare<M_GRAM_LEVEL> >
                            (ref.data(), 0, ref.size() - 1, m_tmp_gram_id,
                            found_idx);
                }

                /**
                 * Gets the probability for the given level M-gram, searches on specific level
                 * @param BUCKET_TYPE the level bucket type
                 * @param back_off true if this is the back-off data we are retrieving, otherwise false, default is false
                 * @param query the query M-gram state 
                 * @param ref the bucket to search in
                 * @param payload_ptr [out] the reference to the pointer of the payload, to be set within this method
                 * @return true if the M-gram was found and otherwise false.
                 */
                template<typename BUCKET_TYPE, bool back_off = false >
                bool get_payload_from_gram_level(const MGramQuery<N, WordIndexType> & query, const BUCKET_TYPE & ref,
                        const typename BUCKET_TYPE::TElemType::TPayloadType * & payload_ptr);

            };
        }
    }
}


#endif	/* G2DHASHMAPTRIE_HPP */

