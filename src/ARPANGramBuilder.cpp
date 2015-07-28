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
#include "ARPANGramBuilder.hpp"

#include <string>  //std::string
#include <vector>  //std::vector

#include "Logger.hpp"
#include "StringUtils.hpp"

using namespace uva::smt::logging;
using namespace uva::smt::utils::text;

namespace uva {
    namespace smt {
        namespace tries {
            namespace arpa {

                //This regular expression for (non) signed floats
#define FLOAT_REGEXP "(\\+|-)?(\\d+|\\d+\\.\\d+|\\.\\d+)"
                //This regular expression for words, any sequence of non-whitespace characters
#define WORD_REGEXP "\\S+"
                //Regular expressions for begin and end line    
#define REG_EXT_BEGIN_LINE "^"
#define REG_EXT_END_LINE "$"

                static string getNGramProbOnlyRegExp(const TModelLevel level) {
                    string result = REG_EXT_BEGIN_LINE + string(FLOAT_REGEXP);
                    for (int i = 0; i < level; i++) {
                        result += string("\\s") + WORD_REGEXP;
                    }
                    result += REG_EXT_END_LINE;
                    LOG_DEBUG2 << "The " << level << "-level N-gram regexp without back-off weights is: " << result << END_LOG;
                    return result;
                }

                static string getNGramFullRegExp(const TModelLevel level) {
                    //Get the regular expression for the partial N-gram
                    string result =  getNGramProbOnlyRegExp(level);
                    //Remove the end of line ending REG_EXT_END_LINE
                    result = result.substr(0,result.length()-string(REG_EXT_END_LINE).length());
                    //Extend it with the back-off weight and new end of line REG_EXT_END_LINE
                    result += string("\\s") + FLOAT_REGEXP + REG_EXT_END_LINE;
                    
                    LOG_DEBUG2 << "The " << level << "-level N-gram regexp with back-off weights is: " << result << END_LOG;
                    
                    return result;
                }

                template<TModelLevel N, bool doCache>
                ARPANGramBuilder<N, doCache>::ARPANGramBuilder(const TModelLevel level, ATrie<N, doCache> & trie, const char delim)
                : _trie(trie), _delim(delim), _level(level),
                _ngramFullRegExp(getNGramFullRegExp(level)),
                _ngramProbOnlyRegExp(getNGramProbOnlyRegExp(level)) {
                    LOG_DEBUG2 << "Constructing ARPANGramBuilder<" << N << ", " << doCache << ">(" << level << ", trie," << delim << ")" << END_LOG;
                }

                template<TModelLevel N, bool doCache>
                ARPANGramBuilder<N, doCache>::ARPANGramBuilder(const ARPANGramBuilder<N, doCache>& orig)
                : _trie(orig._trie), _delim(orig._delim), _level(orig._level),
                _ngramFullRegExp(orig._ngramFullRegExp), _ngramProbOnlyRegExp(orig._ngramProbOnlyRegExp) {
                }

                template<TModelLevel N, bool doCache>
                ARPANGramBuilder<N, doCache>::~ARPANGramBuilder() {
                }

                template<TModelLevel N, bool doCache>
                bool ARPANGramBuilder<N, doCache>::processString(const string & data) {
                    LOG_DEBUG << "Processing the " << _level << "-Gram (?) line: '" << data << "'" << END_LOG;
                    //We expect a good input, so the result is set to false by default.
                    bool result = false;

                    //First check if we have a match, and this is indeed a proper N-gram
                    if (regex_match(data, _ngramFullRegExp)) {
                        //ToDo: This is a complete N-gram with Probability and Back-off weight
                    } else {
                        if (regex_match(data, _ngramProbOnlyRegExp)) {
                            //ToDo: This is a partial N-gram with Probability but without the Back-off weight
                        } else {
                            //This is something else, so we should stop building
                            result = true;
                        }
                    }

                    /*
                    //Tokenise the line of text into a vector first
                    vector<string> tokens;
                    tokenize(data, _delim, tokens);

                    //First add all the words to the trie
                    _trie.addWords(tokens);

                    //Create and record all of the N-grams starting from 2 and 
                    //limited either by Trie or by the available number of Tokens
                    const TModelLevel ngLevel = min<unsigned int>(_trie.getNGramLevel(), tokens.size());
                    LOG_DEBUG << "N-gram level = " << ngLevel << END_LOG;
                    for (int n = 2; n <= ngLevel; n++) {
                        for (int idx = 0; idx <= (tokens.size() - n); idx++) {
                            LOG_DEBUG << "adding N-grams (#tokens=" << tokens.size() << ") idx = " << idx << ", len = " << n << END_LOG;
                            _trie.addNGram(tokens, idx, n);
                        }
                    }
                     */

                    LOG_DEBUG << "Finished processing the " << _level << "-Gram (?) line: '"
                            << data << "', it is " << (result ? "NOT" : "") << " accepted" << END_LOG;

                    return result;
                }

                //Make sure that there will be templates instantiated, at least for the given parameter values
                template class ARPANGramBuilder<N_GRAM_PARAM, true>;
                template class ARPANGramBuilder<N_GRAM_PARAM, false>;
            }
        }
    }
}