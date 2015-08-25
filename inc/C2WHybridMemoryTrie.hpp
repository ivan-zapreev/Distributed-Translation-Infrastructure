/* 
 * File:   ContextToWordHybridMemoryTrie.hpp
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
 * Created on August 25, 2015, 11:10 AM
 */

#ifndef C2WHYBRIDMEMORYTRIE_HPP
#define	C2WHYBRIDMEMORYTRIE_HPP

#include <string>   // std::string

#include "Globals.hpp"
#include "Logger.hpp"
#include "ATrie.hpp"
#include "AWordIndex.hpp"

using namespace std;
using namespace uva::smt::tries::dictionary;

namespace uva {
    namespace smt {
        namespace tries {
            
            

            /**
             * This is the hybrid memory trie implementation class.
             * It implements storage by using maps and arrays and for a M-gram
             * with 1 < M < N it maps previous context to the word/proba/back-off data
             * @param N the maximum number of levelns in the trie.
             * @param C the container class to store context-to-prob_back_off_index pairs, must derive from ACtxToPBStorare
             */
            template<TModelLevel N>
            class C2WHybridMemoryTrie : public ATrie<N> {
            public:

                /**
                 * The basic constructor
                 * @param p_word_index the word index (dictionary) container
                 */
                explicit C2WHybridMemoryTrie(AWordIndex * const p_word_index);

                /**
                 * This method can be used to provide the N-gram count information
                 * That should allow for pre-allocation of the memory
                 * For more details @see ATrie
                 */
                virtual void preAllocate(const size_t counts[N]);

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
                virtual ~C2WHybridMemoryTrie();

            protected:

                /**
                 * Allows to retrieve the data storage structure for the One gram with the given Id.
                 * If the storage structure does not exist, return a new one.
                 * For more details @see ATrie
                 */
                virtual TProbBackOffEntryPair & make_1_GramDataRef(const TShortId wordId) {
                    throw Exception("TProbBackOffEntryPair & make_1_GramDataRef(const TWordId wordId)");
                };

                /**
                 * Allows to retrieve the data storage structure for the M gram
                 * with the given M-gram level Id. M-gram context and last word Id.
                 * If the storage structure does not exist, return a new one.
                 * For more details @see ATrie
                 */
                virtual TProbBackOffEntryPair& make_M_GramDataRef(const TModelLevel level, const TShortId wordId, const TLongId ctxId) {
                    throw Exception("TProbBackOffEntryPair& make_M_GramDataRef(const TModelLevel level, const TWordId wordId, const TContextId ctxId)");
                };

                /**
                 * Allows to retrieve the data storage structure for the N gram.
                 * Given the N-gram context and last word Id.
                 * If the storage structure does not exist, return a new one.
                 * For more details @see ATrie
                 */
                virtual TLogProbBackOff& make_N_GramDataRef(const TShortId wordId, const TLongId ctxId) {
                    throw Exception("TLogProbBackOff& make_N_GramDataRef(const TWordId wordId, const TContextId ctxId)");
                };

            private:
                //The offset, relative to the M-gram level M for the mgram mapping array index
                const static TModelLevel MGRAM_MAPPING_IDX_OFFSET = 2;

                //Will store the next context index counters per M-gram level
                //for 1 < M < N.
                const static TModelLevel NUM_IDX_COUNTERS = N - 2;
                TShortId next_ctx_id[NUM_IDX_COUNTERS];

                /**
                 * Computes the N-Gram context using the previous context and the current word id
                 * 
                 * WARNING: Must only be called for the M-gram level M > 1!
                 * 
                 * @param wordId the current word id
                 * @param ctxId the previous context id
                 * @param level the M-gram level we are working with M
                 * @return the resulting context
                 */
                inline TLongId getContextId(TShortId wordId, TLongId ctxId, const TModelLevel level) {
                    throw Exception("TContextId getContextId(TWordId wordId, TContextId ctxId, const TModelLevel level)");
                }

            };

            typedef C2WHybridMemoryTrie<MAX_NGRAM_LEVEL> TFiveC2WMapHybridMemoryTrie;
        }
    }
}


#endif	/* CONTEXTTOWORDHYBRIDMEMORYTRIE_HPP */

