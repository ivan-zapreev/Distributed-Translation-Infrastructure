/* 
 * File:   CountingWordIndex.hpp
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
 * Created on September 6, 2015, 2:49 PM
 */

#ifndef COUNTINGWORDINDEX_HPP
#define	COUNTINGWORDINDEX_HPP

#include <string>   // std::string

#include "BasicWordIndex.hpp"

#include "Globals.hpp"
#include "Logger.hpp"
#include "Exceptions.hpp"

#include "ArrayUtils.hpp"
#include "TextPieceReader.hpp"

using namespace std;
using namespace uva::smt::file;
using namespace uva::smt::exceptions;
using namespace uva::smt::utils::array;

namespace uva {
    namespace smt {
        namespace tries {
            namespace dictionary {

                namespace __CountingWordIndex {
                    //This structure is used to store the word 
                    //information such as the word and its count

                    typedef struct {
                        string word;
                        TShortId count;
                    } TWordInfo;

                    /**
                     * The comparison operator for two word info objects,
                     * the one that is smaller has the highest word count.
                     * @param one the first object to compare
                     * @param two the second object to compare
                     * @return the smaller one is the most used one, with the higher word count
                     */
                    inline bool operator<(const TWordInfo & one, const TWordInfo & two) {
                        return (one.count > two.count);
                    }
                }
                
                /**
                 * This is a hash-map based implementation of the word index
                 * which extends the basic word index by word counting. This
                 * allows to count the word usages and then to issue lower
                 * word indexes to the more frequently used words. This allows
                 * for, for example, shorter M-gram ids.
                 */
                class CountingWordIndex : public BasicWordIndex {
                public:

                    /**
                     * The basic constructor
                     * @param  wordIndexMemFactor the assigned memory factor for
                     * storage allocation in the unordered_map used for the word index
                     */
                    CountingWordIndex(const float wordIndexMemFactor) : BasicWordIndex(wordIndexMemFactor) {
                    }

                    /**
                     * This method is to be used when the word counting is needed.
                     * The main application here is to first count the number of
                     * word usages and then distribute the word ids in such a way
                     * that the most used words get the lowest ids.
                     * If not re-implemented throws an exception.
                     * @param token the word to count
                     */
                    virtual void count_word(const TextPieceReader & token) {
                        //Misuse the internal word index map for storing the word counts in it.
                        BasicWordIndex::_pWordIndexMap->operator[](token.str()) += 1;
                    };

                    /**
                     * This method allows to indicate whether word counting is
                     * needed by the given implementation of the word index.
                     * @return true 
                     */
                    virtual bool is_word_count_needed() {
                        return true;
                    };

                    /**
                     * Should be called if the word count is needed
                     * after all the words have been counted.
                     */
                    virtual void post_word_count() {
                        //All the words have been filled in, it is time to give them ids.

                        //01. Create an array of words info objects from BasicWordIndex::_pWordIndexMap
                        const size_t num_words = BasicWordIndex::_pWordIndexMap->size();
                        __CountingWordIndex::TWordInfo * word_infos = new __CountingWordIndex::TWordInfo[num_words];
                        
                        //02. Copy the word information from the map into that array
                        BasicWordIndex::TWordIndexMap::const_iterator iter = BasicWordIndex::_pWordIndexMap->begin();
                        for (size_t idx = 0; iter != BasicWordIndex::_pWordIndexMap->end(); ++iter, ++idx) {
                            word_infos[idx].word = iter->first;
                            word_infos[idx].count = iter->second;
                        }

                        //03. Sort the array of word info object in order to get
                        //    the most used words in the beginning of the array
                        my_sort<__CountingWordIndex::TWordInfo>(word_infos, num_words);

                        //04. Iterate through the array and assign the new word ids
                        //    into the _pWordIndexMap using the BasicWordIndex::_nextNewWordId
                        for(BasicWordIndex::_nextNewWordId = 0;
                                BasicWordIndex::_nextNewWordId < num_words;
                                ++BasicWordIndex::_nextNewWordId) {
                            //Get the next word
                            string & word = word_infos[BasicWordIndex::_nextNewWordId].word;
                            //Give it the next index
                            BasicWordIndex::_pWordIndexMap->operator[](word) = BasicWordIndex::_nextNewWordId;
                        }

                        //05. Delete the temporary sorted array
                        delete[] word_infos;

                        //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                        //WARNING: The minimum word id and the unknown word id are now not fixed any more!
                        //We need to make sure that these constants are not used in the tries! Each time one
                        //Needs the minimum word (?) id or the unknown word id(!) they should ask the word index!
                        //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                    };
                };
            }
        }
    }
}

#endif	/* COUNTINGWORDINDEX_HPP */

