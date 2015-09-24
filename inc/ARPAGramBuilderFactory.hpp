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

#include <string>       // std::stringstream
#include <ios>          //std::hex
#include <functional>   // std::function

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
                template<typename TrieType>
                class ARPAGramBuilderFactory {
                public:
                    typedef std::function<void (TrieType & trie, const T_M_Gram&) > TAddGramFunct;

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
                    static inline void get_builder(const TModelLevel level, TrieType & trie, ARPAGramBuilder **ppBuilder) {
                        //First reset the pointer to NULL
                        *ppBuilder = NULL;
                        LOG_DEBUG << "Requested a " << level << "-Gram builder, the maximum level is " << TrieType::max_level << END_LOG;


                        //Then check that the level values are correct!
                        if (DO_SANITY_CHECKS && (level < M_GRAM_LEVEL_1 || level > TrieType::max_level)) {
                            stringstream msg;
                            msg << "The requested N-gram level is '" << level
                                    << "', but it must be within [" << M_GRAM_LEVEL_1
                                    << ", " << TrieType::max_level << "]!";
                            throw Exception(msg.str());
                        } else {
                            //The N-gram level values are correct, so instantiate an appropriate builder

                            if (level == M_GRAM_LEVEL_1) {
                                //If the level is at minimum it means we are filling in the dictionary
                                LOG_DEBUG1 << "Instantiating the " << M_GRAM_LEVEL_1 << "-Gram builder..." << END_LOG;
                                //Create a builder with the proper lambda as an argument
                                *ppBuilder = new ARPAGramBuilder(level,
                                        [&] (const T_M_Gram & gram) {
                                            trie.add_1_gram(gram); });
                                LOG_DEBUG2 << "DONE Instantiating the " << M_GRAM_LEVEL_1 << "-Gram builder!" << END_LOG;
                            } else {
                                if (level == TrieType::max_level) {
                                    //If the minimum is at maximum it means we are filling in the top N-gram level
                                    LOG_DEBUG1 << "Instantiating the " << TrieType::max_level << "-Gram builder..." << END_LOG;
                                    //Create a builder with the proper lambda as an argument
                                    *ppBuilder = new ARPAGramBuilder(level,
                                            [&] (const T_M_Gram & gram) {
                                                trie.add_n_gram(gram); });
                                    LOG_DEBUG2 << "DONE Instantiating the " << TrieType::max_level << "-Gram builder!" << END_LOG;
                                } else {
                                    //Here we are to get the builder for the intermediate N-gram levels
                                    LOG_DEBUG1 << "Instantiating the " << level << "-Gram builder.." << END_LOG;
                                    //Create a builder with the proper lambda as an argument
                                    *ppBuilder = new ARPAGramBuilder(level,
                                            [&] (const T_M_Gram & gram) {
                                                add_m_gram_func[gram.level - ADD_M_GRAM_IDX_OFFSER](trie, gram);
                                            });
                                    LOG_DEBUG2 << "DONE Instantiating the " << level << "-Gram builder!" << END_LOG;
                                }
                            }
                        }
                        LOG_DEBUG << "The " << level << "-Gram builder (" << hex << long(*ppBuilder) << ") is produced!" << END_LOG;
                    }

                    virtual ~ARPAGramBuilderFactory() {
                    }
                private:
                    //The add m-gram functions for levels between 1 and N
                    static const TAddGramFunct add_m_gram_func[];
                    //The level index offset for the array of add_m_gram functions
                    static constexpr TModelLevel ADD_M_GRAM_IDX_OFFSER = 2;

                    ARPAGramBuilderFactory() {
                    }

                    ARPAGramBuilderFactory(const ARPAGramBuilderFactory & other) {
                    }
                };

                template<typename TrieType>
                const typename ARPAGramBuilderFactory<TrieType>::TAddGramFunct ARPAGramBuilderFactory<TrieType>::add_m_gram_func[] = {
                    &TrieType::template add_m_gram<M_GRAM_LEVEL_2>,
                    &TrieType::template add_m_gram<M_GRAM_LEVEL_3>,
                    &TrieType::template add_m_gram<M_GRAM_LEVEL_4>,
                    &TrieType::template add_m_gram<M_GRAM_LEVEL_5>,
                    &TrieType::template add_m_gram<M_GRAM_LEVEL_6>,
                    &TrieType::template add_m_gram<M_GRAM_LEVEL_7>,
                };

            }
        }
    }
}

#endif	/* ARPANGRAMBUILDERFACTORY_HPP */