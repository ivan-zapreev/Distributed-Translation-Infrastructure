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
#include <iostream>
#include <string>

#include "ARPATrieBuilder.hpp"

#include "Logger.hpp"
#include "NGramBuilder.hpp"
#include "Globals.hpp"

using namespace uva::smt::logging;
using uva::smt::tries::ngrams::NGramBuilder;

namespace uva {
    namespace smt {
        namespace tries {

            template<TTrieSize N, bool doCache>
            ARPATrieBuilder<N, doCache>::ARPATrieBuilder(ATrie<N, doCache> & trie, ifstream & fstr, const char delim) : _trie(trie), _fstr(fstr), _delim(delim) {
            }

            template<TTrieSize N, bool doCache>
            ARPATrieBuilder<N, doCache>::ARPATrieBuilder(const ARPATrieBuilder<N, doCache>& orig) : _trie(orig._trie), _fstr(orig._fstr), _delim(orig._delim) {
            }

            template<TTrieSize N, bool doCache>
            ARPATrieBuilder<N, doCache>::~ARPATrieBuilder() {
            }

            template<TTrieSize N, bool doCache>
            void ARPATrieBuilder<N, doCache>::build() {
                LOG_DEBUG << "Starting to read the file and build the trie ..." << END_LOG;

                //Initialize the NGram builder and give it the trie as an argument
                NGramBuilder<N, doCache> ngBuilder(_trie, _delim);

                //Do the progress bard indicator
                Logger::startProgressBar();

                //Iterate through the ARPA file and fill in the back-off model of the trie
                //Note that, this file reader will be made ads flexible as possible,
                //in other words is will ignore as much data as possible and will report
                //as few errors as possible, mostly warnings. Also the Maximum N-Gram Level
                //will be limited by the N parameter provided to the class template and not
                //the maximum N-gram level present in the file.
                string line;
                while (getline(_fstr, line)) {
                    LOG_DEBUG << line << END_LOG;

                    //if ()

                    ngBuilder.processString(line);
                    Logger::updateProgressBar();
                }

                Logger::stopProgressBar();

                LOG_DEBUG << "Done reading the file and building the trie." << END_LOG;
            }

            //Make sure that there will be templates instantiated, at least for the given parameter values
            template class ARPATrieBuilder< N_GRAM_PARAM, true >;
            template class ARPATrieBuilder< N_GRAM_PARAM, false >;
        }
    }
}
