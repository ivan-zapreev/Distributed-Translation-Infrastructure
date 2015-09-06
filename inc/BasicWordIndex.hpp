/* 
 * File:   BasicWordIndex.hpp
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
 * Created on August 21, 2015, 12:41 PM
 */

#ifndef BASICWORDINDEX_HPP
#define	BASICWORDINDEX_HPP

#include <string>   // std::string

#include "AWordIndex.hpp"
#include "Globals.hpp"
#include "Logger.hpp"
#include "Exceptions.hpp"
#include "TextPieceReader.hpp"
#include "GreedyMemoryAllocator.hpp"
#include "HashingUtils.hpp"

using namespace std;
using namespace uva::smt::file;
using namespace uva::smt::hashing;
using namespace uva::smt::exceptions;
using namespace uva::smt::tries::alloc;

namespace uva {
    namespace smt {
        namespace tries {
            namespace dictionary {

                /**
                 * This is a hash-map based implementation of the word index.
                 */
                class BasicWordIndex : public AWordIndex {
                public:

                    /**
                     * The basic constructor
                     * @param  wordIndexMemFactor the assigned memory factor for
                     * storage allocation in the unordered_map used for the word index
                     */
                    BasicWordIndex(const float wordIndexMemFactor)
                    : _pWordIndexAlloc(NULL), _pWordIndexMap(NULL), _nextNewWordId(MIN_KNOWN_WORD_ID), _wordIndexMemFactor(wordIndexMemFactor) {
                    };

                    /**
                     * Allows to get the total words count including the unknown and undefined words
                     * @param num_words the number of words in the language model
                     */
                    virtual size_t get_words_count(const size_t num_words) {
                        return num_words + AWordIndex::EXTRA_NUMBER_OF_WORD_IDs;
                    };

                    /**
                     * This method should be used to pre-allocate the word index
                     * @param num_words the number of words
                     */
                    virtual void reserve(const size_t num_words) {
                        //Compute the number of words to be stored
                        //Add an extra elements for the <unknown/> word
                        const size_t numWords = get_words_count(num_words);

                        //Reserve the memory for the map
                        reserve_mem_unordered_map<TWordIndexMap, TWordIndexAllocator>(&_pWordIndexMap, &_pWordIndexAlloc,
                                numWords, "WordIndex", _wordIndexMemFactor);

                        //Register the unknown word with the first available hash value
                        TShortId& wordId = _pWordIndexMap->operator[](UNKNOWN_WORD_STR);
                        wordId = UNKNOWN_WORD_ID;
                    };

                    /**
                     * This function gets an id for the given word word based no the stored 1-Grams.
                     * If the word is not known then an unknown word ID is returned: UNKNOWN_WORD_ID
                     * @param token the word to hash
                     * @param wordId the resulting wordId or UNKNOWN_WORD_ID if the word is not found
                     * @return true if the word id is found, otherwise false
                     */
                    virtual bool get_word_id(const string & token, TShortId &wordId) const {
                        TWordIndexMapConstIter result = _pWordIndexMap->find(token);
                        if (result == _pWordIndexMap->end()) {
                            LOG_DEBUG << "Word: '" << token << "' is not known! Mapping it to: '"
                                    << UNKNOWN_WORD_STR << "', id: "
                                    << SSTR(UNKNOWN_WORD_ID) << END_LOG;
                            wordId = UNKNOWN_WORD_ID;
                            return false;
                        } else {
                            wordId = result->second;
                            return true;
                        }
                    }

                    /**
                     * This function creates/gets a hash for the given word.
                     * Note: The hash id will be unique!
                     * @param token the word to hash
                     * @return the resulting hash
                     */
                    virtual TShortId register_word(const TextPieceReader & token) {
                        //First get/create an existing/new word entry from from/in the word index
                        TShortId& hash = _pWordIndexMap->operator[](token.str());

                        if (hash == UNDEFINED_WORD_ID) {
                            //If the word hash is not defined yet, then issue it a new hash id
                            hash = _nextNewWordId++;
                            LOG_DEBUG2 << "Word: '" << token.str() << "' is not known yet, issuing it a new id: " << SSTR(hash) << END_LOG;
                        }

                        //Use the Prime numbers hashing algorithm as it outperforms djb2
                        return hash;
                    }

                    /**
                     * The basic destructor
                     */
                    virtual ~BasicWordIndex() {
                        deallocate_container<TWordIndexMap, TWordIndexAllocator>(&_pWordIndexMap, &_pWordIndexAlloc);
                    };
                protected:

                    /**
                     * The copy constructor, is made private as we do not intend to copy this class objects
                     * @param orig the object to copy from
                     */
                    BasicWordIndex(const BasicWordIndex & other)
                    : _pWordIndexAlloc(NULL), _pWordIndexMap(NULL),
                    _nextNewWordId(MIN_KNOWN_WORD_ID), _wordIndexMemFactor(0.0) {
                        throw Exception("HashMapWordIndex copy constructor is not to be used, unless implemented!");
                    }

                    //The type of key,value pairs to be stored in the word index
                    typedef pair< const string, TShortId> TWordIndexEntry;

                    //The typedef for the word index allocator
                    typedef GreedyMemoryAllocator< TWordIndexEntry > TWordIndexAllocator;

                    //The word index map type
                    typedef unordered_map<string, TShortId, std::hash<string>, std::equal_to<string>, TWordIndexAllocator > TWordIndexMap;
                    typedef TWordIndexMap::const_iterator TWordIndexMapConstIter;

                    //This is the pointer to the fixed memory allocator used to allocate the map's memory
                    TWordIndexAllocator * _pWordIndexAlloc;

                    //This map stores the word index, i.e. assigns each unique word a unique id
                    TWordIndexMap * _pWordIndexMap;

                    //Stores the last allocated word hash
                    TShortId _nextNewWordId;

                    //Stores the assigned memory factor for storage allocation
                    //in the unordered_map used for the word index
                    const float _wordIndexMemFactor;

                };
            }
        }
    }
}


#endif	/* HASHMAPWORDINDEX_HPP */

