/* 
 * File:   GreedyMemoryAllocator.hpp
 * Author: Dr. Ivan S. Zapreev
 *
 * Visit my Linked-in profile:
 *      <https://nl.linkedin.com/in/zapreevis>
 * Visit my GitHub:
 *      <https://github.com/ivan-zapreev>
 *
 * The origin of this class is taken from:
 *      <https://github.com/thelazyenginerd/allocator>
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
 * Created on August 14, 2015, 4:43 PM
 */

#ifndef FIXEDMEMORYALLOCATOR_HPP
#define	FIXEDMEMORYALLOCATOR_HPP

#include <typeinfo>  //std::typeid

#include "components/logging/Logger.hpp"
#include "utils/Exceptions.hpp"

#include "utils/containers/GreedyMemoryStorage.hpp"

using namespace uva::utils::logging;
using namespace uva::utils::exceptions;
using namespace uva::utils::containers;

namespace uva {
    namespace utils {
        namespace containers {
            namespace alloc {

                //This is the experimentally obtained memory increase factor for the unordered_map
                //The whole point is that the unordered map does not only reserve memory for key/value
                //pairs but it also usses extra memory for other things, this is why we need a factor
                //for the good initial memory estimate!
                static const float UNORDERED_MAP_MEMORY_FACTOR = 2.6;

                /**
                 * This is helper function that allows to allocate the container, allocator and the actual data storage
                 * Note that, this functions is meant to be used with the unordered_map allocator
                 * @param ppContainer the pointer to the container pointer
                 * @param ppAllocator the pointer to the allocator pointer
                 * @param numEntries the number of entries to pre-allocate for
                 * @param ctName the container name for logging purposes
                 * @param factor the memory multiplication factor, default is UNORDERED_MAP_MEMORY_FACTOR.
                 * This is how many times memory we will allocate (than needed to store numEntries elems) 
                 */
                template<typename TContaner, typename TAllocator>
                void allocate_container(TContaner ** ppContainer, TAllocator ** ppAllocator,
                        const size_t numEntries, const string ctName,
                        const float factor = UNORDERED_MAP_MEMORY_FACTOR) {
                    //Compute the number of bytes needed to store these words
                    const size_t factoredNumElems = numEntries * factor;

                    LOG_DEBUG4 << "Computing the required storage for " << ctName
                            << ", numEntries=" << numEntries << ", factor=" << factor
                            << ", result=" << factoredNumElems << END_LOG;

                    //Allocate the allocator
                    LOG_DEBUG4 << "Allocating the " << ctName << " allocator for " << factoredNumElems << " elements!" << END_LOG;
                    *ppAllocator = new TAllocator(factoredNumElems);

                    //Allocate the map with the given allocator
                    LOG_DEBUG4 << "Allocating the " << ctName << " container with the created allocator!" << END_LOG;
                    *ppContainer = new TContaner(**ppAllocator);
                }

                /**
                 * This is helper function that allows to allocate the container, allocator and the actual data storage
                 * Note that, this functions is meant to be used with the unordered_map allocator
                 * @param ppContainer the pointer to the container pointer
                 * @param ppAllocator the pointer to the allocator pointer
                 * @param numEntries the number of entries to pre-allocate for
                 * @param ctName the container name for logging purposes
                 * @param factor the memory multiplication factor, default is UNORDERED_MAP_MEMORY_FACTOR.
                 * This is how many times memory we will allocate (than needed to store numEntries elems) 
                 */
                template<typename TContaner, typename TAllocator>
                void reserve_mem_unordered_map(TContaner ** ppContainer, TAllocator ** ppAllocator,
                        const size_t numEntries, const string ctName,
                        const float factor = UNORDERED_MAP_MEMORY_FACTOR) {
                    //Call the generic allocation function with the appropriate factor
                    allocate_container<TContaner, TAllocator>(ppContainer, ppAllocator, numEntries, ctName, factor);

                    //Reserve some memory for the buckets!
                    LOG_DEBUG4 << "Reserving " << numEntries << " buckets for the " << ctName << "!" << END_LOG;
                    (*ppContainer)->reserve(numEntries);
                }

                /**
                 * This is helper function that allows to deallocate the container allocator and actual data storage
                 * @param ppContainer the pointer to the container pointer
                 * @param ppAllocator the pointer to the allocator pointer
                 * @param ppStorage the pointer to the storage pointer
                 */
                template<typename TContaner, typename TAllocator >
                void deallocate_container(TContaner ** ppContainer, TAllocator ** ppAllocator) {
                    //First deallocate the memory of the map
                    if (*ppContainer != NULL) {
                        delete *ppContainer;
                        *ppContainer = NULL;
                    }
                    //Second deallocate the allocator
                    if (*ppAllocator != NULL) {
                        delete *ppAllocator;
                        *ppAllocator = NULL;
                    }
                }

                /**
                 * This is the fixed memory allocator class for using in the tries.
                 * Here we pre-allocate some fixed size memory and then just give
                 * it out when needed. Since the Trie is build once and then is not
                 * changed, we do no do any memory deallocation here!
                 */
                template <typename T>
                class GreedyMemoryAllocator {
                public:
                    typedef T value_type;
                    typedef GreedyMemoryStorage::size_type size_type;
                    typedef std::ptrdiff_t difference_type;
                    typedef T* pointer;
                    typedef const T* const_pointer;
                    typedef T& reference;
                    typedef const T& const_reference;

                    template <typename U>
                    struct rebind {
                        typedef GreedyMemoryAllocator<U> other;
                    };

                    /**
                     * The basic constructor.
                     * @param numElems the number of elements of template type T to pre-allocate memory for.
                     */
                    GreedyMemoryAllocator(size_type numElems) throw () :
                    _manager(_storage),
                    _storage(numElems * sizeof (T)) {
                        LOG_DEBUG4 << "Creating FixedMemoryAllocator for "
                                << SSTR(numElems) << " " << typeid (T).name()
                                << " elements of size " << SSTR(sizeof (T)) << END_LOG;
                    }

                    /**
                     * The basic copy constructor. 
                     */
                    GreedyMemoryAllocator(const GreedyMemoryAllocator& other) throw () :
                    _manager(other.getStorageRef()) {
                        LOG_DEBUG4 << "Calling the FixedMemoryAllocator copy constructor." << END_LOG;
                    }

                    /**
                     * The basic re-bind constructor. It is used internally by the
                     * container in case it needs to allocate other sort data than
                     * the stored container elements. 
                     */
                    template <typename U>
                    GreedyMemoryAllocator(const GreedyMemoryAllocator<U>& other) throw () :
                    _manager(other.getStorageRef()) {
                        LOG_DEBUG4 << "Calling the FixedMemoryAllocator re-bind constructor for " << typeid (T).name() << "." << END_LOG;
                    }

                    /**
                     * The standard destructor
                     */
                    virtual ~GreedyMemoryAllocator() throw () {
                        //Nothing to be done!
                        LOG_DEBUG4 << "" << __FUNCTION__ << END_LOG;
                    }

                    /**
                     * Computes the address of the given object
                     * @param obj the object to compute the pointer of
                     * @return the computed pointer
                     */
                    pointer address(reference obj) const {
                        LOG_DEBUG4 << "computing the object address: " << &obj << END_LOG;
                        return &obj;
                    }

                    /**
                     * Computes the address of the given object
                     * @param obj the object to compute the pointer of
                     * @return the computed pointer
                     */
                    const_pointer address(const_reference obj) const {
                        LOG_DEBUG4 << "computing the const object address: " << &obj << END_LOG;
                        return &obj;
                    }

                    /**
                     * Allocates memory for the given number of objects
                     * @param num the number of objects to allocate
                     * @param cp NOT USED
                     * @return the pointer to the first allocated object
                     */
                    pointer allocate(size_type num, const_pointer cp = 0) {
                        const size_type bytes = num * sizeof (T);

                        LOG_DEBUG4 << "Allocating: " << SSTR(num)
                                << " elements for type " << typeid (T).name() << ", of " << SSTR(bytes)
                                << " bytes. Available: " << SSTR(available()) << "/" << SSTR(max_size()) << END_LOG;

                        pointer const ptr = static_cast<pointer> (_manager.allocate(bytes));

                        LOG_DEBUG4 << "The pointer to the first allocated object is: " << ptr << END_LOG;
                        return ptr;
                    }

                    /**
                     * This function is supposed to deallocate the memory.
                     * We do not do that as this is fixed memory allocator
                     * @param ptr the pointer to free memory from
                     * @param num the number of objects to deallocate
                     */
                    void deallocate(pointer ptr, size_type num) {
                        LOG_DEBUG4 << "Requested to deallocate: " << SSTR(num)
                                << " objects starting from: " << ptr << ". Ignoring!" << END_LOG;
                    }

                    /**
                     * Returns the available number of free elements we can store
                     * @return the available number of free elements we can store
                     */
                    size_type available() const throw () {
                        return static_cast<size_type> (_manager.getAvailableBytes() / sizeof (T));
                    }

                    /**
                     * Returns the maximum number of elements we can store
                     * @return the maximum number of elements we can store
                     */
                    size_type max_size() const throw () {
                        return static_cast<size_type> (_manager.getBufferSizeBytes() / sizeof (T));
                    }

                    /**
                     * Calling the constructor on the given pointer
                     * @param ptr the pointer to work with
                     * @param value the type value to work with
                     */
                    void construct(pointer ptr, const value_type& value) {
                        LOG_DEBUG4 << "Calling constructor on: " << ptr << END_LOG;
                        ::new ((void*) ptr) value_type(value);
                    }

                    /**
                     * Calling the destructor on the given pointer
                     * @param ptr the pointer to work with
                     */
                    void destroy(pointer ptr) {
                        LOG_DEBUG4 << "Calling destructor on: " << ptr << END_LOG;
                        ptr->~T();
                    }

                    /**
                     * Returns the reference to the buffer manager
                     * @return the reference to the buffer manager
                     */
                    GreedyMemoryStorage& getStorageRef() const {
                        return _manager;
                    }

                protected:
                    //The reference to the buffer manager that is actively being used
                    GreedyMemoryStorage& _manager;

                private:
                    //The space for a buffer_manager object if this object isn't copy-constructed
                    GreedyMemoryStorage _storage;

                    /**
                     * The default constructor
                     */
                    GreedyMemoryAllocator() throw () :
                    _manager(_storage), _storage(0) {
                        throw Exception("The default constructor is not to be used!");
                    }

                };

                // these are a necessary evil cos some STL classes (e.g. std::basic_string uses them)

                template<typename T, typename U>
                bool operator==(const GreedyMemoryAllocator<T>&, const GreedyMemoryAllocator<U>&) {
                    return true;
                }

                template<typename T>
                bool operator==(const GreedyMemoryAllocator<T>&, const GreedyMemoryAllocator<T>&) {
                    return true;
                }

                template<typename T, typename U>
                bool operator!=(const GreedyMemoryAllocator<T>&, const GreedyMemoryAllocator<U>&) {
                    return false;
                }

                template<typename T>
                bool operator!=(const GreedyMemoryAllocator<T>&, const GreedyMemoryAllocator<T>&) {
                    return false;
                }
            }
        }
    }
}


#endif	/* FIXEDMEMORYALLOCATOR_HPP */

