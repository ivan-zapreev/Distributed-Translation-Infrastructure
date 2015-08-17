/* 
 * File:   ARPANGramBuilderFactory.hpp
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
 * Created on July 27, 2015, 11:17 PM
 */

#ifndef ARPANGRAMBUILDERFACTORY_HPP
#define	ARPANGRAMBUILDERFACTORY_HPP

#include <string> // std::stringstream
#include <ios>    //std::hex

#include "Globals.hpp"
#include "ARPAGramBuilder.hpp"
#include "Exceptions.hpp"
#include "Logger.hpp"

using namespace std;
using namespace uva::smt::exceptions;

namespace uva {
    namespace smt {
        namespace tries {
            namespace arpa {

                /**
                 * This is the ARPA N-gram Builder Factory class that is supposed
                 * to be used to instantiate the proper ARPA N-Gram Builder class.
                 * Note that there can be a small difference in the data provided
                 * for the N-grams of different levels. For example the N-gram of 
                 * the maximum level does not have back-off weights, so knowing 
                 * that can allow for a more optimal reading the data and filling 
                 * in the Trie. Also the first level N-grams (N==1) are just words 
                 * and have to be added as vocabulary words into the Trie and not
                 * as regular N-grams.
                 */
                class ARPAGramBuilderFactory {
                public:

                    /**
                     * This is a template method for getting the proper ARPA
                     * N-gram Builder for the given N-gram level. The two
                     * template parameters of this method N and doCache have
                     * to do with the Trie template parameters. N is the maximum
                     * N-gram level and doCache indicates whether the given Trie
                     * does caching of query results.
                     * 
                     * Note: the returned pointer to the dynamically allocated
                     * builder is to be freed by the caller!
                     * 
                     * @param level the level of the N-gram
                     * @param trie the trie to be filled in with the N-grams
                     * @param pBuilder the pointer to a dynamically allocated N-Gram builder
                     */
                    template<TModelLevel N>
                    static inline void getBuilder(const TModelLevel level, ATrie<N> & trie, ARPAGramBuilder **ppBuilder) {
                        //First reset the pointer to NULL
                        *ppBuilder = NULL;
                        LOG_DEBUG << "Requested a " << level << "-Gram builder, the maximum level is " << N << END_LOG;
                        //Then check that the level values are correct!
                        if (level < MIN_NGRAM_LEVEL || level > N) {
                            stringstream msg;
                            msg << "The requested N-gram level is '" << level
                                    << "', but it must be within [" << MIN_NGRAM_LEVEL << ", " << N << "]!";
                            throw Exception(msg.str());
                        } else {
                            //The N-gram level values are correct, so instantiate an appropriate builder

                            if (level == MIN_NGRAM_LEVEL) {
                                //ToDo: If the level is at minimum it means we are filling in the dictionary
                                LOG_DEBUG1 << "Instantiating the " << MIN_NGRAM_LEVEL << "-Gram builder..." << END_LOG;
                                //Create a builder with the proper lambda as an argument
                                *ppBuilder = new ARPAGramBuilder(level,
                                        [&] (const SBackOffNGram & gram) {
                                            trie.add1Gram(gram); });
                                LOG_DEBUG2 << "DONE Instantiating the " << MIN_NGRAM_LEVEL << "-Gram builder!" << END_LOG;
                            } else {
                                if (level == N) {
                                    //ToDo: If the minimum is at maximum it means we are filling in the top N-gram level
                                    LOG_DEBUG1 << "Instantiating the " << N << "-Gram builder..." << END_LOG;
                                    //Create a builder with the proper lambda as an argument
                                    *ppBuilder = new ARPAGramBuilder(level,
                                            [&] (const SBackOffNGram & gram) {
                                                trie.addNGram(gram); });
                                    LOG_DEBUG2 << "DONE Instantiating the " << N << "-Gram builder!" << END_LOG;
                                } else {
                                    //Here we are to get the builder for the intermediate N-gram levels
                                    LOG_DEBUG1 << "Instantiating the " << level << "-Gram builder.." << END_LOG;
                                    //Create a builder with the proper lambda as an argument
                                    *ppBuilder = new ARPAGramBuilder(level,
                                            [&] (const SBackOffNGram & gram) {
                                                trie.addMGram(gram); });
                                    LOG_DEBUG2 << "DONE Instantiating the " << level << "-Gram builder!" << END_LOG;
                                }
                            }
                        }
                        LOG_DEBUG << "The " << level << "-Gram builder (" << hex << long(*ppBuilder) << ") is produced!" << END_LOG;
                    }

                    virtual ~ARPAGramBuilderFactory() {
                    }
                private:

                    ARPAGramBuilderFactory() {
                    }

                    ARPAGramBuilderFactory(const ARPAGramBuilderFactory & other) {
                    }
                };

            }
        }
    }
}

#endif	/* ARPANGRAMBUILDERFACTORY_HPP */