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
#include "ARPAGramBuilder.hpp"

#include <string>    // std::string
#include <vector>    // std::vector
#include <stdexcept> // std::invalid_argument
#include <algorithm> // std::min

#include "Logger.hpp"
#include "StringUtils.hpp"
#include "Exceptions.hpp"

using std::invalid_argument;
using namespace uva::smt::logging;
using namespace uva::smt::utils::text;

namespace uva {
    namespace smt {
        namespace tries {
            namespace arpa {

                const unsigned short int ARPAGramBuilder::MIN_NUM_TOKENS_NGRAM_STR = 1;
                const unsigned short int ARPAGramBuilder::MAX_NUM_TOKENS_NGRAM_STR = 2;

                ARPAGramBuilder::ARPAGramBuilder(const TModelLevel level, TAddGramFunct addGarmFunc)
                : _addGarmFunc(addGarmFunc), _level(level) {
                    LOG_DEBUG2 << "Constructing ARPANGramBuilder(" << level << ", trie)" << END_LOG;
                }

                ARPAGramBuilder::ARPAGramBuilder(const ARPAGramBuilder& orig)
                : _addGarmFunc(orig._addGarmFunc), _level(orig._level) {
                }

                ARPAGramBuilder::~ARPAGramBuilder() {
                }

                bool ARPAGramBuilder::processString(const string & data) {
                    LOG_DEBUG << "Processing the " << _level << "-Gram (?) line: '" << data << "'" << END_LOG;
                    //We expect a good input, so the result is set to false by default.
                    bool result = false;

                    //Clear the current data, set the probability and weight to zero
                    _ngramParts.clear();
                    _ngram.tokens.clear();
                    _ngram.prob = ZERO_LOG_PROB_WEIGHT;
                    _ngram.back_off = ZERO_LOG_PROB_WEIGHT;

                    //First tokenize as a pattern "prob \t gram \t back-off"
                    tokenize(data, '\t', _ngramParts);

                    //Get the number of tokens
                    const size_t size = _ngramParts.size();

                    LOG_DEBUG1 << "The number of tokens is: " << size
                            << ", it is expected to be within ["
                            << MIN_NUM_TOKENS_NGRAM_STR << ","
                            << MAX_NUM_TOKENS_NGRAM_STR << "]" << END_LOG;

                    //Check the number of tokens and by that detect the sort of N-Gram it is
                    if ((MIN_NUM_TOKENS_NGRAM_STR <= size) && (size <= MAX_NUM_TOKENS_NGRAM_STR)) {
                        try {
                            //If there is one extra token then it must be a probability
                            //The probability is located at the very first place in string,
                            //so it must be the very first token in the vector - parse it
                            _ngram.prob = stof(*_ngramParts.begin());

                            LOG_DEBUG2 << "Parsed the N-gram probability: " << _ngram.prob << END_LOG;

                            //Tokenise the gram words
                            tokenize(*(++_ngramParts.begin()), ' ', _ngram.tokens);
                            
                            if (size == MAX_NUM_TOKENS_NGRAM_STR) {
                                //If there is two extra tokens then it must be a
                                //probability and a back-off weight. The back-off is
                                //located at the very last place in the string, so it
                                //must be the very last token in the vector - parse it
                                vector<string>::iterator lastElem = --_ngramParts.end();
                                _ngram.back_off = stof(*lastElem);

                                LOG_DEBUG2 << "Parsed the N-gram back-off weight: " << _ngram.back_off << END_LOG;
                            }

                            //Add the obtained N-gram data to the Trie
                            _addGarmFunc(_ngram);
                        } catch (invalid_argument) {
                            stringstream msg;
                            msg << "The probability or back-off value of the "
                                    << "expected N-gram (?) '" << data << "' of level "
                                    << _level << " could not be parsed !";
                            throw Exception(msg.str());
                        }
                    } else {
                        //This is a possible situation, there is an unexpected
                        //number of tokens, so we should stop with this level N-grams
                        if (size > MAX_NUM_TOKENS_NGRAM_STR) {
                            LOG_WARNING << "There is too many tokens in '" << data
                                    << "' there should be at most two tab symbols! IGNORING!" << END_LOG;
                        } else {
                            //If there is less than the minimum number of tokens then
                            //it should be the beginning of the next m-gram section
                            result = true;
                        }
                    }

                    LOG_DEBUG << "Finished processing the " << _level << "-Gram (?) line: '"
                            << data << "', it is " << (result ? "NOT " : "") << "accepted" << END_LOG;

                    return result;
                }
            }
        }
    }
}