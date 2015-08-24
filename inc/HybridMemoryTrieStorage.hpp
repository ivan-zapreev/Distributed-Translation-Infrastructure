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
#include <map>              // std::map

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
            typedef TWordId TIndexSize;

            //The type of key,value pairs to be stored
            typedef pair< const TIndexSize, TIndexSize> TStorageMapEntry;
            //The typedef for the map allocator
            typedef GreedyMemoryAllocator< TStorageMapEntry > TStorageMapAllocator;
            //The map type
            typedef unordered_map<TIndexSize, TIndexSize, std::hash<TIndexSize>, std::equal_to<TIndexSize>, TStorageMapAllocator > TStorageMap;

            /**
             * The unordered hash map-based storage for the HybridMemoryTrie
             */
            class CtxToPBUnorderedMapStorage {
            public:

                CtxToPBUnorderedMapStorage(TStorageMapAllocator & alloc) {
                    m_p_map = new TStorageMap(alloc);
                };

                virtual ~CtxToPBUnorderedMapStorage() {
                    delete m_p_map;
                };

                TIndexSize & operator[](const TIndexSize ctx_idx) {
                    return m_p_map->operator[](ctx_idx);
                };

                const TIndexSize & at(const TIndexSize ctx_idx) const throw (out_of_range) {
                    return m_p_map->at(ctx_idx);
                };

            private:
                //The map storage
                TStorageMap * m_p_map;
            };

            /**
             * This is a factory class that should be used to produce containers of CtxToPBMapStorage.
             */
            template<TModelLevel N>
            class CtxToPBUnorderedMapStorageFactory {
            public:

                /**
                 * This is a basic constructor for the factory
                 * @param _counts the number of elements to insert per trie level
                 * @param factor the memory multiplication factor, by default
                 * __CtxToPBMapStorageFactory::UM_CTX_TO_PB_MAP_STORE_MEMORY_FACTOR
                 */
                CtxToPBUnorderedMapStorageFactory(const size_t _counts[N], const float factor = __CtxToPBMapStorageFactory::UM_CTX_TO_PB_MAP_STORE_MEMORY_FACTOR)
                {
                    for (size_t i = 1; i < N; i++) {
                        const GreedyMemoryStorage::size_type size = _counts[i] * factor;
                        m_p_alloc[i - 1] = new TStorageMapAllocator(size);
                        LOG_DEBUG2 << "Allocating a new TStorageMapAllocator("
                                << size << ") for level " << i+1
                                << ", the allocator m_p_alloc[" << (i - 1)
                                << "] = " << SSTR(m_p_alloc[i - 1]) << END_LOG;
                    }
                };

                /**
                 * The basic destructor
                 */
                virtual ~CtxToPBUnorderedMapStorageFactory() {
                    for (size_t i = 1; i < N; i++) {
                        delete m_p_alloc[i - 1];
                    }
                }

                /**
                 * Allocates a new storage container for the given M-gram level
                 * @param level the N-gram level must be > 1 and <= N
                 * @return the pointer to the allocated container
                 */
                CtxToPBUnorderedMapStorage * create(const TModelLevel level) {
                    const TModelLevel idx = level -2;
                    LOG_DEBUG3 << "Allocating a new CtxToPBMapStorage for level "
                            << level << ", the allocator m_p_alloc[" << idx
                            << "] = " << SSTR(m_p_alloc[idx]) << END_LOG;
                    return new CtxToPBUnorderedMapStorage(*m_p_alloc[idx]);
                }

            protected:

                //The array of length ACtxToPBStorage::m_max_level
                TStorageMapAllocator * m_p_alloc[N - 1];
            };


            /**
             * The vector based storage for the HybridMemoryTrie
             */
            class CtxToPBMapStorage {
            public:

                CtxToPBMapStorage() {
                };

                virtual ~CtxToPBMapStorage() {
                };

                TIndexSize & operator[](const TIndexSize ctx_idx) {
                    return m_map[ctx_idx];
                };

                const TIndexSize & at(const TIndexSize ctx_idx) const throw (out_of_range) {
                    return m_map.at(ctx_idx);
                };

            private:
                //The map storage
                map<TIndexSize,TIndexSize>  m_map;
            };

            /**
             * This is a factory class that should be used to produce containers of CtxToPBVectorStorage.
             */
            template<TModelLevel N>
            class CtxToPBMapStorageFactory {
            public:

                /**
                 * This is a basic constructor for the factory
                 * @param _counts the number of elements to insert per trie level
                 */
                CtxToPBMapStorageFactory(const size_t _counts[N])
                {
                };

                /**
                 * The basic destructor
                 */
                virtual ~CtxToPBMapStorageFactory() {
                }

                /**
                 * Allocates a new storage container for the given M-gram level
                 * @param level the N-gram level must be > 1 and <= N
                 * @return the pointer to the allocated container
                 */
                CtxToPBMapStorage * create(const TModelLevel level) {
                    return new CtxToPBMapStorage();
                }
            };
        }
    }
}


#endif	/* HYBRIDMEMORYTRIESTORAGE_HPP */

