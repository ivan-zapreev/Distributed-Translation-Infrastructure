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

#include "Globals.hpp"
#include "Logger.hpp"
#include "StringUtils.hpp"
#include "NGramBuilder.hpp"

using namespace uva::smt::logging;
using namespace uva::smt::utils::string;
using uva::smt::tries::ngrams::NGramBuilder;

namespace uva {
    namespace smt {
        namespace tries {

            template<TTrieSize N, bool doCache>
            ARPATrieBuilder<N, doCache>::ARPATrieBuilder(ATrie<N, doCache> & trie, ifstream & fstr, const char delim) :
            _trie(trie), _fstr(fstr), _delim(delim), _ngaRegExp("ngram [[:d:]]+=[[:d:]]+") {
            }

            template<TTrieSize N, bool doCache>
            ARPATrieBuilder<N, doCache>::ARPATrieBuilder(const ARPATrieBuilder<N, doCache>& orig) :
            _trie(orig._trie), _fstr(orig._fstr), _delim(orig._delim), _ngaRegExp("ngram [[:d:]]+=[[:d:]]+") {
            }

            template<TTrieSize N, bool doCache>
            ARPATrieBuilder<N, doCache>::~ARPATrieBuilder() {
            }

            template<TTrieSize N, bool doCache>
            void ARPATrieBuilder<N, doCache>::readHeaders(string &line) {
                LOG_DEBUG << "Start reading ARPA headers." << END_LOG;

                while (true) {
                    LOG_DEBUG1 << "Reading header (?) line: '" << line << "'" << END_LOG;
                    reduce(line);

                    //If the line is empty then we keep reading
                    if (line != "") {
                        //If the line begins with "<" then it could be a header
                        //or something else that we do not need to read or
                        //interpret! Therefore we skip it and read on, otherwise
                        //it is potentially a meaningful data so we should stop
                        //and go on to the next section, namely" data
                        if (line.substr(0, 1) != "<") {
                            LOG_DEBUG1 << "Read something meaningful, moving to data section!" << END_LOG;
                            break;
                        } else {
                            LOG_DEBUG1 << "Read something meaningless, starting with <, skipping forward" << END_LOG;
                        }
                    } else {
                        LOG_DEBUG1 << "Read an empty line, skipping forward" << END_LOG;
                    }

                    //Update the progress bar status
                    Logger::updateProgressBar();

                    //Read the next line from the file if it is there
                    if (!getline(_fstr, line)) {
                        throw Exception("Incorrect ARPA format: An unexpected end of file while reading the ARPA headers!");
                    }
                }

                LOG_DEBUG << "Finished reading ARPA headers." << END_LOG;
            }

            template<TTrieSize N, bool doCache>
            void ARPATrieBuilder<N, doCache>::readData(string &line) {
                LOG_DEBUG << "Start reading ARPA data." << END_LOG;

                //If we are here then it means we just finished reading the
                //ARPA headers and we stumbled upon something meaningful,
                //that actually must be the begin of the data section
                if (line == "\\data\\") {
                    while (true) {
                        if (getline(_fstr, line)) {
                            LOG_DEBUG1 << "Reading data (?) line: '" << line << "'" << END_LOG;
                            reduce(line);

                            //Update the progress bar status
                            Logger::updateProgressBar();

                            //If the line is empty then we keep reading
                            if (line != "") {
                                //Check that the next line contains the meaningful N-gram amount information!
                                if (regex_match(line, _ngaRegExp)) {
                                    //This is a valid data section entry, there is no need to do anything with it.
                                    //Later we might want to read the numbers and then check them against the
                                    //actual number of provided n-grams but for now it is not needed. 
                                    LOG_DEBUG1 << "Read the n-gram amount: '" << line << "', ignoring!" << END_LOG;
                                } else {
                                    LOG_DEBUG1 << "Read something other than n-gram amount, moving to n-gram sections!" << END_LOG;
                                    break;
                                }
                            } else {
                                LOG_DEBUG1 << "Read an empty line, skipping forward" << END_LOG;
                            }

                        } else {
                            throw Exception("Incorrect ARPA format: An unexpected end of file while reading the ARPA data section!");
                        }
                    }
                } else {
                    stringstream msg;
                    msg << "Incorrect ARPA format: Got '" << line << "' instead of \\data\\ when starting on the data section!";
                    throw Exception(msg.str());
                }

                LOG_DEBUG << "Finished reading ARPA data." << END_LOG;
            }

            template<TTrieSize N, bool doCache>
            void ARPATrieBuilder<N, doCache>::readNGrams(string &line, const int level) {
                LOG_DEBUG << "Start reading ARPA " << level << "-Grams." << END_LOG;

                do {
                    LOG_DEBUG1 << "Reading " << level << "-Gram (?) line: '" << line << "'" << END_LOG;

                    //Update the progress bar status
                    Logger::updateProgressBar();
                    
                    //ToDo: Implement!
                    
                } while (getline(_fstr, line));

                LOG_DEBUG << "Finished reading ARPA " << level << "-Grams." << END_LOG;
            }

            //Iterate through the ARPA file and fill in the back-off model of the trie
            //Note that, this file reader will be made ads flexible as possible,
            //in other words is will ignore as much data as possible and will report
            //as few errors as possible, mostly warnings. Also the Maximum N-Gram Level
            //will be limited by the N parameter provided to the class template and not
            //the maximum N-gram level present in the file.

            template<TTrieSize N, bool doCache>
            void ARPATrieBuilder<N, doCache>::build() {
                LOG_DEBUG << "Starting to read the file and build the trie ..." << END_LOG;

                //Do the progress bard indicator
                Logger::startProgressBar();

                try {
                    //This is the variable that will store the last read line of ARPA file
                    string line = "";
                    //Read the first line from the file
                    getline(_fstr, line);

                    //Skip on ARPA headers
                    readHeaders(line);

                    //Read the DATA section of ARPA
                    readData(line);

                    //Read the N-grams
                    readNGrams(line, 1);
                } catch (...) {
                    //Stop the progress bar in case of an exception
                    Logger::stopProgressBar();
                    throw;
                }

                //Stop the progress bar in case of no exception
                Logger::stopProgressBar();

                LOG_DEBUG << "Done reading the file and building the trie." << END_LOG;
            }

            //Make sure that there will be templates instantiated, at least for the given parameter values
            template class ARPATrieBuilder< N_GRAM_PARAM, true >;
            template class ARPATrieBuilder< N_GRAM_PARAM, false >;
        }
    }
}
