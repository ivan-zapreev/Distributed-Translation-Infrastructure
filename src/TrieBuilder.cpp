/* 
 * File:   TrieBuilder.cpp
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
 * Created on April 18, 2015, 11:58 AM
 */

#include "TrieBuilder.hpp"

#include "Logger.hpp"
#include "NGramBuilder.hpp"
#include "Globals.hpp"

namespace tries {

    using ngrams::NGramBuilder;
    
    template<TTrieSize N, bool doCache>
    TrieBuilder<N,doCache>::TrieBuilder(ATrie<N,doCache> & trie, ifstream & fstr, const char delim) : _trie(trie), _fstr(fstr),_delim(delim){
    }

    template<TTrieSize N, bool doCache>
    TrieBuilder<N,doCache>::TrieBuilder(const TrieBuilder<N,doCache>& orig) : _trie(orig._trie), _fstr(orig._fstr), _delim(orig._delim) {
    }

    template<TTrieSize N, bool doCache>
    TrieBuilder<N,doCache>::~TrieBuilder() {
    }

    template<TTrieSize N, bool doCache>
    void TrieBuilder<N,doCache>::build() {
        LOGGER(Logger::DEBUG) << "Starting to read the file and build the trie ..." << endl;
        
        //Initialize the NGram builder and give it the trie as an argument
        NGramBuilder<N,doCache> ngBuilder(_trie,_delim);

        //Do the progress bard indicator
        Logger::startProgressBar();
        
        //Iterate through the file and build n-grams per line and fill in the trie
        string line;
        while( getline(_fstr, line) )
        {
            LOGGER(Logger::DEBUG) << line << endl;
            ngBuilder.processString(line);
            Logger::updateProgressBar();
        }

        Logger::stopProgressBar();

        LOGGER(Logger::DEBUG) << "Done reading the file and building the trie." << endl;
    }
    
    //Make sure that there will be templates instantiated, at least for the given parameter values
    template class TrieBuilder< N_GRAM_PARAM,true >;
    template class TrieBuilder< N_GRAM_PARAM,false >;
}


