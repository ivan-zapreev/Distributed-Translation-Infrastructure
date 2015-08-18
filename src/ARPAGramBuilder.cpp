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

                const unsigned short int ARPAGramBuilder::MIN_NUM_TOKENS_NGRAM_STR = 2;
                const unsigned short int ARPAGramBuilder::MAX_NUM_TOKENS_NGRAM_STR = 3;

                ARPAGramBuilder::ARPAGramBuilder(const TModelLevel level, TAddGramFunct addGarmFunc)
                : _addGarmFunc(addGarmFunc), _level(level), _token(), _ngram({0,}) {
                    LOG_DEBUG2 << "Constructing ARPANGramBuilder(" << level << ", trie)" << END_LOG;
                    _ngram.level = _level;
                }

                ARPAGramBuilder::ARPAGramBuilder(const ARPAGramBuilder& orig)
                : _addGarmFunc(orig._addGarmFunc), _level(orig._level), _token(), _ngram(orig._ngram) {
                }

                ARPAGramBuilder::~ARPAGramBuilder() {
                }

                bool ARPAGramBuilder::parseToGram(BasicTextPiece &line, SRawNGram & gram) {
                    //Read the first element until the tab, we read until the tab because it should be the probability
                    if (line.getTab(_token)) {
                        //Try to parse it float
                        if (fast_stoT<float>(_ngram.prob, _token.getRestCStr())) {
                            LOG_DEBUG2 << "Parsed the N-gram probability: " << _ngram.prob << END_LOG;

                            //Read the first (N-1) string of the N-gram - space separated
                            for (int i = 0; i < (_level - 1); i++) {
                                if (!line.getSpace(gram.tokens[i])) {
                                    LOG_WARNING << "An unexpected end of line '" << line.str()
                                            << "' when reading the " << (i + 1)
                                            << "'th " << _level << "-gram token!" << END_LOG;
                                    //The unexpected end of line, broken file format (?)
                                    return false;
                                }
                            }
                            //Read the last N-Gram token, read until the tab as after the 
                            //tab there is a back-off weight or there is no tab in the line
                            if (!line.getTab(gram.tokens[_level - 1])) {
                                LOG_WARNING << "An unexpected end of line '" << line.str()
                                        << "' when reading the " << _level << "'th "
                                        << _level << "-gram token!" << END_LOG;
                                //The unexpected end of line, broken file format (?)
                                return false;
                            }
                            //Now if there is something left it should be the back-off weight, otherwise we are done
                            if (line.isSmthLeft()) {
                                //Take the remainder of the line and try to parse it!
                                if (!fast_stoT<float>(_ngram.back_off, line.getRestCStr())) {
                                    LOG_WARNING << "Could not parse the remainder of the line '" << line.str()
                                            << "' as a back-off weight!" << END_LOG;
                                    //The first token was not a float, need to skip to another N-Gram section(?)
                                    return false;
                                }
                                LOG_DEBUG2 << "Parsed the N-gram back-off weight: " << _ngram.back_off << END_LOG;
                            } else {
                                //There is no back-off so set it to zero
                                _ngram.back_off = ZERO_LOG_PROB_WEIGHT;
                                LOG_DEBUG2 << "The parsed N-gram '" << line.str()
                                        << "' does not have back-off using: " << _ngram.back_off << END_LOG;
                            }
                            return true;
                        } else {
                            //NOTE: Do it as a debug3 level and not a warning because 
                            //this will happen each time we need to move on to a new section!
                            LOG_DEBUG3 << "Could not parse the the string '" << _token.str()
                                    << "' as a probability!" << END_LOG;
                            //The first token was not a float, need to skip to another N-Gram section(?)
                            
                            //Take the line and convert it into a string, then trim it.
                            string line = _token.str();
                            trim(line);
                            //Skip to the next section only if we are dealing with a non-empty line!
                            return (line == "");
                        }
                    } else {
                        LOG_DEBUG3 << "An unexpected end of line '" << line.str()
                                << "', an empty line detected!" << END_LOG;
                        //The unexpected end of line, it is an empty line so we skip it but keep reading this N-gram section
                        return true;
                    }
                }

                bool ARPAGramBuilder::parseLine(BasicTextPiece & line) {
                    LOG_DEBUG << "Processing the " << _level << "-Gram (?) line: '" << line << "'" << END_LOG;
                    //We expect a good input, so the result is set to false by default.
                    bool result = false;

                    //First tokenize as a pattern "prob \t gram \t back-off"
                    if (parseToGram(line, _ngram)) {
                        //Add the obtained N-gram data to the Trie
                        _addGarmFunc(_ngram);
                    } else {
                        //If we could not parse the line to gram then it should
                        //be the beginning of the next m-gram section
                        result = true;
                    }

                    LOG_DEBUG << "Finished processing the " << _level << "-Gram (?) line: '"
                            << line << "', it is " << (result ? "NOT " : "") << "accepted" << END_LOG;

                    return result;
                }
            }
        }
    }
}