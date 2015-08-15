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

#include <vector>       // std::vector

#include "Logger.hpp"
#include "Exceptions.hpp"
#include "Globals.hpp"

using namespace std;

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
                            _pBuffer = new TStorageData[_numBytes];
                            //Register the first allocated memory buffer
                            _memoryBuffers.push_back(_pBuffer);
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
                        for (std::vector<void*>::iterator it = _memoryBuffers.begin(); it != _memoryBuffers.end(); ++it) {
                            delete [] static_cast<TStorageData*> (*it);
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
                        void* ptr = NULL;

                        //Check if there is more space needed!
                        if (remains > num) {
                            //Allocate the requested memory in the buffer
                            ptr = static_cast<void*> (static_cast<char*> (_pBuffer) + _allocBytes);
                            _allocBytes += num;
                        } else {
                            //WARNING: If we need more space we allocate it additionally in a not so efficient way.
                            LOG_DEBUG3 << "Allocating additional: " << num << " bytes!" END_LOG;
                            void * ptr = static_cast<void*> (new TStorageData[num]);
                            _memoryBuffers.push_back(ptr);
                        }
                        return ptr;
                    }

                protected:
                    //The pre-allocated block of memory
                    void* _pBuffer;

                    //The additionally allocated memory will be stored here
                    vector<void*> _memoryBuffers;

                    //The pre-allocated buffer size
                    const size_type _numBytes;

                    //The number of pre-allocated bytes in the preallocated buffer
                    size_type _allocBytes;
                };

            }
        }
    }
}


#endif	/* FIXEDMEMORYSTORAGE_HPP */

