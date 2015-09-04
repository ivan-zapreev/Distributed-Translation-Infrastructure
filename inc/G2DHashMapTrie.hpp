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
#include "TextPieceReader.hpp"
#include "AWordIndex.hpp"
#include "ATrie.hpp"

using namespace std;
using namespace uva::smt::logging;
using namespace uva::smt::file;
using namespace uva::smt::tries::dictionary;

namespace uva {
    namespace smt {
        namespace tries {

            /**
             * This class represents a dynamic M-Gram id.
             * It stores the byte-array that contains the id and its length.
             * All bytes in this array are meaningful.
             */
            class M_Gram_Id
            {
            public: 
                M_Gram_Id(){}
                virtual ~M_Gram_Id(){}
            private:
                //Stores the pinter to the id data
                uint8_t * id;
                //Stores the length of the id
                uint8_t len;
            };

            /**
             * This template structure is used for storing trie hash map elements
             * Each element contains and id of the m-gram and its payload -
             * the probability/back-off data, the latter is the template parameter
             */
            template<typename PAYLOAD_TYPE>
            struct STrieHashMapEntry {
                M_Gram_Id id;
                PAYLOAD_TYPE data;
            };

            /**
             * The wrapper structure for the templated structure above.
             * Just a technicality, to be able to access the type easily
             */
            template<typename PAYLOAD_TYPE>
            struct TTrieHashMapEntry {
                typedef STrieHashMapEntry<PAYLOAD_TYPE> type;
            };

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
                explicit G2DHashMapTrie(AWordIndex * const _pWordIndex)
                : ATrie<N>(_pWordIndex), m_1_gram_data(NULL) {
                    //Initialize the array of number of gram ids per level
                    memset(num_buckets, 0, N * sizeof (TShortId));
                }

                /**
                 * This method can be used to provide the N-gram count information
                 * That should allow for pre-allocation of the memory
                 * @see ATrie
                 */
                virtual void preAllocate(const size_t counts[N]) {
                    //Call the base-class
                    ALayeredTrie<N>::preAllocate(counts);

                    //02) Pre-allocate the 1-Gram data
                    num_buckets[0] = ATrie<N>::getWordIndex()->getTotalWordsCount(counts[0]);
                    m_1_gram_data = new TProbBackOffEntry[num_buckets[0]];
                    memset(m_1_gram_data, 0, num_buckets[0] * sizeof (TProbBackOffEntry));

                    //03) Insert the unknown word data into the allocated array
                    TProbBackOffEntry & pbData = m_1_gram_data[AWordIndex::UNKNOWN_WORD_ID];
                    pbData.prob = UNK_WORD_LOG_PROB_WEIGHT;
                    pbData.back_off = ZERO_BACK_OFF_WEIGHT;

                    //Copy the counts to the local storage
                    for (TModelLevel level = 1; level < N; level++) {
                        num_buckets[level] = counts[level];
                    }

                    //ToDo: Implement
                    throw Exception("Pre-allocate the trie levels");
                };

                /**
                 * This method adds a 1-Gram (word) to the trie.
                 * It it snot guaranteed that the parameter will be checked to be a 1-Gram!
                 * @see ATrie
                 */
                virtual void add_1_Gram(const T_M_Gram &oGram) {
                    //Register a new word, and the word id will be the one-gram id
                    const TShortId oneGramId = ATrie<N>::getWordIndex()->makeId(oGram.tokens[0]);
                    //Store the probability data in the one gram data storage, under its id
                    m_1_gram_data[oneGramId].prob = oGram.prob;
                    m_1_gram_data[oneGramId].back_off = oGram.back_off;
                };

                /**
                 * This method adds a M-Gram (word) to the trie where 1 < M < N
                 * @see ATrie
                 */
                virtual void add_M_Gram(const T_M_Gram &mGram) {
                    //Compute the hash value for the given M-gram, it must
                    //be the M-Gram id in the M-Gram data storage
                    const TShortId mGramHash = mGram.hash();
                    //Compute the bucket Id from the M-Gram hash
                    const TShortId bucketId = mGramHash % num_buckets[mGram.level];

                    //If the sanity check is on then check on that the id is within the range
                    if (DO_SANITY_CHECKS && ((bucketId < 0) || (bucketId >= num_buckets[mGram.level]))) {
                        stringstream msg;
                        msg << "The " << SSTR(mGram.level) << "-gram: " << tokensToString<N>(mGram.tokens, mGram.level)
                                << " was given an incorrect hash: " << SSTR(bucketId)
                                << ", must be within [0, " << SSTR(num_buckets[mGram.level]) << "]";
                        throw Exception(msg.str());
                    }

                    //ToDo: Create a new M-Gram Id (unique) and then add it to the data

                    throw Exception("add_M_Gram");
                };

                /**
                 * This method adds a N-Gram (word) to the trie where
                 * It it not guaranteed that the parameter will be checked to be a N-Gram!
                 * @see ATrie
                 */
                virtual void add_N_Gram(const T_M_Gram &nGram) {
                    //ToDo: Implement
                    throw Exception("add_N_Gram");
                };

                /**
                 * This method will get the N-gram in a form of a vector, e.g.:
                 *      [word1 word2 word3 word4 word5]
                 * and will compute and return the Language Model Probability for it
                 * @see ATrie
                 */
                virtual void queryNGram(const T_M_Gram & ngram, TQueryResult & result) {
                    //ToDo: Implement
                    throw Exception("queryNGram");
                };

                /**
                 * The basic class destructor
                 */
                virtual ~G2DHashMapTrie() {
                    //Check that the one grams were allocated, if yes then the rest must have been either
                    if (m_1_gram_data != NULL) {
                        delete[] m_1_gram_data;
                    }
                };

            protected:


            private:

                //Stores the 1-gram data
                TProbBackOffEntry * m_1_gram_data;

                //Stores the number of gram ids/buckets per level
                TShortId num_buckets[N];
            };
        }
    }
}


#endif	/* G2DHASHMAPTRIE_HPP */

