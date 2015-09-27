/* 
 * File:   NGramBuilder.hpp
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

#ifndef NGRAMBUILDER_HPP
#define	NGRAMBUILDER_HPP

#include <regex>      // std::regex, std::regex_match
#include <functional> // std::function 

#include <GenericTrieDriver.hpp>

#include "Globals.hpp"
#include <Exceptions.hpp>
#include "MemoryMappedFileReader.hpp"

using namespace std;
using namespace uva::smt::file;

namespace uva {
    namespace smt {
        namespace tries {
            namespace arpa {

                template<typename WordIndexType> struct TAddGramFunct {
                    typedef std::function<void (const T_M_Gram<WordIndexType>&) > func;
                };

                /**
                 * This class is responsible for splitting a piece of text in a number of ngrams and place it into the trie
                 */
                template<typename WordIndexType>
                class ARPAGramBuilder {
                public:

                    /**
                     * The constructor to be used in order to instantiate a N-Gram builder
                     * @param word_index the word index to be used
                     * @param level the level of the N-grams to be processed
                     * @param addGarmFunc the strategy for adding the N-grams
                     */
                    ARPAGramBuilder(WordIndexType & word_index, const TModelLevel level, typename TAddGramFunct<WordIndexType>::func addGarmFunc);

                    /**
                     * This pure virtual method is supposed to parse the N-Gram
                     * string from the ARPA file format of a Back-Off language
                     * model and then add the obtained data to the Trie.
                     * This method has a default implementation that should work
                     * for N-grams with level > MIN_NGRAM_LEVEL and level < N
                     * @param data the string to process, has to be space a
                     *             separated sequence of tokens
                     * @result returns true if the provided line is NOT recognized
                     *         as the N-Gram of the specified level.
                     */
                    bool parse_line(TextPieceReader & data);

                    /**
                     * Tokenise a given piece of text into a set of text peices.
                     * The text piece should be a M-gram line from ARPA file with
                     * tab separated probability and back-off and space separated words.
                     * The back-off is optional
                     * @param text the piece of text to tokenise
                     * @param gram the gram container to put data into
                     * @param level the expected M-gram level
                     * @return true if the M-gram was successfully parsed
                     */
                    static inline bool gram_line_to_tokens(TextPieceReader &text, TextPieceReader * tokens, const TModelLevel level) {
                        TextPieceReader storage;
                        //First read text until the first tab, it should be present if it is a M-gram line
                        if (text.getTab(storage)) {
                            //There should be some text left it it is an M-gram
                            if (text.hasMore()) {
                                //Read until the next tab or end of line into the storage
                                if (text.getTab(storage)) {
                                    //Note storage should contain all the space separated M-gram tokens, read them
                                    TModelLevel idx = 0;
                                    while (storage.getSpace(tokens[idx])) {
                                        idx++;
                                    }
                                    //We should read exactly as many tokens as expected
                                    if (idx == level) {
                                        //We read as many tokens as there should be
                                        return true;
                                    } else {
                                        //We read fewer tokens! This is not an M-gram we expected
                                        LOG_WARNING << "Read only " << SSTR(idx) << " words from a "
                                                << SSTR(level) << "-gram: [" << text.str()
                                                << "], expected: " << SSTR(level) << END_LOG;
                                        return false;
                                    }
                                } else {
                                    //Unexpected end of text
                                    LOG_WARNING << "An unexpected end of line '" << text.str()
                                            << "' when reading the " << level << "-gram!" << END_LOG;
                                    return false;
                                }
                            } else {
                                LOG_DEBUG1 << " Encountered [" << SSTR(text.str())
                                        << "] while trying to parse an " << SSTR(level)
                                        << "-gram." << END_LOG;
                                //There is nothing left, this is definitely not an M-gram!
                                return false;
                            }
                        } else {
                            //Unexpected end of text
                            LOG_WARNING << "An unexpected end of line '" << text.str()
                                    << "' when reading the " << level << "-gram!" << END_LOG;
                            return false;
                        }
                    }

                    /**
                     * Tokenise a given piece of text into a set of text peices.
                     * The text piece should be a M-gram piece - a space separated
                     * string of words.
                     * @param text the piece of text to tokenise
                     * @param gram the gram container to put data into
                     */
                    static inline void gram_to_tokens(TextPieceReader &text, T_M_Gram<WordIndexType> & ngram) {
                        //Re-set the level to zero
                        ngram.m_used_level = 0;

                        //Read the tokens one by one and do not forget to increment the level
                        while (text.getSpace(ngram.m_tokens[ngram.m_used_level])) {
                            ngram.m_used_level++;
                        }
                    }

                    virtual ~ARPAGramBuilder();
                protected:
                    //The function that is to be used to add an N-gram to a trie
                    typename TAddGramFunct<WordIndexType>::func m_add_garm_func;

                    //The level of the N-grams to be processed by the given builder
                    const TModelLevel m_level;

                    //The temporary storage for read pieces of text
                    TextPieceReader m_token;

                    //This is the N-Gram container to store the parsed N-gram data
                    T_M_Gram<WordIndexType> m_ngram;

                    //The minimum and maximum number of tokens in the N-Gram string
                    static const unsigned short int MIN_NUM_TOKENS_NGRAM_STR;
                    static const unsigned short int MAX_NUM_TOKENS_NGRAM_STR;

                    /**
                     * Parse the given text into a N-Gram entry from the ARPA file
                     * @param line the piece of text to parse into the M-gram
                     */
                    bool parse_to_gram(TextPieceReader & line);

                    /**
                     * The copy constructor
                     * @param orig the other builder to copy
                     */
                    ARPAGramBuilder(const ARPAGramBuilder & orig);

                };
            }
        }
    }
}
#endif	/* NGRAMBUILDER_HPP */

