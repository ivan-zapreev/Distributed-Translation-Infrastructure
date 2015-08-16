/* 
 * File:   ContextMultiHashMapTrie.hpp
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
 * Created on August 14, 2015, 1:53 PM
 */

#ifndef CONTEXTMULTIHASHMAPTRIE_HPP
#define	CONTEXTMULTIHASHMAPTRIE_HPP


/**
 * We actually have several choices:
 * 
 * Continue to use <ext/hash_map.h> and use -Wno-deprecated to stop the warning
 * 
 * Use <tr1/unordered_map> and std::tr1::unordered_map
 * 
 * Use <unordered_map> and std::unordered_map and -std=c++0x
 * 
 * We will need to test which one runs better, it is an unordered_map for now.
 * http://www.cplusplus.com/reference/unordered_map/unordered_map/
 */
#include <utility>        // std::pair, std::make_pair
#include <unordered_map>  // std::unordered_map

#include "AHashMapTrie.hpp"
#include "Globals.hpp"
#include "HashingUtils.hpp"
#include "Logger.hpp"

using namespace std;
using namespace uva::smt::hashing;
using namespace uva::smt::logging;

namespace uva {
    namespace smt {
        namespace tries {

            /**
             * This is a HashMpa based ITrie interface implementation class.
             * Note 1: This implementation uses the unsigned long for the hashes it is not optimal
             * Note 2: the unordered_map might be not as efficient as a hash_map with respect to memory usage but it is supposed to be faster
             * 
             * This implementation is chosen because it resembles the ordered array implementation from:
             *      "Faster and Smaller N -Gram Language Models"
             *      Adam Pauls Dan Klein
             *      Computer Science Division
             *      University of California, Berkeley
             * 
             * and unordered_maps showed good performance in:
             *      "Efficient in-memory data structures for n-grams indexing"
             *       Daniel Robenek, Jan Platoˇs, and V ́aclav Sn ́aˇsel
             *       Department of Computer Science, FEI, VSB – Technical University of Ostrava
             *       17. listopadu 15, 708 33, Ostrava-Poruba, Czech Republic
             *       {daniel.robenek.st, jan.platos, vaclav.snasel}@vsb.cz
             * 
             */
            template<TModelLevel N>
            class ContextMultiHashMapTrie : public AHashMapTrie<N> {
            public:
                //Stores the offset for the MGram index, this is the number of M-gram levels stored elsewhere
                static const TModelLevel MGRAM_IDX_OFFSET = 2;

                /**
                 * The basic class constructor, accepts memory factors that are the
                 * coefficients used when pre-allocating memory for unordered maps.
                 * 
                 * If a factor is equal to 0.0 then no memory is pre-allocated.
                 * If the factor is equal to 1.0 then there is only as much preallocated
                 * as needed to store the gram entries. The latter is typically not enough
                 * as unordered_map needs more memory for internal administration.
                 * If there is not enough memory pre-allocated then additional allocations
                 * will take place but it does not alway lead to more efficient memory
                 * usage. The observed behavior is that it is better to pre-allocate
                 * a bit more memory beforehand, than needed. This leads to less
                 * memory consumption. Depending on the type of unordered_map
                 * key/value pair types the advised factor values are from 2.0 to 2.6.
                 * Because it can not be optimally determined beforehand, these are made
                 * constructor parameters so that they can be configured by the used.
                 * This breaks encapsulation a bit, exposing the internals, but
                 * there is no other better way, for fine tuning the memory usage.
                 * 
                 * @param  wordIndexMemFactor the assigned memory factor for
                 * storage allocation in the unordered_map used for the word index
                 * @param  oGramMemFactor The One-Gram memory factor needed for
                 * the greedy allocator for the unordered_map
                 * @param  mGramMemFactor The M-Gram memory factor needed for
                 * the greedy allocator for the unordered_map
                 * @param  nGramMemFactor The N-Gram memory factor needed for
                 * the greedy allocator for the unordered_map
                 */
                explicit ContextMultiHashMapTrie(const float _wordIndexMemFactor,
                        const float _oGramMemFactor,
                        const float _mGramMemFactor,
                        const float _nGramMemFactor);

                /**
                 * This method can be used to provide the N-gram count information
                 * That should allow for pre-allocation of the memory
                 * For more details @see ITrie
                 */
                virtual void preAllocate(const size_t counts[N]);

                /**
                 * This method adds a 1-Gram (word) to the trie.
                 * For more details @see ITrie
                 */
                virtual void add1Gram(const SBackOffNGram &oGram);

                /**
                 * This method adds a M-Gram (word) to the trie where 1 < M < N
                 * For more details @see ITrie
                 */
                virtual void addMGram(const SBackOffNGram &mGram);

                /**
                 * This method adds a N-Gram (word) to the trie where
                 * For more details @see ITrie
                 */
                virtual void addNGram(const SBackOffNGram &nGram);

                /**
                 * This method will get the N-gram in a form of a vector, e.g.:
                 *      [word1 word2 word3 word4 word5]
                 * and will compute and return the Language Model Probability for it
                 * For more details @see ITrie
                 */
                virtual void queryNGram(const vector<string> & ngram, SProbResult & result);

                virtual ~ContextMultiHashMapTrie();

            private:
                //The One-Gram memory factor needed for the greedy allocator for the unordered_map
                const float oGramMemFactor;
                //The M-Gram memory factor needed for the greedy allocator for the unordered_map
                const float mGramMemFactor;
                //The N-Gram memory factor needed for the greedy allocator for the unordered_map
                const float nGramMemFactor;

                //The type of key,value pairs to be stored in the One Grams map
                typedef pair< const TWordHashSize, TProbBackOffEntryPair> TOneGramEntry;
                //The typedef for the One Grams map allocator
                typedef GreedyMemoryAllocator< TOneGramEntry > TOneGramAllocator;
                //The One Grams map type
                typedef unordered_map<TWordHashSize, TProbBackOffEntryPair, std::hash<TWordHashSize>, std::equal_to<TWordHashSize>, TOneGramAllocator > TOneGramsMap;
                //The actual data storage for the One Grams
                TOneGramAllocator * pOneGramAlloc;
                //The map storing the One-Grams: I.e. the word indexes and the word probabilities.
                //NOTE: Using an array here in place of an unordered hash map gave 
                //some 25 Mb reduction on a 20 Gb model ... this is negligible. As
                //the memory statistics is not accurate that could just be noise! Also,
                //I see that there are typically not so many 1-Grams an plenty of 5-grams
                TOneGramsMap * pOneGramMap;

                //The type of key,value pairs to be stored in the M Grams map
                typedef pair< const TReferenceHashSize, TProbBackOffEntryPair> TMGramEntry;
                //The typedef for the M Grams map allocator
                typedef GreedyMemoryAllocator< TMGramEntry > TMGramAllocator;
                //The N Grams map type
                typedef unordered_map<TReferenceHashSize, TProbBackOffEntryPair, std::hash<TReferenceHashSize>, std::equal_to<TReferenceHashSize>, TMGramAllocator > TMGramsMap;
                //The actual data storage for the M Grams for 1 < M < N
                TMGramAllocator * pMGramAlloc[N - MGRAM_IDX_OFFSET];
                //The array of maps map storing M-grams for 1 < M < N
                TMGramsMap * pMGramMap[N - MGRAM_IDX_OFFSET];

                //The type of key,value pairs to be stored in the N Grams map
                typedef pair< const TReferenceHashSize, TLogProbBackOff> TNGramEntry;
                //The typedef for the N Grams map allocator
                typedef GreedyMemoryAllocator< TNGramEntry > TNGramAllocator;
                //The N Grams map type
                typedef unordered_map<TReferenceHashSize, TLogProbBackOff, std::hash<TReferenceHashSize>, std::equal_to<TReferenceHashSize>, TNGramAllocator > TNGramsMap;
                //The actual data storage for the N Grams
                TNGramAllocator * pNGramAlloc;
                //The map storing the N-Grams, they do not have back-off values
                TNGramsMap * pNGramMap;

                /**
                 * The copy constructor, is made private as we do not intend to copy this class objects
                 * @param orig the object to copy from
                 */
                ContextMultiHashMapTrie(const ContextMultiHashMapTrie& orig)
                : AHashMapTrie<N>(0.0), oGramMemFactor(0.0), mGramMemFactor(0.0), nGramMemFactor(0.0) {
                    throw Exception("ContextMultiHashMapTrie copy constructor must not be used, unless implemented!");
                };

                /**
                 * This recursive function implements the computation of the
                 * N-Gram probabilities in the Back-Off Language Model. The
                 * N-Gram hashes are obtained from the _wordHashes member
                 * variable of the class. So it must be pre-set with proper
                 * word hash values first!
                 * @param contextLength this is the length of the considered context.
                 * @return the computed probability value
                 */
                TLogProbBackOff computeLogProbability(const TModelLevel contextLength);

                /**
                 * This recursive function allows to get the back-off weight for the current context.
                 * The N-Gram hashes are obtained from the pre-computed data memeber array _wordHashes
                 * @param contextLength the current context length
                 * @return the resulting back-off weight probability
                 */
                TLogProbBackOff getBackOffWeight(const TModelLevel contextLength);

                /**
                 * This method must used to provide the N-gram count information
                 * That should allow for pre-allocation of the memory
                 * @param counts the counts for the number of elements of each gram level
                 */
                void preAllocateOGrams(const size_t counts[N]);

                /**
                 * This method must used to provide the N-gram count information
                 * That should allow for pre-allocation of the memory
                 * @param counts the counts for the number of elements of each gram level
                 */
                void preAllocateMGrams(const size_t counts[N]);

                /**
                 * This method must used to provide the N-gram count information
                 * That should allow for pre-allocation of the memory
                 * @param counts the counts for the number of elements of each gram level
                 */
                void preAllocateNGrams(const size_t counts[N]);

            };

            typedef ContextMultiHashMapTrie<MAX_NGRAM_LEVEL> TFiveContextMultiHashMapTrie;
        }
    }
}
#endif	/* CONTEXTMULTIHASHMAPTRIE_HPP */

