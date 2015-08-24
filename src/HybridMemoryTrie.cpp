/* 
 * File:   HybridMemoryTrie.cpp
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
 * Created on August 21, 2015, 4:18 PM
 */
#include "HybridMemoryTrie.hpp"

#include <inttypes.h>       // std::uint32_t

#include "Globals.hpp"
#include "Logger.hpp"
#include "Exceptions.hpp"

namespace uva {
    namespace smt {
        namespace tries {

            template<TModelLevel N, template<TModelLevel > class StorageFactory, class StorageContainer>
            HybridMemoryTrie<N, StorageFactory, StorageContainer>::HybridMemoryTrie(AWordIndex * const p_word_index)
            : ATrie<N>(p_word_index, [&] (const TWordId wordId, const TContextId ctxId, const TModelLevel level) {

                return this->getContextId(wordId, ctxId, level); }), m_storage_factory(NULL) {
                //Check for the storage memory sized. This one is needed to be able to store
                //N-gram probabilities in the C type container as its value! See description
                //of the m_mgram_mapping data member.
                const size_t float_size = sizeof (TLogProbBackOff);
                const size_t idx_size = sizeof (TIndexSize);
                if (float_size != idx_size) {
                    stringstream msg;
                    msg << "Unable to use " << __FILE__ << " for a trie as it expects ( sizeof(TLogProbBackOff) = "
                            << float_size << " ) == ( sizeof(TIndexSize) = " << idx_size << ")!";
                    throw Exception(msg.str());
                }

                //This one is needed for having a proper non-null word index pointer.
                if (p_word_index == NULL) {
                    stringstream msg;
                    msg << "Unable to use " << __FILE__ << ", the word index pointer must not be NULL!";
                    throw Exception(msg.str());
                }

                //Initialize the array of counters
                memset(next_ctx_id, 0, NUM_IDX_COUNTERS * sizeof (TIndexSize));
            }

            template<TModelLevel N, template<TModelLevel > class StorageFactory, class StorageContainer>
            void HybridMemoryTrie<N, StorageFactory, StorageContainer>::preAllocate(const size_t counts[N]) {
                //Store the number of words plus 2 because a word with index 0 is
                //UNDEFINED and a word with index 1 is UNKNOWN (<unk>)
                m_word_arr_size = counts[0] + 2;

                //01) Pre-allocate the word index
                ATrie<N>::getWordIndex()->reserve(counts[0]);

                //02) Allocate the factory
                m_storage_factory = new StorageFactory<N>(counts);

                //03) Allocate the main arrays of pointers where probs/back-offs will be stored

                //First allocate the memory for the One-grams, add an extra
                //element for the unknown word and initialize it!
                m_mgram_data[0] = new TProbBackOffEntryPair[m_word_arr_size];
                memset(m_mgram_data[0], 0, m_word_arr_size * sizeof (TProbBackOffEntryPair));

                //Record the dummy probability and back-off values for the unknown word
                TProbBackOffEntryPair & pbData = m_mgram_data[0][UNKNOWN_WORD_ID];
                pbData.prob = MINIMAL_LOG_PROB_WEIGHT;
                pbData.back_off = UNDEFINED_LOG_PROB_WEIGHT;

                //Allocate more memory for probabilities and back off weight for
                //the remaining M-gram levels until M < N. For M==N there is no
                //back-off weights and thus we will store the probabilities just
                //Inside the C container class values.
                for (int idx = 1; idx < (N - 1); idx++) {
                    m_mgram_data[idx] = new TProbBackOffEntryPair[counts[idx]];
                    memset(m_mgram_data[idx], 0, counts[idx] * sizeof (TProbBackOffEntryPair));
                }

                //04) Allocate the word map arrays per level There is N-1 levels to have 
                //as the for M == 0 - the One Grams, we do not need this mappings
                for (int idx = 0; idx < (N - 1); idx++) {
                    m_mgram_mapping[idx] = new StorageContainer*[m_word_arr_size];
                    memset(m_mgram_mapping[idx], 0, m_word_arr_size * sizeof (StorageContainer*));
                }
            }

            template<TModelLevel N, template<TModelLevel > class StorageFactory, class StorageContainer>
            void HybridMemoryTrie<N, StorageFactory, StorageContainer>::add1Gram(const SRawNGram &oGram) {
                //First get the token/word from the 1-Gram
                const TextPieceReader & token = oGram.tokens[0];

                LOG_DEBUG << "Adding a 1-Gram: '" << token << "' to the Trie." << END_LOG;

                //Compute it's hash value
                TIndexSize wordId = ATrie<N>::getWordIndex()->makeId(token);
                //Get the word probability and back-off data reference
                TProbBackOffEntryPair & pbData = m_mgram_data[0][wordId];

                //Check that the probability data is not set yet, otherwise a warning!
                if (MONITORE_COLLISIONS && (pbData.prob != ZERO_LOG_PROB_WEIGHT)) {
                    //If the probability is not zero then this word has been already seen!
                    REPORT_COLLISION_WARNING(N, oGram, wordId, UNDEFINED_WORD_ID,
                            pbData.prob, pbData.back_off,
                            oGram.prob, oGram.back_off);
                }
                //Set/Update the probability and back-off values for the word
                pbData.prob = oGram.prob;
                pbData.back_off = oGram.back_off;

                LOG_DEBUG1 << "Inserted the (prob,back-off) data ("
                        << pbData.prob << "," << pbData.back_off << ") for "
                        << tokensToString<N>(oGram.tokens, oGram.level) << " wordHash = "
                        << wordId << END_LOG;
            }

            template<TModelLevel N, template<TModelLevel > class StorageFactory, class StorageContainer>
            void HybridMemoryTrie<N, StorageFactory, StorageContainer>::addMGram(const SRawNGram &mGram) {
                const TModelLevel level = mGram.level;
                LOG_DEBUG << "Adding a " << level << "-Gram " << tokensToString<N>(mGram.tokens, mGram.level) << " to the Trie" << END_LOG;

                //Check that this is not an 1-Gram or N-Gram for those we need another method!
                if ((MIN_NGRAM_LEVEL < level) || (level < N)) {
                    //To add the new N-gram (e.g.: w1 w2 w3 w4) data inserted, we need to:

                    // 1. Compute the context hash defined by w1 w2 w3, although the getContextId returns TContextSize
                    //    in this class we work with TIndexSize ids only, so it is safe to cast it to TIndexSize
                    const TIndexSize ctxId = (TIndexSize) ATrie<N>::template getContextId<DebugLevel::DEBUG2>(mGram);

                    // 2. Compute the hash of w4
                    const TextPieceReader & endWord = mGram.tokens[level - 1];
                    const TIndexSize wordId = ATrie<N>::getWordIndex()->getId(endWord.str());
                    LOG_DEBUG2 << "wordHash = getId('" << endWord.str() << "') = " << wordId << END_LOG;

                    // 3. Insert the probability data into the trie
                    StorageContainer*& ctx_mapping = m_mgram_mapping[level - MGRAM_MAPPING_IDX_OFFSET][wordId];
                    if (ctx_mapping == NULL) {
                        ctx_mapping = m_storage_factory->create(level);
                        LOG_DEBUG3 << "A new ACtxToPBStorage container is allocated for level " << level << END_LOG;
                    }
                    TIndexSize & ctx_id = (*ctx_mapping)[ctxId];
                    ctx_id = next_ctx_id[level - MGRAM_MAPPING_IDX_OFFSET]++;
                    TProbBackOffEntryPair& pbData = m_mgram_data[level - 1][ctx_id];

                    //Check that the probability data is not set yet, otherwise a warning!
                    if (MONITORE_COLLISIONS && (pbData.prob != ZERO_LOG_PROB_WEIGHT)) {
                        //If the probability is not zero then this word has been already seen!
                        REPORT_COLLISION_WARNING(N, mGram, wordId, ctxId,
                                pbData.prob, pbData.back_off,
                                mGram.prob, mGram.back_off);
                    }

                    //Set/Update the probability and back-off values for the word
                    pbData.prob = mGram.prob;
                    pbData.back_off = mGram.back_off;

                    LOG_DEBUG1 << "Inserted the (prob,back-off) data ("
                            << pbData.prob << "," << pbData.back_off << ") for "
                            << tokensToString<N>(mGram.tokens, mGram.level) << " contextHash = "
                            << ctxId << ", wordHash = " << wordId << END_LOG;
                } else {
                    stringstream msg;
                    msg << "Internal error: The " << level << "-Grams are to be handled with another add method!";
                    throw Exception(msg.str());
                }
            }

            template<TModelLevel N, template<TModelLevel > class StorageFactory, class StorageContainer>
            void HybridMemoryTrie<N, StorageFactory, StorageContainer>::addNGram(const SRawNGram &nGram) {
                const size_t level = nGram.level;
                LOG_DEBUG << "Adding a " << level << "-Gram " << tokensToString<N>(nGram.tokens, nGram.level) << " to the Trie" << END_LOG;

                //To add the new N-gram (e.g.: w1 w2 w3 w4) data inserted, we need to:

                // 1. Compute the context hash defined by w1 w2 w3, although the getContextId returns TContextSize
                //    in this class we work with TIndexSize ids only, so it is safe to cast it to TIndexSize
                const TIndexSize ctxId = (TIndexSize) ATrie<N>::template getContextId<DebugLevel::DEBUG2>(nGram);

                // 2. Compute the hash of w4
                const TextPieceReader & endWord = nGram.tokens[level - 1];
                const TIndexSize wordId = ATrie<N>::getWordIndex()->getId(endWord.str());
                LOG_DEBUG2 << "wordHash = getId('" << endWord << "') = " << wordId << END_LOG;

                // 3. Insert the probability data into the trie
                StorageContainer*& ctx_mapping = m_mgram_mapping[level - MGRAM_MAPPING_IDX_OFFSET][wordId];
                if (ctx_mapping == NULL) {
                    ctx_mapping = m_storage_factory->create(level);
                    LOG_DEBUG3 << "A new ACtxToPBStorage container is allocated for level " << level << END_LOG;
                }
                TLogProbBackOff & pData = (TLogProbBackOff &) (*ctx_mapping)[ctxId];

                //Check that the probability data is not set yet, otherwise a warning!
                if (MONITORE_COLLISIONS && (pData != ZERO_LOG_PROB_WEIGHT)) {
                    //If the probability is not zero then this word has been already seen!
                    REPORT_COLLISION_WARNING(N, nGram, wordId, ctxId,
                            pData, UNDEFINED_LOG_PROB_WEIGHT,
                            nGram.prob, UNDEFINED_LOG_PROB_WEIGHT);
                }

                //Set/Update the probability
                pData = nGram.prob;

                LOG_DEBUG1 << "Inserted the prob. data (" << pData << ") for "
                        << tokensToString<N>(nGram.tokens, nGram.level) << " contextHash = "
                        << ctxId << ", wordHash = " << wordId << END_LOG;
            }

            template<TModelLevel N, template<TModelLevel > class StorageFactory, class StorageContainer>
            void HybridMemoryTrie<N, StorageFactory, StorageContainer>::queryNGram(const vector<string> & ngram, SProbResult & result) {
                //ToDo: Implement
                throw Exception("Not implemented: HybridMemoryTrie<N, StorageFactory, StorageContainer>::queryNGram(const vector<string> & ngram, SProbResult & result)");
            }

            template<TModelLevel N, template<TModelLevel > class StorageFactory, class StorageContainer>
            HybridMemoryTrie<N, StorageFactory, StorageContainer>::~HybridMemoryTrie() {
                //Delete the probability and back-off data
                for (TModelLevel idx = 0; idx < (N - 1); idx++) {
                    //Delete the prob/back-off arrays per level
                    if (m_mgram_data[idx] != NULL) {
                        delete[] m_mgram_data[idx];
                    }
                }
                //Delete the mapping data
                for (TModelLevel idx = 0; idx < (N - 1); idx++) {
                    //Delete the word arrays per level
                    if (m_mgram_mapping[idx] != NULL) {
                        for (TIndexSize widx = 0; widx < m_word_arr_size; widx++) {
                            //Delete the C containers per word index
                            if (m_mgram_mapping[idx][widx] != NULL) {
                                delete m_mgram_mapping[idx][widx];
                            }
                        }
                        delete[] m_mgram_mapping[idx];
                    }
                }
                if (m_storage_factory != NULL) {
                    delete m_storage_factory;
                }
            }

            //Make sure that there will be templates instantiated, at least for the given parameter values
            template class HybridMemoryTrie<MAX_NGRAM_LEVEL, CtxToPBUnorderedMapStorageFactory, CtxToPBUnorderedMapStorage>;
        }
    }
}
