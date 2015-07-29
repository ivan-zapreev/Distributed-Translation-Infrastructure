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

                ARPAGramBuilder::ARPAGramBuilder(const TModelLevel level, TAddGramFunct addGarmFunc, const char delim)
                : _addGarmFunc(addGarmFunc), _delim(delim), _level(level),
                MIN_NUM_TOKENS_NGRAM_STR(_level + 1), MAX_NUM_TOKENS_NGRAM_STR(_level + 2) {
                    LOG_DEBUG2 << "Constructing ARPANGramBuilder(" << level << ", trie," << delim << ")" << END_LOG;
                }

                ARPAGramBuilder::ARPAGramBuilder(const ARPAGramBuilder& orig)
                : _addGarmFunc(orig._addGarmFunc), _delim(orig._delim), _level(orig._level),
                MIN_NUM_TOKENS_NGRAM_STR(_level + 1), MAX_NUM_TOKENS_NGRAM_STR(_level + 2) {
                }

                ARPAGramBuilder::~ARPAGramBuilder() {
                }

                bool ARPAGramBuilder::processString(const string & data) {
                    LOG_DEBUG << "Processing the " << _level << "-Gram (?) line: '" << data << "'" << END_LOG;
                    //We expect a good input, so the result is set to false by default.
                    bool result = false;

                    //Clear the current data
                    _ngram.tokens.clear();
                    _ngram.prob = UNDEFINED_LOG_PROB_WEIGHT;
                    _ngram.back_off = UNDEFINED_LOG_PROB_WEIGHT;

                    //Tokenise the line of text into a vector
                    tokenize(data, _delim, _ngram.tokens);
                    //Compute the total number of tokens
                    const size_t size = _ngram.tokens.size();

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
                            _ngram.prob = stof(*_ngram.tokens.begin());
                            //Erase this first token as we only want words as tokens left
                            _ngram.tokens.erase(_ngram.tokens.begin());

                            LOG_DEBUG2 << "Parsed the N-gram probability: " << _ngram.prob << END_LOG;

                            if (size == MAX_NUM_TOKENS_NGRAM_STR) {
                                //If there is two extra tokens then it must be a
                                //probability and a back-off weight. The back-off is
                                //located at the very last place in the string, so it
                                //must be the very last token in the vector - parse it
                                vector<string>::iterator lastElem = --_ngram.tokens.end();
                                _ngram.back_off = stof(*lastElem);
                                //Erase this last token as we only want words as tokens left
                                _ngram.tokens.erase(lastElem);

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
                        result = true;
                    }

                    LOG_DEBUG << "Finished processing the " << _level << "-Gram (?) line: '"
                            << data << "', it is " << (result ? "NOT " : "") << "accepted" << END_LOG;

                    return result;
                }
            }
        }
    }
}