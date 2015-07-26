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
#include "NGramBuilder.hpp"

#include "Logger.hpp"

namespace tries {
namespace ngrams {
    template<TTrieSize N, bool doCache>
    NGramBuilder<N,doCache>::NGramBuilder(ATrie<N,doCache> & trie, const char delim) : _trie(trie), _delim(delim) {
    }

    template<TTrieSize N, bool doCache>
    NGramBuilder<N,doCache>::NGramBuilder(const NGramBuilder<N,doCache>& orig) : _trie(orig._trie), _delim(orig._delim) {
    }

    template<TTrieSize N, bool doCache>
    NGramBuilder<N,doCache>::~NGramBuilder() {
    }

    template<TTrieSize N, bool doCache>
    void NGramBuilder<N,doCache>::processString(const string & data ) {
        //Tokenise the line of text into a vector first
        vector<string> tokens;
        tokenize(data, _delim, tokens);

        //First add all the words to the trie
        _trie.addWords(tokens);

        //Create and record all of the N-grams starting from 2 and 
        //limited either by Trie or by the available number of Tokens
        const TTrieSize ngLevel = min<unsigned int>(_trie.getNGramLevel(), tokens.size());
        LOGGER(Logger::DEBUG) << "N-gram level = " << ngLevel << endl;
        for(int n=2; n <= ngLevel; n++) {
            for(int idx=0; idx <= (tokens.size() - n); idx++){
                LOGGER(Logger::DEBUG) << "adding N-grams (#tokens=" << tokens.size() << ") idx = " << idx << ", len = " << n << endl;
                _trie.addNGram(tokens, idx, n );
            }
        }
    }
    
    //Make sure that there will be templates instantiated, at least for the given parameter values
    template class NGramBuilder<N_GRAM_PARAM,true>;
    template class NGramBuilder<N_GRAM_PARAM,false>;
}
}