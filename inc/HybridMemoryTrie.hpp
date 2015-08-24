/* 
 * File:   HybridMemoryTrie.hpp
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

#ifndef HYBRIDMEMORYTRIE_HPP
#define	HYBRIDMEMORYTRIE_HPP

#include <string>           // std::string

#include "Globals.hpp"
#include "Logger.hpp"
#include "ATrie.hpp"
#include "TextPieceReader.hpp"
#include "AWordIndex.hpp"
#include "HybridMemoryTrieStorage.hpp"

using namespace std;
using namespace uva::smt::tries::dictionary;

namespace uva {
    namespace smt {
        namespace tries {

            /**
             * This is the hybrid memory trie implementation class. It has two template parameters.
             * @param N the maximum number of levelns in the trie.
             * @param C the container class to store context-to-prob_back_off_index pairs, must derive from ACtxToPBStorare
             */
            template<TModelLevel N, template<TModelLevel > class StorageFactory, class StorageContainer>
            class HybridMemoryTrie : public ATrie<N> {
            public:

                /**
                 * The basic constructor
                 * @param p_word_index the word index (dictionary) container
                 */
                explicit HybridMemoryTrie(AWordIndex * const p_word_index);

                /**
                 * This method can be used to provide the N-gram count information
                 * That should allow for pre-allocation of the memory
                 * For more details @see ATrie
                 */
                virtual void preAllocate(const size_t counts[N]);

                /**
                 * This method adds a 1-Gram (word) to the trie.
                 * For more details @see ATrie
                 */
                virtual void add1Gram(const SRawNGram &oGram);

                /**
                 * This method adds a M-Gram (word) to the trie where 1 < M < N
                 * For more details @see ATrie
                 */
                virtual void addMGram(const SRawNGram &mGram);

                /**
                 * This method adds a N-Gram (word) to the trie where
                 * For more details @see ATrie
                 */
                virtual void addNGram(const SRawNGram &nGram);

                /**
                 * This method will get the N-gram in a form of a vector, e.g.:
                 *      [word1 word2 word3 word4 word5]
                 * and will compute and return the Language Model Probability for it
                 * For more details @see ATrie
                 */
                virtual void queryNGram(const vector<string> & ngram, SProbResult & result);

                /**
                 * The basic destructor
                 */
                virtual ~HybridMemoryTrie();

            private:
                //The offset, relative to the M-gram level M for the mgram mapping array index
                const static TModelLevel MGRAM_MAPPING_IDX_OFFSET = 2;

                //Stores the number of words
                size_t m_word_arr_size;

                //The factory to produce the storage containers
                StorageFactory<N> * m_storage_factory;

                //M-Gram data for 1 <= M < N. This is a 2D array storing
                //For each M-Gram level M an array of prob-back_off values
                // m_mgram_data[M][0] - probability/back-off pair for the given M-gram
                // m_mgram_data[M][1] --//--
                // ...
                // m_mgram_data[M][#M-Grams - 1] --//--
                // m_mgram_data[M][#M-Grams] --//--
                TProbBackOffEntryPair * m_mgram_data[N - 1];

                //M-Gram data for 1 < M <= N. This is a 2D array storing
                //For each M-Gram level M an array of #words elements of
                //pointers to C template parameter type:
                // m_mgram_mapping[M][0] -> NULL (if there is no M-gram ending with word 0 in the level)
                // m_mgram_mappin[M]g[1] -> C instance
                // ...
                // m_mgram_mapping[M][#words - 1] -> NULL
                // m_mgram_mapping[M][#words] -> C instance
                //
                //For all 1 < M < N instances of C contain mappings from the
                //context index to the index in the m_mgram_data[M] array -
                //the array storing probability/back-off values.
                //For M = N the pair value stored in the instance of C is the
                //probability itself! Note that that the probabilities are
                //stored as floats - 4 bytes and m_mgram_data[M] array is also a
                //4 byte integer, so we minimize memory usage by storing float
                //probability in place of the index.
                StorageContainer** m_mgram_mapping[N - 1];

                //Will store the next context index counters per M-gram level
                //for 1 < M < N.
                const static TModelLevel NUM_IDX_COUNTERS = N - 2;
                TIndexSize next_ctx_id[NUM_IDX_COUNTERS];

                /**
                 * This function gets the context id of the N-gram given by the tokens, e.g. [w1 w2 w3 w4]
                 * 
                 * WARNING: Must not be called on M-grams with M <= 1!
                 * 
                 * @param gram the N-gram with its tokens to create context for
                 * @return the resulting the context(w1 w2 w3)
                 */
                template<DebugLevel logLevel>
                inline TIndexSize getContextId(const SRawNGram & gram) {
                    TContextId ctxId;

                    //Try to retrieve the context from the cache, if not present then compute it
                    if (ATrie<N>::getCachedContextId(gram, ctxId)) {
                        //Get the start context value for the first token
                        const string & token = gram.tokens[0].str();

                        //There is no id cached for this M-gram context - compute it
                        ctxId = ATrie<N>::getWordIndex()->getId(token);
                        LOGGER(logLevel) << "ctxId = getId('" << token << "') = " << SSTR(ctxId) << END_LOG;

                        //Iterate and compute the hash:
                        for (int i = 1; i < (gram.level - 1); i++) {
                            const string & token = gram.tokens[i].str();
                            TWordId wordId = ATrie<N>::getWordIndex()->getId(token);
                            LOGGER(logLevel) << "wordId = getId('" << token << "') = " << SSTR(wordId) << END_LOG;
                            ctxId = m_mgram_mapping[i - 1][wordId]->at(ctxId);
                            LOGGER(logLevel) << "ctxId = contextId( wordId, ctxId ) = " << SSTR(ctxId) << END_LOG;
                        }

                        //Cache the newly computed context id for the given n-gram context
                        ATrie<N>::cacheContextId(gram, ctxId);
                    }

                    //The ids we work with here are of index size - smaller than general purpose context ids
                    return (TIndexSize) ctxId;
                }
            };

            typedef HybridMemoryTrie<MAX_NGRAM_LEVEL, CtxToPBMapStorageFactory, CtxToPBMapStorage> TFiveMapHybridMemoryTrie;
        }
    }
}

#endif	/* HYBRIDMEMORYTRIE_HPP */

