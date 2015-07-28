/* 
 * File:   NGramBuilder.cpp
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
 * Created on April 18, 2015, 12:02 PM
 */
#include "ARPANGramBuilder.hpp"

#include <string>  //std::string
#include <vector>  //std::vector

#include "Logger.hpp"
#include "StringUtils.hpp"

using namespace uva::smt::logging;
using namespace uva::smt::utils::text;

namespace uva {
    namespace smt {
        namespace tries {
            namespace arpa {

                template<TModelLevel N, bool doCache>
                ARPANGramBuilder<N, doCache>::ARPANGramBuilder(ATrie<N, doCache> & trie, const char delim) : _trie(trie), _delim(delim) {
                }

                template<TModelLevel N, bool doCache>
                ARPANGramBuilder<N, doCache>::ARPANGramBuilder(const ARPANGramBuilder<N, doCache>& orig) : _trie(orig._trie), _delim(orig._delim) {
                }

                template<TModelLevel N, bool doCache>
                ARPANGramBuilder<N, doCache>::~ARPANGramBuilder() {
                }

                template<TModelLevel N, bool doCache>
                void ARPANGramBuilder<N, doCache>::processString(const string & data) {
                    //Tokenise the line of text into a vector first
                    vector<string> tokens;
                    tokenize(data, _delim, tokens);

                    //First add all the words to the trie
                    _trie.addWords(tokens);

                    //Create and record all of the N-grams starting from 2 and 
                    //limited either by Trie or by the available number of Tokens
                    const TModelLevel ngLevel = min<unsigned int>(_trie.getNGramLevel(), tokens.size());
                    LOG_DEBUG << "N-gram level = " << ngLevel << END_LOG;
                    for (int n = 2; n <= ngLevel; n++) {
                        for (int idx = 0; idx <= (tokens.size() - n); idx++) {
                            LOG_DEBUG << "adding N-grams (#tokens=" << tokens.size() << ") idx = " << idx << ", len = " << n << END_LOG;
                            _trie.addNGram(tokens, idx, n);
                        }
                    }
                }

                //Make sure that there will be templates instantiated, at least for the given parameter values
                template class ARPANGramBuilder<N_GRAM_PARAM, true>;
                template class ARPANGramBuilder<N_GRAM_PARAM, false>;
            }
        }
    }
}