/* 
 * File:   C2WOrderedArrayTrie.hpp
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

#ifndef C2WORDEREDARRAYTRIE_HPP
#define	C2WORDEREDARRAYTRIE_HPP

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
             * This structure stores two things the word id
             * and the corresponding probability/back-off data.
             * @param wordId the word id
             * @param data the back-off and probability data
             */
            typedef struct {
                TShortId wordId;
                TProbBackOffEntryPair data;
            } TWordIdProbBackOffEntryPair;

            /**
             * This structure is needed to store begin and end index to reference pieces of an array
             * @param begin_idx the begin index
             * @param end_idx the end index
             */
            typedef struct {
                TShortId begin_idx;
                TShortId end_idx;
            } TSubArrReference;

            /**
             * Stores the information about the context it word id and corresponding probability
             * This data structure is to be used for the N-Gram data, as there are no back-offs
             */
            typedef struct {
                TShortId ctx_id;
                TShortId word_id;
                TLogProbBackOff prob;
            } TCtxIdProbEntryPair;

            /**
             * This is the hybrid memory trie implementation class.
             * It implements storage by using maps and arrays and for a M-gram
             * with 1 < M < N it maps previous context to the word/proba/back-off data.
             * 
             * WARNING: This trie assumes that the N-grams are added to the Trie in an
             * ordered way and there are no duplicates in the 1-Grams. The order is
             * assumed to be lexicographical as in the ARPA files! This is not checked!
             * 
             * @param N the maximum number of levelns in the trie.
             * @param C the container class to store context-to-prob_back_off_index pairs, must derive from ACtxToPBStorare
             */
            template<TModelLevel N>
            class C2WOrderedArrayTrie : public ATrie<N> {
            public:

                /**
                 * The basic constructor
                 * @param p_word_index the word index (dictionary) container
                 */
                explicit C2WOrderedArrayTrie(AWordIndex * const p_word_index);

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
                virtual ~C2WOrderedArrayTrie();

            protected:

                /**
                 * Allows to retrieve the data storage structure for the One gram with the given Id.
                 * If the storage structure does not exist, return a new one.
                 * For more details @see ATrie
                 */
                virtual TProbBackOffEntryPair & make_1_GramDataRef(const TShortId wordId) {
                    return m_1_gram_data[wordId];
                };

                /**
                 * Allows to retrieve the data storage structure for the M gram
                 * with the given M-gram level Id. M-gram context and last word Id.
                 * If the storage structure does not exist, return a new one.
                 * For more details @see ATrie
                 */
                virtual TProbBackOffEntryPair& make_M_GramDataRef(const TModelLevel level, const TShortId wordId, const TLongId ctxId) {
                    //Compute the m-gram index
                    const TModelLevel mgram_idx = level - MGRAM_IDX_OFFSET;

                    //First get the sub-array reference. 
                    TSubArrReference & ref = m_M_gram_ctx_2_data[mgram_idx][ctxId];

                    //Get the new index and increment - this will be the new end index
                    ref.end_idx = m_MN_gram_idx_cnts[mgram_idx]++;

                    //Check if we exceeded the maximum allowed number of M-grams
                    if (ref.end_idx >= m_MN_gram_size[mgram_idx]) {
                        stringstream msg;
                        msg << "The maximum allowed number of " << SSTR(level) << "-grams: " << SSTR(m_MN_gram_size[mgram_idx]) << " is exceeded )!";
                        throw Exception(msg.str());
                    }

                    //Check if there are already elements for this context
                    if (ref.begin_idx == UNDEFINED_ARR_IDX) {
                        //There was no elements put into this contex, the begin index is then equal to the end index
                        ref.begin_idx = ref.end_idx;
                    }

                    //Store the word id
                    m_M_gram_data[mgram_idx][ref.end_idx].wordId = wordId;

                    //Return the reference to the newly allocated element
                    return m_M_gram_data[mgram_idx][ref.end_idx].data;

                    //there are elements in this context already
                    throw Exception("TProbBackOffEntryPair& make_M_GramDataRef(const TModelLevel level, const TWordId wordId, const TContextId ctxId)");
                };

                /**
                 * Allows to retrieve the data storage structure for the N gram.
                 * Given the N-gram context and last word Id.
                 * If the storage structure does not exist, return a new one.
                 * For more details @see ATrie
                 */
                virtual TLogProbBackOff& make_N_GramDataRef(const TShortId wordId, const TLongId ctxId) {
                    //Get the new n-gram index
                    const TShortId n_gram_idx = m_MN_gram_idx_cnts[N_GRAM_IDX]++;

                    //Check if we exceeded the maximum allowed number of M-grams
                    if (n_gram_idx >= m_MN_gram_size[N_GRAM_IDX]) {
                        stringstream msg;
                        msg << "The maximum allowed number of " << SSTR(N) << "-grams: " << SSTR(m_MN_gram_size[N_GRAM_IDX]) << " is exceeded )!";
                        throw Exception(msg.str());
                    }

                    //Store the context and word ids
                    m_N_gram_data[n_gram_idx].ctx_id = ctxId;
                    m_N_gram_data[n_gram_idx].word_id = wordId;
                    
                    //return the reference to the probability
                    return m_N_gram_data[n_gram_idx].prob;
                };

            private:
                //The offset, relative to the M-gram level M for the mgram mapping array index
                const static TModelLevel MGRAM_IDX_OFFSET = 2;

                //Will store the the number of M levels such that 1 < M < N.
                const static TModelLevel NUM_M_GRAM_LEVELS = N - MGRAM_IDX_OFFSET;

                //Will store the the number of M levels such that 1 < M < N.
                const static TModelLevel NUM_M_N_GRAM_LEVELS = N - 1;

                //Compute the N-gram index in m_MN_gram_size and  m_MN_gram_idx_cnts
                static const TModelLevel N_GRAM_IDX = N - MGRAM_IDX_OFFSET;

                // Stores the undefined index array value
                static const TShortId UNDEFINED_ARR_IDX = 0;

                //Stores the 1-gram data
                TProbBackOffEntryPair * m_1_gram_data;

                //Stores the M-gram context to data mappings for: 1 < M < N
                //This is a two dimensional array
                TSubArrReference * m_M_gram_ctx_2_data[NUM_M_GRAM_LEVELS];
                //Stores the M-gram data for the M levels: 1 < M < N
                //This is a two dimensional array
                TWordIdProbBackOffEntryPair * m_M_gram_data[NUM_M_GRAM_LEVELS];

                //Stores the N-gram data
                TCtxIdProbEntryPair * m_N_gram_data;

                //Stores the index counters per M-gram level: 1 < M <= N
                TShortId m_MN_gram_size[NUM_M_N_GRAM_LEVELS];
                //Stores the index counters per M-gram level: 1 < M <= N
                TShortId m_MN_gram_idx_cnts[NUM_M_N_GRAM_LEVELS];

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

            typedef C2WOrderedArrayTrie<MAX_NGRAM_LEVEL> TFiveC2WOrderedArrayTrie;
        }
    }
}


#endif	/* CONTEXTTOWORDHYBRIDMEMORYTRIE_HPP */

