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
#include "ARPAGramBuilder.hpp"
#include "ARPAGramBuilderFactory.hpp"

using namespace uva::smt::logging;
using namespace uva::smt::utils::text;

namespace uva {
    namespace smt {
        namespace tries {
            namespace arpa {

#define END_OF_ARPA_FILE "\\end\\"

                template<TModelLevel N>
                ARPATrieBuilder<N>::ARPATrieBuilder(ATrie<N> & trie, ifstream & fstr, const char delim) :
                _trie(trie), _fstr(fstr), _delim(delim), _ngAmountRegExp("ngram [[:d:]]+=[[:d:]]+") {
                }

                template<TModelLevel N>
                ARPATrieBuilder<N>::ARPATrieBuilder(const ARPATrieBuilder<N>& orig) :
                _trie(orig._trie), _fstr(orig._fstr), _delim(orig._delim), _ngAmountRegExp("ngram [[:d:]]+=[[:d:]]+") {
                }

                template<TModelLevel N>
                ARPATrieBuilder<N>::~ARPATrieBuilder() {
                }

                template<TModelLevel N>
                void ARPATrieBuilder<N>::readHeaders(string &line) {
                    LOG_DEBUG << "Start reading ARPA headers." << END_LOG;

                    while (true) {
                        LOG_DEBUG1 << "Read header (?) line: '" << line << "'" << END_LOG;
                        reduce(line);

                        //If the line is empty then we keep reading
                        if (line != "") {
                            //If the line begins with "<" then it could be a header
                            //or something else that we do not need to read or
                            //interpret! Therefore we skip it and read on, otherwise
                            //it is potentially a meaningful data so we should stop
                            //and go on to the next section, namely" data
                            if (line.substr(0, 1) != "<") {
                                LOG_DEBUG1 << "Is something meaningful, moving to data section!" << END_LOG;
                                break;
                            } else {
                                LOG_DEBUG1 << "Is something meaningless, starting with <, skipping forward" << END_LOG;
                            }
                        } else {
                            LOG_DEBUG1 << "Is an empty line, skipping forward" << END_LOG;
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

                template<TModelLevel N>
                void ARPATrieBuilder<N>::readData(string &line, uint counts[N]) {
                    LOG_DEBUG << "Start reading ARPA data." << END_LOG;
                    
                    LOG_WARNING << "The N-Gram Counts provided in the data section of ARPA are not read yet!" << END_LOG;

                    //If we are here then it means we just finished reading the
                    //ARPA headers and we stumbled upon something meaningful,
                    //that actually must be the begin of the data section
                    if (line != END_OF_ARPA_FILE) {
                        while (true) {
                            if (getline(_fstr, line)) {
                                LOG_DEBUG1 << "Read data (?) line: '" << line << "'" << END_LOG;
                                reduce(line);

                                //Update the progress bar status
                                Logger::updateProgressBar();

                                //If the line is empty then we keep reading
                                if (line != "") {
                                    //Check that the next line contains the meaningful N-gram amount information!
                                    if (regex_match(line, _ngAmountRegExp)) {
                                        //This is a valid data section entry, there is no need to do anything with it.
                                        //Later we might want to read the numbers and then check them against the
                                        //actual number of provided n-grams but for now it is not needed. 
                                        LOG_DEBUG1 << "Is the n-gram amount: '" << line << "', ignoring!" << END_LOG;

                                        //ToDo: Read the number of N-grams in order to have enough data
                                        //      for the pre-allocation of the memory in the trie! For
                                        //      large language models that should both improve the memory
                                        //      usage and the performance of adding new N-Grams!
                                    } else {
                                        LOG_DEBUG1 << "Is something other than n-gram amount, moving to n-gram sections!" << END_LOG;
                                        break;
                                    }
                                } else {
                                    LOG_DEBUG1 << "Is an empty line, skipping forward" << END_LOG;
                                }

                            } else {
                                throw Exception("Incorrect ARPA format: An unexpected end of file while reading the ARPA data section!");
                            }
                        }
                    } else {
                        stringstream msg;
                        msg << "Incorrect ARPA format: Got '" << line << "' instead of '" << END_OF_ARPA_FILE << "' when starting on the data section!";
                        throw Exception(msg.str());
                    }

                    LOG_DEBUG << "Finished reading ARPA data." << END_LOG;
                }

                template<TModelLevel N>
                void ARPATrieBuilder<N>::readNGrams(string &line, const TModelLevel level) {
                    LOG_DEBUG << "Start reading ARPA " << level << "-Grams." << END_LOG;
                    //The regular expression for matching the n-grams section
                    stringstream regexpStr;
                    regexpStr << "\\\\" << level << "\\-grams\\:";
                    LOG_DEBUG1 << "The N-gram section reg-exp: '" << regexpStr.str() << "'" << END_LOG;
                    const regex ngSectionRegExp(regexpStr.str());

                    //Check if the line that was input is the header of the N-grams section for N=level
                    if (regex_match(line, ngSectionRegExp)) {
                        //Declare the pointer to the N-Grma builder
                        ARPAGramBuilder *pNGBuilder = NULL;
                        ARPAGramBuilderFactory::getBuilder<N>(level, _trie, _delim, &pNGBuilder);

                        try {
                            //The counter of the N-grams
                            uint numNgrams = 0;
                            //Read the current level N-grams and add them to the trie
                            while (true) {
                                //Try to read the next line
                                if (getline(_fstr, line)) {
                                    LOG_DEBUG1 << "Read " << level << "-Gram (?) line: '" << line << "'" << END_LOG;
                                    reduce(line);

                                    //Empty lines will just be skipped
                                    if (line != "") {
                                        //Pass the given N-gram string to the N-Gram Builder. If the
                                        //N-gram is not matched then stop the loop and move on
                                        if (pNGBuilder->processString(line)) {
                                            //If there was no match then it is something else
                                            //than the given level N-gram so we move on
                                            
                                            //First log the actual amount of read N-grams
                                            const bool wasPBOn = Logger::isProgressBarOn();
                                            Logger::stopProgressBar();
                                            LOG_INFO << "The number of read " << level << "-grams is: " << numNgrams << END_LOG;
                                            if( wasPBOn ) {
                                                Logger::startProgressBar();
                                            }
                                            
                                            //Now stop reading this level N-grams and move on
                                            break;
                                        }
                                        numNgrams++;
                                    }

                                    //Update the progress bar status
                                    Logger::updateProgressBar();
                                } else {
                                    //If the next line does not exist then it an error as we expect the end of data section any way
                                    stringstream msg;
                                    msg << "Incorrect ARPA format: Unexpected end of file, missing the '" << END_OF_ARPA_FILE << "' tag!";
                                    throw Exception(msg.str());
                                }
                            }
                        } catch (...) {
                            //Free the allocated N-gram builder in case of an exception 
                            delete pNGBuilder;
                            //Re-throw the exception
                            throw;
                        }
                        //Free the allocated N-gram builder in case of no exception 
                        delete pNGBuilder;
                        pNGBuilder = NULL;

                        LOG_DEBUG << "Finished reading ARPA " << level << "-Grams." << END_LOG;

                        //If we expect more N-grams then make a recursive call to read the higher order N-gram
                        LOG_DEBUG2 << "The currently read N-grams level is " << level << ", the maximum level is " << N
                                << ", the current line is '" << line << "'" << END_LOG;

                        //Test if we need to move on or we are done or an error is detected
                        if (level < N) {
                            //There are still N-Gram levels to read
                            if (line != END_OF_ARPA_FILE) {
                                //We did not encounter the \end\ tag yet so do recursion to the next level
                                readNGrams(line, level + 1);
                            } else {
                                //We did encounter the \end\ tag, this is not really expected, but it is not fatal
                                LOG_WARNING << "End of ARPA file, read " << level << "-grams and there is "
                                        << "nothing more to read. The maximum allowed N-gram level is " << N << END_LOG;
                            }
                        } else {
                            //Here the level is >= N, so we must have read a valid \end\ tag, otherwise an error!
                            if (line != END_OF_ARPA_FILE) {
                                stringstream msg;
                                msg << "Incorrect ARPA format: Got '" << line << "' instead of '" << END_OF_ARPA_FILE
                                        << "' when reading " << level << "-grams section!";
                                throw Exception(msg.str());
                            }
                        }
                    } else {
                        //The obtained string is something else than the next n-grams section header
                        //So the only thing it is allowed to be is the end of file, let's check on
                        //it and otherwise report an error
                        if (line != END_OF_ARPA_FILE) {
                            stringstream msg;
                            msg << "Incorrect ARPA format: Got '" << line
                                    << "' when trying to read the " << level
                                    << "-grams section!";
                            throw Exception(msg.str());
                        }
                    }
                }

                //Iterate through the ARPA file and fill in the back-off model of the trie
                //Note that, this file reader will be made ads flexible as possible,
                //in other words is will ignore as much data as possible and will report
                //as few errors as possible, mostly warnings. Also the Maximum N-Gram Level
                //will be limited by the N parameter provided to the class template and not
                //the maximum N-gram level present in the file.

                template<TModelLevel N>
                void ARPATrieBuilder<N>::build() {
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

                        //Declare an array of N-Gram counts, that is to be filled from the
                        //headers. This data will be used to pre-allocate memory for the Trie 
                        uint counts[N];

                        //Read the DATA section of ARPA
                        readData(line, counts);

                        //Provide the N-Gram counts data to the Trie

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
                template class ARPATrieBuilder< MAX_NGRAM_LEVEL >;
            }
        }
    }
}