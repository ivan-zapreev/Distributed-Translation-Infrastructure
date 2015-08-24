/* 
 * File:   HybridMemoryTrieStorage.hpp
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
 * Created on August 24, 2015, 9:10 AM
 */

#ifndef HYBRIDMEMORYTRIESTORAGE_HPP
#define	HYBRIDMEMORYTRIESTORAGE_HPP

#include <inttypes.h>       // std::uint32_t
#include <utility>          // std::pair, std::make_pair
#include <unordered_map>    // std::unordered_map

#include "Globals.hpp"
#include "Logger.hpp"
#include "AWordIndex.hpp"
#include "GreedyMemoryAllocator.hpp"

using namespace std;
using namespace uva::smt::tries::dictionary;
using namespace uva::smt::tries::alloc;

namespace uva {
    namespace smt {
        namespace tries {
            //This is the id type size to be used as index
            typedef TWordIndexSize TIndexSize;

            /**
             * This is an abstract class that defines an interface for all container
             * types that can be used in the HybridMemoryTrie class for storing
             * the context-to-prob_back_off_index pairs.
             */
            class ACtxToPBStorage {
            public:

                /**
                 * The basic constructor
                 */
                ACtxToPBStorage() {
                }

                /**
                 * The basic destructor
                 */
                virtual ~ACtxToPBStorage() {
                }

                /**
                 * This operator will search for a key ctx_idx and if found will
                 * return the reference to the corresponding value. If not will
                 * create a new entry and return a reference for the uninitialized
                 * value. Does not throw any exceptions
                 * @param ctx_idx the key value
                 * @return the reference to the value
                 */
                virtual TIndexSize & operator[](const TIndexSize ctx_idx) = 0;

                /**
                 * This method will search for a key ctx_idx and if found will
                 * return the reference to the const value. If not found will
                 * throw an exception.
                 * @param ctx_idx the key value
                 * @return the reference to the value
                 * @throws out_of_range
                 */
                virtual const TIndexSize & at(const TIndexSize ctx_idx) const throw (out_of_range) = 0;
            };

            /**
             * This is an abstract class factory class that should be used as a
             * base class for the factories producing instances of ACtxToPBStorage.
             * The M-Gram level must be 1 < M <= N. This is not checked and will
             * cause a segmentation fault if used with M other than specified!
             */
            template<TModelLevel N>
            class ACtxToPBStorageFactory {
            public:

                /**
                 * This is a basic constructor for the factory
                 * @param _counts the number of elements to insert per trie level
                 */
                ACtxToPBStorageFactory(const size_t _counts[N]) {
                };

                /**
                 * The basic destructor
                 */
                virtual ~ACtxToPBStorageFactory() {
                }

                /**
                 * This method should provide a new storage instance for the
                 * given M-gram level M.
                 * 
                 * WARNING: M must be > 1 No allocation for One grams is done!
                 * 
                 * @param level the M-Gram level M
                 * @return the storage instance for this level
                 */
                virtual ACtxToPBStorage * create(const TModelLevel level) = 0;
            };

            //The type of key,value pairs to be stored
            typedef pair< const TIndexSize, TIndexSize> TStorageMapEntry;
            //The typedef for the map allocator
            typedef GreedyMemoryAllocator< TStorageMapEntry > TStorageMapAllocator;
            //The map type
            typedef unordered_map<TIndexSize, TIndexSize, std::hash<TIndexSize>, std::equal_to<TIndexSize>, TStorageMapAllocator > TStorageMap;

            /**
             * The unordered hash map-based storage for the HybridMemoryTrie
             */
            class CtxToPBMapStorage : public ACtxToPBStorage {
            public:
                //Stores the map type
                typedef unordered_map<TIndexSize, TIndexSize> TCtxToPBMapElement;

                CtxToPBMapStorage(TStorageMapAllocator & alloc) : ACtxToPBStorage() {
                    m_p_map = new TStorageMap(alloc);
                };

                virtual ~CtxToPBMapStorage() {
                    delete m_p_map;
                };

                virtual TIndexSize & operator[](const TIndexSize ctx_idx) {
                    return m_p_map->operator[](ctx_idx);
                };

                virtual const TIndexSize & at(const TIndexSize ctx_idx) const throw (out_of_range) {
                    return m_p_map->at(ctx_idx);
                };

            private:
                //The map storage
                TStorageMap * m_p_map;
            };

            /**
             * This is an abstract class factory class that should be used as a
             * base class for the factories producing instances of ACtxToPBStorage.
             */
            template<TModelLevel N>
            class CtxToPBMapStorageFactory : public ACtxToPBStorageFactory<N> {
            public:

                /**
                 * This is a basic constructor for the factory
                 * @param _counts the number of elements to insert per trie level
                 */
                CtxToPBMapStorageFactory(const size_t _counts[N])
                : ACtxToPBStorageFactory<N>(_counts) {
                    for (size_t i = 1; i < N; i++) {
                        m_p_alloc[i - 1] = new TStorageMapAllocator(_counts[i] * UNORDERED_MAP_MEMORY_FACTOR);
                        LOG_DEBUG2 << "Allocating a new TStorageMapAllocator("
                                << _counts[i] * UNORDERED_MAP_MEMORY_FACTOR << ") for level "
                                << i << ", the allocator m_p_alloc[" << (i - 1)
                                << "] = " << SSTR(m_p_alloc[i - 1]) << END_LOG;
                    }
                };

                /**
                 * The basic destructor
                 */
                virtual ~CtxToPBMapStorageFactory() {
                    for (size_t i = 1; i < N; i++) {
                        delete m_p_alloc[i - 1];
                    }
                }

                /**
                 * Allocates a new storage container for the given M-gram level
                 * @param level the N-gram level must be > 1 and <= N
                 * @return the pointer to the allocated container
                 */
                virtual ACtxToPBStorage * create(const TModelLevel level) {
                    LOG_DEBUG3 << "Allocating a new CtxToPBMapStorage for level "
                            << level << ", the allocator m_p_alloc[" << (level - 1)
                            << "] = " << SSTR(m_p_alloc[level - 1]) << END_LOG;
                    return new CtxToPBMapStorage(*m_p_alloc[level - 1]);
                }

            protected:

                //The array of length ACtxToPBStorage::m_max_level
                TStorageMapAllocator * m_p_alloc[N - 1];
            };
        }
    }
}


#endif	/* HYBRIDMEMORYTRIESTORAGE_HPP */

