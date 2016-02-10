/* 
 * File:   W2CH_UM_Storage.hpp
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

#ifndef W2CHYBRIDMEMORYTRIESTORAGE_HPP
#define W2CHYBRIDMEMORYTRIESTORAGE_HPP

#include <inttypes.h>       // std::uint32_t
#include <utility>          // std::pair, std::make_pair
#include <unordered_map>    // std::unordered_map
#include <map>              // std::map

#include "server/lm/lm_consts.hpp"
#include "common/utils/logging/logger.hpp"
#include "server/lm/dictionaries/AWordIndex.hpp"
#include "common/utils/containers/greedy_memory_allocator.hpp"

using namespace std;
using namespace uva::smt::bpbd::server::lm::dictionary;
using namespace uva::utils::containers::alloc;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace lm {

                    //The type of key,value pairs to be stored
                    typedef pair< const TShortId, TShortId> TStorageMapEntry;
                    //The typedef for the map allocator
                    typedef GreedyMemoryAllocator< TStorageMapEntry > TStorageMapAllocator;
                    //The unsigned map type
                    typedef unordered_map<TShortId, TShortId, std::hash<TShortId>, std::equal_to<TShortId>, TStorageMapAllocator > TStorageUnsignedMap;
                    //The unsigned map type
                    typedef map<TShortId, TShortId> TStorageMap;

                    /**
                     * The unordered hash map-based storage for the HybridMemoryTrie
                     */
                    class W2CH_UM_Storage {
                    public:

                        typedef TStorageUnsignedMap::const_iterator const_iterator;

                        W2CH_UM_Storage(TStorageMapAllocator & alloc) {
                            m_p_map = new TStorageUnsignedMap(alloc);
                        };

                        virtual ~W2CH_UM_Storage() {
                            delete m_p_map;
                        };

                        inline TShortId & operator[](const TShortId ctx_idx) {
                            return m_p_map->operator[](ctx_idx);
                        };

                        inline const TShortId & at(const TShortId ctx_idx) const throw (out_of_range) {
                            return m_p_map->at(ctx_idx);
                        };

                        inline const_iterator find(const TShortId ctx_idx) {
                            return m_p_map->find(ctx_idx);
                        };

                        inline const_iterator end() {
                            return m_p_map->end();
                        };

                    private:
                        //The map storage
                        TStorageUnsignedMap * m_p_map;
                    };

                    /**
                     * This is a factory class that should be used to produce containers of CtxToPBMapStorage.
                     */
                    template<TModelLevel N>
                    class W2CH_UM_StorageFactory {
                    public:

                        /**
                         * This is a basic constructor for the factory
                         * @param _counts the number of elements to insert per trie level
                         * @param factor the memory multiplication factor, by default
                         * __CtxToPBMapStorageFactory::UM_CTX_TO_PB_MAP_STORE_MEMORY_FACTOR
                         */
                        W2CH_UM_StorageFactory(const size_t _counts[N], const float factor = __W2CHybridTrie::UM_CTX_TO_PB_MAP_STORE_MEMORY_FACTOR) {
                            for (size_t i = 1; i < N; i++) {
                                const GreedyMemoryStorage::size_type size = _counts[i] * factor;
                                m_p_alloc[i - 1] = new TStorageMapAllocator(size);
                                LOG_DEBUG2 << "Allocating a new TStorageMapAllocator("
                                        << size << ") for level " << i + 1
                                        << ", the allocator m_p_alloc[" << (i - 1)
                                        << "] = " << SSTR(m_p_alloc[i - 1]) << END_LOG;
                            }
                        };

                        /**
                         * The basic destructor
                         */
                        virtual ~W2CH_UM_StorageFactory() {
                            for (size_t i = 1; i < N; i++) {
                                delete m_p_alloc[i - 1];
                            }
                        }

                        /**
                         * Allocates a new storage container for the given M-gram level
                         * @param level the N-gram level must be > 1 and <= N
                         * @return the pointer to the allocated container
                         */
                        W2CH_UM_Storage * create(const TModelLevel level) {
                            const TModelLevel idx = level - 2;
                            LOG_DEBUG3 << "Allocating a new CtxToPBMapStorage for level "
                                    << level << ", the allocator m_p_alloc[" << idx
                                    << "] = " << SSTR(m_p_alloc[idx]) << END_LOG;
                            return new W2CH_UM_Storage(*m_p_alloc[idx]);
                        }

                    protected:

                        //The array of length ACtxToPBStorage::m_max_level
                        TStorageMapAllocator * m_p_alloc[N - 1];
                    };
                }
            }
        }
    }
}

#endif /* HYBRIDMEMORYTRIESTORAGE_HPP */

