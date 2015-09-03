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

using namespace std;
using namespace uva::smt::logging;
using namespace uva::smt::file;
using namespace uva::smt::tries::dictionary;

namespace uva {
    namespace smt {
        namespace tries {

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
                : ATrie<N>(_pWordIndex) {
                }

                /**
                 * This method can be used to provide the N-gram count information
                 * That should allow for pre-allocation of the memory
                 * @see ATrie
                 */
                virtual void preAllocate(const size_t counts[N]) {
                    //Call the base-class
                    ATrie<N>::preAllocate(counts);
                    
                    //ToDo: Implement
                    throw Exception("Pre-allocate the trie levels");
                };

                /**
                 * This method adds a 1-Gram (word) to the trie.
                 * It it snot guaranteed that the parameter will be checked to be a 1-Gram!
                 * @see ATrie
                 */
                virtual void add_1_Gram(const T_M_Gram &oGram) {
                    //ToDo: Implement
                    throw Exception("add_1_Gram");
                };

                /**
                 * This method adds a M-Gram (word) to the trie where 1 < M < N
                 * @see ATrie
                 */
                virtual void add_M_Gram(const T_M_Gram &mGram) {
                    //ToDo: Implement
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
                };

            protected:
            private:
            };
        }
    }
}


#endif	/* G2DHASHMAPTRIE_HPP */

