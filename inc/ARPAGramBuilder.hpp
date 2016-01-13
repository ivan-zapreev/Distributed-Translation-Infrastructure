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

#include "TrieConstants.hpp"
#include "Exceptions.hpp"
#include "MemoryMappedFileReader.hpp"

#include "ModelMGram.hpp"

using namespace std;
using namespace uva::utils::file;
using namespace uva::smt::tries::m_grams;

namespace uva {
    namespace smt {
        namespace tries {
            namespace arpa {

                template<typename WordIndexType> struct TAddGramFunct {
                    typedef std::function<void (const T_Model_M_Gram<WordIndexType>&) > func;
                };

                /**
                 * This class is responsible for splitting a piece of text in a number of ngrams and place it into the trie
                 */
                template<typename WordIndexType, TModelLevel CURR_LEVEL>
                class ARPAGramBuilder {
                public:

                    /**
                     * The constructor to be used in order to instantiate a N-Gram builder
                     * @param word_index the word index to be used
                     * @param level the level of the N-grams to be processed
                     * @param addGarmFunc the strategy for adding the N-grams
                     */
                    ARPAGramBuilder(WordIndexType & word_index, typename TAddGramFunct<WordIndexType>::func addGarmFunc);

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
                     * Takes the uni-gram line and parses it to the word and its probability, no back-off weight!
                     * @param text the piece to read the uni-gram line from
                     * @param word [out] the text piece reader to read the word into
                     * @param prob [out] the variable to set the probability value into
                     * @return true if the uni-gram was successfully parsed
                     */
                    static inline bool unigram_to_prob(TextPieceReader &text, TextPieceReader & word, TLogProbBackOff & prob) {
                        //There should be some text left it it is an M-gram
                        if (text.has_more()) {
                            //First read text until the first tab, it should be present if it is a uni-gram line
                            if (text.get_first_tab(word)) {
                                if (fast_s_to_f(prob, word.get_rest_c_str())) {
                                    //If the uni-gram probability has been parsed, then read the next token which should be the word itself
                                    if (text.get_first_tab(word)) {
                                        //We read the word and so we are done reading this uni-gram the rest is irrelevant
                                        return true;
                                    } else {
                                        //It looks like we are done with reading uni-grams
                                        LOG_DEBUG1 << "Could not read the uni-gram word, looks like we are done!" << END_LOG;
                                        return false;
                                    }
                                } else {
                                    //It looks like we are done with reading uni-grams
                                    LOG_DEBUG1 << "Could not parse the uni-gram probability (" << word.str() << "), looks like we are done!" << END_LOG;
                                    return false;
                                }
                            } else {
                                //It looks like we are done with reading uni-grams
                                LOG_DEBUG1 << "Could read the uni-gram probability as string, looks like we are done!" << END_LOG;
                                return false;
                            }
                        } else {
                            //Unexpected end of text
                            LOG_WARNING << "An unexpected end of line '" << text.str() << "' when reading the uni-gram!" << END_LOG;
                            return false;
                        }
                    }

                    virtual ~ARPAGramBuilder();
                protected:
                    //The function that is to be used to add an N-gram to a trie
                    typename TAddGramFunct<WordIndexType>::func m_add_garm_func;

                    //The temporary storage for read pieces of text
                    TextPieceReader m_token;

                    //This is the N-Gram container to store the parsed N-gram data
                    T_Model_M_Gram<WordIndexType> m_m_gram;

                    //The minimum and maximum number of tokens in the N-Gram string
                    static const unsigned short int MIN_NUM_TOKENS_NGRAM_STR;
                    static const unsigned short int MAX_NUM_TOKENS_NGRAM_STR;

                    /**
                     * Parse the given text into a N-Gram entry from the ARPA file
                     * @param line the piece of text to parse into the M-gram
                     * @return true if the line has been successfully parsed
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

