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

#include "C2DHashMapTrie.hpp"
#include "W2CHybridMemoryTrie.hpp"
#include "C2WOrderedArrayTrie.hpp"
#include "W2COrderedArrayTrie.hpp"
#include "C2DMapArrayTrie.hpp"
#include "G2DHashMapTrie.hpp"

using namespace uva::smt::tries;
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

                template<typename TrieType>
                ARPATrieBuilder<TrieType>::ARPATrieBuilder(TrieType & trie, AFileReader & file) :
                m_trie(trie), m_file(file), m_line(), m_ng_amount_reg_exp("ngram [[:d:]]+=[[:d:]]+") {
                }

                template<typename TrieType>
                ARPATrieBuilder<TrieType>::ARPATrieBuilder(const ARPATrieBuilder<TrieType>& orig) :
                m_trie(orig.m_trie), m_file(orig.m_file), m_line(orig.m_line), m_ng_amount_reg_exp("ngram [[:d:]]+=[[:d:]]+") {
                }

                template<typename TrieType>
                ARPATrieBuilder<TrieType>::~ARPATrieBuilder() {
                }

                template<typename TrieType>
                void ARPATrieBuilder<TrieType>::read_headers() {
                    LOG_DEBUG << "Start reading ARPA headers." << END_LOG;

                    while (true) {
                        LOG_DEBUG1 << "Read header (?) line: '" << m_line.str() << "'" << END_LOG;

                        //If the line is empty then we keep reading
                        if (m_line.hasMore()) {
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

                template<typename TrieType>
                void ARPATrieBuilder<TrieType>::pre_allocate(size_t counts[TrieType::max_level]) {
                    LOG_INFO << "Expected number of M-grams per level: "
                            << arrayToString<size_t, TrieType::max_level>(counts) << END_LOG;

                    //Do the progress bard indicator
                    Logger::startProgressBar(string("Pre-allocating memory"));

                    //Provide the N-Gram counts data to the Trie
                    m_trie.pre_allocate(counts);

                    Logger::updateProgressBar();

                    LOG_DEBUG << "Finished pre-allocating memory" << END_LOG;
                    //Stop the progress bar in case of no exception
                    Logger::stopProgressBar();
                }

                template<typename TrieType>
                void ARPATrieBuilder<TrieType>::read_data(size_t counts[TrieType::max_level]) {
                    LOG_DEBUG << "Start reading ARPA data." << END_LOG;

                    //If we are here then it means we just finished reading the
                    //ARPA headers and we stumbled upon something meaningful,
                    //that actually must be the begin of the data section
                    if (m_line != END_OF_ARPA_FILE) {
                        TModelLevel level = M_GRAM_LEVEL_1;
                        while (true) {
                            if (m_file.getLine(m_line)) {
                                LOG_DEBUG1 << "Read data (?) line: '" << m_line.str() << "'" << END_LOG;

                                //Update the progress bar status
                                Logger::updateProgressBar();

                                //If the line is empty then we keep reading
                                if (m_line.hasMore()) {
                                    //Check that the next line contains the meaningful N-gram amount information!
                                    if (regex_match(m_line.str(), m_ng_amount_reg_exp)) {
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
                        msg << "Incorrect ARPA format: Got '" << m_line << "' instead of '"
                                << END_OF_ARPA_FILE << "' when starting on the data section!";
                        throw Exception(msg.str());
                    }

                    LOG_DEBUG << "Finished reading ARPA data." << END_LOG;
                }

                template<typename TrieType>
                void ARPATrieBuilder<TrieType>::read_m_gram_level(const TModelLevel level) {
                    //Declare the pointer to the N-Grma builder
                    ARPAGramBuilder *pNGBuilder = NULL;
                    ARPAGramBuilderFactory<TrieType>::get_builder(level, m_trie, &pNGBuilder);

                    try {
                        //The counter of the N-grams
                        uint numNgrams = 0;
                        //Read the current level N-grams and add them to the trie
                        while (true) {
                            //Try to read the next line
                            if (m_file.getLine(m_line)) {
                                LOG_DEBUG1 << "Read " << level << "-Gram (?) line: '" << m_line.str() << "'" << END_LOG;

                                //Empty lines will just be skipped
                                if (m_line.hasMore()) {
                                    //Pass the given N-gram string to the N-Gram Builder. If the
                                    //N-gram is not matched then stop the loop and move on
                                    if (pNGBuilder->parse_line(m_line)) {
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
                }

                template<typename TrieType>
                void ARPATrieBuilder<TrieType>::check_and_go_m_grams(const TModelLevel level) {
                    //If we expect more N-grams then make a recursive call to read the higher order N-gram
                    LOG_DEBUG2 << "The currently read N-grams level is " << level << ", the maximum level is " << TrieType::max_level
                            << ", the current line is '" << m_line << "'" << END_LOG;

                    //Test if we need to move on or we are done or an error is detected
                    if (level < TrieType::max_level) {
                        //There are still N-Gram levels to read
                        if (m_line != END_OF_ARPA_FILE) {
                            //We did not encounter the \end\ tag yet so do recursion to the next level
                            read_grams(level + 1);
                        } else {
                            //We did encounter the \end\ tag, this is not really expected, but it is not fatal
                            LOG_WARNING << "End of ARPA file, read " << level << "-grams and there is "
                                    << "nothing more to read. The maximum allowed N-gram level is " << TrieType::max_level << END_LOG;
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
                }

                template<typename TrieType>
                void ARPATrieBuilder<TrieType>::do_post_m_gram_actions(const TModelLevel level) {
                    //Check if the post gram actions are needed! If yes - perform.
                    if (m_trie.is_post_grams(level)) {
                        //Do the progress bard indicator
                        stringstream msg;
                        msg << "Cultivating " << level << "-Grams";
                        Logger::startProgressBar(msg.str());

                        //Do the post level actions
                        m_trie.post_grams(level);

                        //Stop the progress bar in case of no exception
                        Logger::stopProgressBar();
                        LOG_DEBUG << "Finished post actions of " << level << "-Grams." << END_LOG;
                    } else {
                        LOG_INFO3 << "Cultivating " << level << "-Grams:\t Not needed!" << END_LOG;
                    }
                }

                template<typename TrieType>
                void ARPATrieBuilder<TrieType>::get_word_counts() {
                    //Check if we need another pass for words counting.
                    if (m_trie.get_word_index().is_word_counts_needed()) {
                        //Do the progress bard indicator
                        Logger::startProgressBar(string("Counting all words"));

                        //Start recursive counting of words
                        get_word_counts(1);
                        LOG_DEBUG1 << "Finished counting words in M-grams!" << END_LOG;

                        //Perform the post counting actions;
                        m_trie.get_word_index().do_post_word_count();

                        LOG_DEBUG << "Finished counting all words" << END_LOG;
                        //Stop the progress bar in case of no exception
                        Logger::stopProgressBar();

                        //Rewind to the beginning of the 1-grams section
                        return_to_grams();
                    }
                }

                template<typename TrieType>
                void ARPATrieBuilder<TrieType>::do_word_index_post_1_gram_actions() {
                    //Perform the post actions if needed
                    if (m_trie.get_word_index().is_post_actions_needed()) {
                        //Do the progress bard indicator
                        Logger::startProgressBar(string("Word Index post actions"));

                        LOG_DEBUG << "Starting to perform the Word Index post actions" << END_LOG;

                        //Perform the post actions
                        m_trie.get_word_index().do_post_actions();

                        LOG_DEBUG << "Finished performing the Word Index post actions" << END_LOG;

                        //Stop the progress bar in case of no exception
                        Logger::stopProgressBar();
                    }
                }

                template<typename TrieType>
                void ARPATrieBuilder<TrieType>::read_grams(const TModelLevel level) {
                    stringstream msg;
                    //Do the progress bard indicator
                    msg << "Reading ARPA " << level << "-Grams";
                    Logger::startProgressBar(msg.str());

                    //The regular expression for matching the n-grams section
                    stringstream regexpStr;
                    regexpStr << "\\\\" << level << "\\-grams\\:";
                    LOG_DEBUG1 << "The N-gram section reg-exp: '" << regexpStr.str() << "'" << END_LOG;
                    const regex n_gram_sect_reg_exp(regexpStr.str());

                    //Check if the line that was input is the header of the N-grams section for N=level
                    if (regex_match(m_line.str(), n_gram_sect_reg_exp)) {
                        //Read the M-grams of the given level
                        read_m_gram_level(level);

                        //If the first M-gram level has been read then do
                        //the word index post-actions if needed.
                        if (level == M_GRAM_LEVEL_1) {
                            do_word_index_post_1_gram_actions();
                        }

                        //Perform the post-M-gram actions if needed
                        do_post_m_gram_actions(level);

                        //Check if we need to keep reading and recurse or we are done
                        check_and_go_m_grams(level);
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

                /**
                 * If the word counts are needed then we are starting
                 * on reading the M-gram section of the ARPA file.
                 * All we need to do is then read all the words in
                 * M-Gram sections and count them with the word index.
                 */
                template<typename TrieType>
                void ARPATrieBuilder<TrieType>::get_word_counts(const TModelLevel level) {
                    typename TrieType::WordIndexType & word_index = m_trie.get_word_index();

                    //The regular expression for matching the n-grams section
                    stringstream regexpStr;
                    regexpStr << "\\\\" << level << "\\-grams\\:";
                    LOG_DEBUG1 << "The N-gram section reg-exp: '" << regexpStr.str() << "'" << END_LOG;
                    const regex n_gram_sect_reg_exp(regexpStr.str());

                    //Check if the line that was input is the header of the N-grams section for N=level
                    if (regex_match(m_line.str(), n_gram_sect_reg_exp)) {
                        //The tokens array to put words into
                        TextPieceReader tokens[TrieType::max_level];

                        //Read the current level N-grams and add them to the trie
                        while (m_file.getLine(m_line)) {
                            LOG_DEBUG1 << "Reading " << SSTR(level) << "-gram, got: [" << m_line.str() << "]" << END_LOG;
                            //If this is not an empty line
                            if (m_line.hasMore()) {
                                //Parse line to words without probabilities and back-offs
                                //If it is not the M-gram line then we stop break
                                if (ARPAGramBuilder::gram_line_to_tokens(m_line, tokens, level)) {
                                    //Add words to the index: count them
                                    for (size_t idx = 0; idx < level; idx++) {
                                        LOG_DEBUG2 << "Adding the " << SSTR(idx) << "'th word to word index." << END_LOG;
                                        word_index.count_word(tokens[idx]);
                                        //Update the progress bar status
                                        Logger::updateProgressBar();
                                    }
                                } else {
                                    //This is not an expected M-gram
                                    LOG_DEBUG1 << "Stopping words counting in M-gram level: " << SSTR(level) << END_LOG;
                                    break;
                                }
                            }
                        }

                        LOG_DEBUG3 << "Line : " << m_line.str() << END_LOG;

                        //Test if we need to move on or we are done or an error is detected
                        if ((level < TrieType::max_level) && (m_line != END_OF_ARPA_FILE)) {
                            LOG_DEBUG1 << "Finished counting words in " << SSTR(level)
                                    << "-grams, going to the next level" << END_LOG;
                            //There are still N-Gram levels to read
                            //We did not encounter the \end\ tag yet so do recursion to the next level
                            get_word_counts(level + 1);
                        }
                        LOG_DEBUG1 << "Finished counting words in "
                                << SSTR(level) << "-grams" << END_LOG;
                    }

                    //Update the progress bar status
                    Logger::updateProgressBar();
                }

                /**
                 * This method should be called after the word counting is done.
                 * Then we can re-start reading the file and move forward to the
                 * one gram section again. After this method we can proceed
                 * reading M-grams and add them to the trie.
                 */
                template<typename TrieType>
                void ARPATrieBuilder<TrieType>::return_to_grams() {
                    //Reset the file
                    m_file.reset();

                    //Read the first line from the file
                    m_file.getLine(m_line);

                    //Skip on ARPA headers
                    read_headers();

                    //Read the DATA section of ARPA
                    size_t counts[TrieType::max_level];
                    memset(counts, 0, TrieType::max_level * sizeof (size_t));
                    read_data(counts);
                }


                //Iterate through the ARPA file and fill in the back-off model of the trie
                //Note that, this file reader will be made ads flexible as possible,
                //in other words is will ignore as much data as possible and will report
                //as few errors as possible, mostly warnings. Also the Maximum N-Gram Level
                //will be limited by the N parameter provided to the class template and not
                //the maximum N-gram level present in the file.

                template<typename TrieType>
                void ARPATrieBuilder<TrieType>::build() {
                    LOG_DEBUG << "Starting to read the file and build the trie ..." << END_LOG;

                    //Declare an array of N-Gram counts, that is to be filled from the
                    //headers. This data will be used to pre-allocate memory for the Trie 
                    size_t counts[TrieType::max_level];
                    memset(counts, 0, TrieType::max_level * sizeof (size_t));

                    try {
                        //Read the first line from the file
                        m_file.getLine(m_line);

                        //Skip on ARPA headers
                        read_headers();

                        //Read the DATA section of ARPA
                        read_data(counts);

                        //Pre-allocate memory
                        pre_allocate(counts);

                        //Get the word counts, if needed
                        get_word_counts();

                        //Read the N-grams, starting from 1-Grams
                        read_grams(1);
                    } catch (...) {
                        //Stop the progress bar in case of an exception
                        Logger::stopProgressBar();
                        throw;
                    }

                    LOG_DEBUG << "Done reading the file and building the trie." << END_LOG;
                }

                //Make sure that there will be templates instantiated, at least for the given parameter values
                template class ARPATrieBuilder<TrieDriver<LayeredTrieDriver< C2DMapTrie<M_GRAM_LEVEL_MAX, BasicWordIndex> > > >;
                template class ARPATrieBuilder<TrieDriver<LayeredTrieDriver< C2DMapTrie<M_GRAM_LEVEL_MAX, CountingWordIndex> > > >;
                template class ARPATrieBuilder<TrieDriver<LayeredTrieDriver< C2DMapTrie<M_GRAM_LEVEL_MAX, OptimizingWordIndex<BasicWordIndex> > > > >;
                template class ARPATrieBuilder<TrieDriver<LayeredTrieDriver< C2DMapTrie<M_GRAM_LEVEL_MAX, OptimizingWordIndex<CountingWordIndex> > > > >;

                template class ARPATrieBuilder<TrieDriver<LayeredTrieDriver< typename TW2CHybridTrie<M_GRAM_LEVEL_MAX, BasicWordIndex>::type > > >;
                template class ARPATrieBuilder<TrieDriver<LayeredTrieDriver< typename TW2CHybridTrie<M_GRAM_LEVEL_MAX, CountingWordIndex>::type > > >;
                template class ARPATrieBuilder<TrieDriver<LayeredTrieDriver< typename TW2CHybridTrie<M_GRAM_LEVEL_MAX, OptimizingWordIndex<BasicWordIndex> >::type > > >;
                template class ARPATrieBuilder<TrieDriver<LayeredTrieDriver< typename TW2CHybridTrie<M_GRAM_LEVEL_MAX, OptimizingWordIndex<CountingWordIndex> >::type > > >;

                template class ARPATrieBuilder<TrieDriver<LayeredTrieDriver< C2WArrayTrie<M_GRAM_LEVEL_MAX, BasicWordIndex> > > >;
                template class ARPATrieBuilder<TrieDriver<LayeredTrieDriver< C2WArrayTrie<M_GRAM_LEVEL_MAX, CountingWordIndex> > > >;
                template class ARPATrieBuilder<TrieDriver<LayeredTrieDriver< C2WArrayTrie<M_GRAM_LEVEL_MAX, OptimizingWordIndex<BasicWordIndex> > > > >;
                template class ARPATrieBuilder<TrieDriver<LayeredTrieDriver< C2WArrayTrie<M_GRAM_LEVEL_MAX, OptimizingWordIndex<CountingWordIndex> > > > >;

                template class ARPATrieBuilder<TrieDriver<LayeredTrieDriver< W2CArrayTrie<M_GRAM_LEVEL_MAX, BasicWordIndex> > > >;
                template class ARPATrieBuilder<TrieDriver<LayeredTrieDriver< W2CArrayTrie<M_GRAM_LEVEL_MAX, CountingWordIndex> > > >;
                template class ARPATrieBuilder<TrieDriver<LayeredTrieDriver< W2CArrayTrie<M_GRAM_LEVEL_MAX, OptimizingWordIndex<BasicWordIndex> > > > >;
                template class ARPATrieBuilder<TrieDriver<LayeredTrieDriver< W2CArrayTrie<M_GRAM_LEVEL_MAX, OptimizingWordIndex<CountingWordIndex> > > > >;

                template class ARPATrieBuilder<TrieDriver<LayeredTrieDriver< C2DHybridTrie<M_GRAM_LEVEL_MAX, BasicWordIndex> > > >;
                template class ARPATrieBuilder<TrieDriver<LayeredTrieDriver< C2DHybridTrie<M_GRAM_LEVEL_MAX, CountingWordIndex> > > >;
                template class ARPATrieBuilder<TrieDriver<LayeredTrieDriver< C2DHybridTrie<M_GRAM_LEVEL_MAX, OptimizingWordIndex<BasicWordIndex> > > > >;
                template class ARPATrieBuilder<TrieDriver<LayeredTrieDriver< C2DHybridTrie<M_GRAM_LEVEL_MAX, OptimizingWordIndex<CountingWordIndex> > > > >;

                template class ARPATrieBuilder<TrieDriver<G2DMapTrie<M_GRAM_LEVEL_MAX, BasicWordIndex> > >;
                template class ARPATrieBuilder<TrieDriver<G2DMapTrie<M_GRAM_LEVEL_MAX, CountingWordIndex> > >;
                template class ARPATrieBuilder<TrieDriver<G2DMapTrie<M_GRAM_LEVEL_MAX, OptimizingWordIndex<BasicWordIndex> > > >;
                template class ARPATrieBuilder<TrieDriver<G2DMapTrie<M_GRAM_LEVEL_MAX, OptimizingWordIndex<CountingWordIndex> > > >;
            }
        }
    }
}