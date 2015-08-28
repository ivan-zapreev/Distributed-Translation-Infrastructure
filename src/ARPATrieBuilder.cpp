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

                //The end of ARPA file constant
                static const string END_OF_ARPA_FILE = "\\end\\";
                //The N-gram Data Section Amoung delimiter
                static const char NGRAM_COUNTS_DELIM = '=';

                template<TModelLevel N>
                ARPATrieBuilder<N>::ARPATrieBuilder(ATrie<N> & trie, TextPieceReader & file) :
                m_trie(trie), m_file(file), m_line(), m_ngAmountRegExp("ngram [[:d:]]+=[[:d:]]+") {
                }

                template<TModelLevel N>
                ARPATrieBuilder<N>::ARPATrieBuilder(const ARPATrieBuilder<N>& orig) :
                m_trie(orig.m_trie), m_file(orig.m_file), m_line(orig.m_line), m_ngAmountRegExp("ngram [[:d:]]+=[[:d:]]+") {
                }

                template<TModelLevel N>
                ARPATrieBuilder<N>::~ARPATrieBuilder() {
                }

                template<TModelLevel N>
                void ARPATrieBuilder<N>::readHeaders() {
                    LOG_DEBUG << "Start reading ARPA headers." << END_LOG;

                    while (true) {
                        LOG_DEBUG1 << "Read header (?) line: '" << m_line << "'" << END_LOG;

                        //If the line is empty then we keep reading
                        if (m_line != "") {
                            //If the line begins with "<" then it could be a header
                            //or something else that we do not need to read or
                            //interpret! Therefore we skip it and read on, otherwise
                            //it is potentially a meaningful data so we should stop
                            //and go on to the next section, namely" data
                            if (m_line[0] != '<') {
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
                        if (!m_file.getLine(m_line)) {
                            throw Exception("Incorrect ARPA format: An unexpected end of file while reading the ARPA headers!");
                        }
                    }

                    LOG_DEBUG << "Finished reading ARPA headers." << END_LOG;
                }

                template<TModelLevel N>
                void ARPATrieBuilder<N>::readData(size_t counts[N]) {
                    LOG_DEBUG << "Start reading ARPA data." << END_LOG;

                    //If we are here then it means we just finished reading the
                    //ARPA headers and we stumbled upon something meaningful,
                    //that actually must be the begin of the data section
                    if (m_line != END_OF_ARPA_FILE) {
                        TModelLevel level = MIN_NGRAM_LEVEL;
                        while (true) {
                            if (m_file.getLine(m_line)) {
                                LOG_DEBUG1 << "Read data (?) line: '" << m_line << "'" << END_LOG;

                                //Update the progress bar status
                                Logger::updateProgressBar();

                                //If the line is empty then we keep reading
                                if (m_line != "") {
                                    //Check that the next line contains the meaningful N-gram amount information!
                                    if (regex_match(m_line.str(), m_ngAmountRegExp)) {
                                        //This is a valid data section entry, there is no need to do anything with it.
                                        //Later we might want to read the numbers and then check them against the
                                        //actual number of provided n-grams but for now it is not needed. 
                                        LOG_DEBUG1 << "Reading " << level << "-gram amount, got: '" << m_line << "'!" << END_LOG;

                                        //Read the number of N-grams in order to have enough data
                                        //for the pre-allocation of the memory in the trie! For
                                        //large language models that should both improve the memory
                                        //usage and the performance of adding new N-Grams!

                                        //First tokenize the string
                                        vector<string> elems;
                                        tokenize(m_line.str(), NGRAM_COUNTS_DELIM, elems);

                                        //Parse the second (last) value and store it as the amount
                                        string & amount = *(--elems.end());
                                        try {
                                            counts[level - 1] = stoull(amount);
                                            LOG_DEBUG << "Expected number of " << level << "-grams is: " << counts[level - 1] << END_LOG;
                                        } catch (invalid_argument) {
                                            stringstream msg;
                                            msg << "Incorrect ARPA format: Can not parse the "
                                                    << level << "-gram amount: '" << amount
                                                    << "' from string: '" << m_line << "'";
                                            throw Exception(msg.str());
                                        }
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
                            level++;
                        }
                    } else {
                        stringstream msg;
                        msg << "Incorrect ARPA format: Got '" << m_line << "' instead of '" << END_OF_ARPA_FILE << "' when starting on the data section!";
                        throw Exception(msg.str());
                    }

                    LOG_INFO << "Expected number of M-grams per level: " << arrayToString<size_t, N>(counts) << END_LOG;

                    LOG_DEBUG << "Finished reading ARPA data." << END_LOG;
                }

                template<TModelLevel N>
                void ARPATrieBuilder<N>::readNGrams(const TModelLevel level) {
                    stringstream msg;
                    //Do the progress bard indicator
                    msg << "Reading ARPA " << level << "-Grams";
                    Logger::startProgressBar(msg.str());

                    //The regular expression for matching the n-grams section
                    stringstream regexpStr;
                    regexpStr << "\\\\" << level << "\\-grams\\:";
                    LOG_DEBUG1 << "The N-gram section reg-exp: '" << regexpStr.str() << "'" << END_LOG;
                    const regex ngSectionRegExp(regexpStr.str());

                    //Check if the line that was input is the header of the N-grams section for N=level
                    if (regex_match(m_line.str(), ngSectionRegExp)) {
                        //Declare the pointer to the N-Grma builder
                        ARPAGramBuilder *pNGBuilder = NULL;
                        ARPAGramBuilderFactory::getBuilder<N>(level, m_trie, &pNGBuilder);

                        try {
                            //The counter of the N-grams
                            uint numNgrams = 0;
                            //Read the current level N-grams and add them to the trie
                            while (true) {
                                //Try to read the next line
                                if (m_file.getLine(m_line)) {
                                    LOG_DEBUG1 << "Read " << level << "-Gram (?) line: '" << m_line << "'" << END_LOG;

                                    //Empty lines will just be skipped
                                    if (m_line != "") {
                                        //Pass the given N-gram string to the N-Gram Builder. If the
                                        //N-gram is not matched then stop the loop and move on
                                        if (pNGBuilder->parseLine(m_line)) {
                                            //If there was no match then it is something else
                                            //than the given level N-gram so we move on
                                            LOG_DEBUG << "Actual number of " << level << "-grams is: " << numNgrams << END_LOG;

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
                        //Stop the progress bar in case of no exception
                        Logger::stopProgressBar();

                        //Check if the post gram actions are needed! If yes - perform.
                        if (m_trie.isPost_Grams(level)) {
                            //Do the progress bard indicator
                            stringstream msg;
                            msg << "Cultivating " << level << "-Grams";
                            Logger::startProgressBar(msg.str());

                            //Do the post level actions
                            m_trie.post_Grams(level);

                            //Stop the progress bar in case of no exception
                            Logger::stopProgressBar();
                            LOG_DEBUG << "Finished post actions of " << level << "-Grams." << END_LOG;
                        } else {
                            LOG_INFO3 << "Cultivating " << level << "-Grams:\t Not needed!" << END_LOG;
                        }

                        //If we expect more N-grams then make a recursive call to read the higher order N-gram
                        LOG_DEBUG2 << "The currently read N-grams level is " << level << ", the maximum level is " << N
                                << ", the current line is '" << m_line << "'" << END_LOG;

                        //Test if we need to move on or we are done or an error is detected
                        if (level < N) {
                            //There are still N-Gram levels to read
                            if (m_line != END_OF_ARPA_FILE) {
                                //We did not encounter the \end\ tag yet so do recursion to the next level
                                readNGrams(level + 1);
                            } else {
                                //We did encounter the \end\ tag, this is not really expected, but it is not fatal
                                LOG_WARNING << "End of ARPA file, read " << level << "-grams and there is "
                                        << "nothing more to read. The maximum allowed N-gram level is " << N << END_LOG;
                            }
                        } else {
                            //Here the level is >= N, so we must have read a valid \end\ tag, otherwise an error!
                            if (m_line != END_OF_ARPA_FILE) {
                                stringstream msg;
                                msg << "Incorrect ARPA format: Got '" << m_line << "' instead of '" << END_OF_ARPA_FILE
                                        << "' when reading " << level << "-grams section!";
                                throw Exception(msg.str());
                            }
                        }
                    } else {
                        //The obtained string is something else than the next n-grams section header
                        //So the only thing it is allowed to be is the end of file, let's check on
                        //it and otherwise report an error
                        if (m_line != END_OF_ARPA_FILE) {
                            stringstream msg;
                            msg << "Incorrect ARPA format: Got '" << m_line
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

                    try {
                        //Read the first line from the file
                        m_file.getLine(m_line);

                        //Skip on ARPA headers
                        readHeaders();

                        //Declare an array of N-Gram counts, that is to be filled from the
                        //headers. This data will be used to pre-allocate memory for the Trie 
                        size_t counts[N];
                        for (int i = 0; i < N; i++) {
                            counts[i] = 0;
                        }

                        //Read the DATA section of ARPA
                        readData(counts);

                        //Provide the N-Gram counts data to the Trie
                        m_trie.preAllocate(counts);

                        //Read the N-grams, starting from 1-Grams
                        readNGrams(1);
                    } catch (...) {
                        //Stop the progress bar in case of an exception
                        Logger::stopProgressBar();
                        throw;
                    }

                    LOG_DEBUG << "Done reading the file and building the trie." << END_LOG;
                }

                //Make sure that there will be templates instantiated, at least for the given parameter values
                template class ARPATrieBuilder< MAX_NGRAM_LEVEL >;
            }
        }
    }
}