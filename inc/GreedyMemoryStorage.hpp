/* 
 * File:   FixedMemoryStorage.hpp
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
 * Created on August 14, 2015, 7:46 PM
 */

#ifndef FIXEDMEMORYSTORAGE_HPP
#define	FIXEDMEMORYSTORAGE_HPP

#include "cstdlib"

#include "Logger.hpp"
#include "Exceptions.hpp"
#include "Globals.hpp"

using uva::smt::logging::Logger;
using uva::smt::exceptions::Exception;

namespace uva {
    namespace smt {
        namespace tries {
            namespace alloc {

                /**
                 * This is the greedy memory storage class that in the first place
                 * allocates some storage and then only grows it if more space is needed!
                 */
                class GreedyMemoryStorage {
                public:

                    //The data type used for data storage elements for the custom allocators
                    typedef uint8_t TStorageData;

                    //The size type to be used
                    typedef std::size_t size_type;

                    /**
                     * The basic constructor of the greedy storage.
                     * @param numBytes the number of bytes to pre-allocate the buffer for - the initial buffer capacity
                     */
                    GreedyMemoryStorage(size_type numBytes) :
                    _numBytes(numBytes),
                    _allocBytes(0) {
                        //Allocate the data buffer
                        LOG_DEBUG << "Pre-Allocating " << numBytes << " bytes storage!" << END_LOG;
                        if (_numBytes > 0) {
                            _pBuffer = malloc(_numBytes * sizeof (TStorageData));
                        } else {
                            _pBuffer = NULL;
                        }
                    }

                    /**
                     * The copy constructor
                     */
                    GreedyMemoryStorage(const GreedyMemoryStorage& source) :
                    _pBuffer(source._pBuffer),
                    _numBytes(source._numBytes),
                    _allocBytes(source._allocBytes) {
                        throw Exception("The GreedyMemoryStorage is not to be copied!");
                    }

                    /**
                     * The basic destructor.
                     */
                    ~GreedyMemoryStorage() {
                        //Deallocate the actual storage
                        if (_pBuffer != NULL) {
                            free(_pBuffer);
                            _pBuffer = NULL;
                        }
                    }

                    /**
                     * Returns the current buffer size
                     * @return the current buffer size
                     */
                    size_type getBufferSizeBytes() const {
                        return _numBytes;
                    }

                    /**
                     * Returns the number of free bytes remaining
                     * @return the number of free bytes remaining
                     */
                    size_type getAvailableBytes() const {
                        return (_numBytes - _allocBytes);
                    }

                    /**
                     * Allocates the memory of required size,if there is no enough space in the buffer, then reallocates!
                     * @param num the number of bytes to allocate in the buffer
                     * @return the pointer to the beginning of the allocated memory block
                     */
                    void* allocate(size_type num) {
                        const size_type remains = getAvailableBytes();
                        
                        //Check if there is more space needed!
                        //WARNING: If yes then we can not reallocate as this can move
                        //the allocated memory and this will screw up many things!
                        if (remains < num) {
                            std::__throw_bad_alloc();
                        }
                        
                        //Allocate the requested memory in the buffer
                        void* cursor = static_cast<void*> (static_cast<char*> (_pBuffer) + _allocBytes);
                        _allocBytes += num;
                        return cursor;
                    }

                protected:
                    // \brief block of memory
                    void* _pBuffer;

                    // \brief buffer size
                    size_type _numBytes;

                    // \brief the number of bytes allocated
                    size_type _allocBytes;
                };

            }
        }
    }
}


#endif	/* FIXEDMEMORYSTORAGE_HPP */

