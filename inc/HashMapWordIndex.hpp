/* 
 * File:   HashMapWordIndex.hpp
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

#ifndef HASHMAPWORDINDEX_HPP
#define	HASHMAPWORDINDEX_HPP


#include <string>   // std::string

#include "AWordIndex.hpp"
#include "Globals.hpp"
#include "Logger.hpp"
#include "Exceptions.hpp"
#include "TextPieceReader.hpp"
#include "GreedyMemoryAllocator.hpp"

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
                class HashMapWordIndex : public AWordIndex {
                public:

                    /**
                     * The basic constructor
                     * @param  wordIndexMemFactor the assigned memory factor for
                     * storage allocation in the unordered_map used for the word index
                     */
                    HashMapWordIndex(const float wordIndexMemFactor)
                    : _pWordIndexAlloc(NULL), _pWordIndexMap(NULL), _nextNewWordId(MIN_KNOWN_WORD_ID), _wordIndexMemFactor(wordIndexMemFactor) {
                    };

                    /**
                     * This method should be used to pre-allocate the word index
                     * @param num_words the number of words
                     */
                    virtual void reserve(const size_t num_words) {
                        //Compute the number of words to be stored
                        const size_t numWords = num_words + 1; //Add an extra element for the <unknown/> word

                        //Reserve the memory for the map
                        reserve_mem_unordered_map<TWordIndexMap, TWordIndexAllocator>(&_pWordIndexMap, &_pWordIndexAlloc, numWords, "WordIndex", _wordIndexMemFactor);

                        //Register the unknown word with the first available hash value
                        TShortId& hash = _pWordIndexMap->operator[](UNKNOWN_WORD_STR);
                        hash = UNKNOWN_WORD_ID;
                    };

                    /**
                     * This function gets an id for the given word word based no the stored 1-Grams.
                     * If the word is not known then an unknown word ID is returned: UNKNOWN_WORD_HASH
                     * @param isThrow if true then throws an exception if the word is not known,
                     * if false then in the same situation will return UNKNOWN_WORD_HASH
                     * @param token the word to hash
                     * @return the resulting hash
                     * @throw out_of_range exception if the given word is not known if isThrow == true,
                     * otherwise return UNKNOWN_WORD_HASH
                     */
                    virtual TShortId getId(const string & token, const bool isThrow = true) const {
                        if (isThrow) {
                            return _pWordIndexMap->at(token);
                        } else {
                            try {
                                return _pWordIndexMap->at(token);
                            } catch (out_of_range e) {
                                LOG_INFO2 << "Word: '" << token << "' is not known! Mapping it to: '"
                                        << UNKNOWN_WORD_STR << "', id: "
                                        << SSTR(UNKNOWN_WORD_ID) << END_LOG;
                            }
                            return UNKNOWN_WORD_ID;
                        }
                    }

                    /**
                     * This function creates/gets a hash for the given word.
                     * Note: The hash id will be unique!
                     * @param token the word to hash
                     * @return the resulting hash
                     */
                    virtual TShortId makeId(const TextPieceReader & token) {
                        //First get/create an existing/new word entry from from/in the word index
                        TShortId& hash = _pWordIndexMap->operator[](token.str());

                        if (hash == UNDEFINED_WORD_ID) {
                            //If the word hash is not defined yet, then issue it a new hash id
                            hash = _nextNewWordId;
                            LOG_DEBUG2 << "Word: '" << token.str() << "' is not known yet, issuing it a new id: " << SSTR(hash) << END_LOG;
                            _nextNewWordId++;
                        }

                        //Use the Prime numbers hashing algorithm as it outperforms djb2
                        return hash;
                    }

                    /**
                     * The basic destructor
                     */
                    virtual ~HashMapWordIndex() {
                        deallocate_container<TWordIndexMap, TWordIndexAllocator>(&_pWordIndexMap, &_pWordIndexAlloc);
                    };
                protected:

                    /**
                     * The copy constructor, is made private as we do not intend to copy this class objects
                     * @param orig the object to copy from
                     */
                    HashMapWordIndex(const HashMapWordIndex & other)
                    : _pWordIndexAlloc(NULL), _pWordIndexMap(NULL),
                    _nextNewWordId(MIN_KNOWN_WORD_ID), _wordIndexMemFactor(0.0) {
                        throw Exception("HashMapWordIndex copy constructor is not to be used, unless implemented!");
                    }

                private:

                    //The type of key,value pairs to be stored in the word index
                    typedef pair< const string, TShortId> TWordIndexEntry;

                    //The typedef for the word index allocator
                    typedef GreedyMemoryAllocator< TWordIndexEntry > TWordIndexAllocator;

                    //The word index map type
                    typedef unordered_map<string, TShortId, std::hash<string>, std::equal_to<string>, TWordIndexAllocator > TWordIndexMap;

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

