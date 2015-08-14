/* 
 * File:   FixedMemoryStorage.hpp
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
 * Created on August 14, 2015, 7:46 PM
 */

#ifndef FIXEDMEMORYSTORAGE_HPP
#define	FIXEDMEMORYSTORAGE_HPP

#include "Logger.hpp"
#include "Exceptions.hpp"
#include "Globals.hpp"

using uva::smt::logging::Logger;
using uva::smt::exceptions::Exception;

namespace uva {
    namespace smt {
        namespace tries {
            namespace alloc {

                // \brief manages the allocation of memory from the buffer

                class FixedMemoryStorage {
                public:
                    typedef std::size_t size_type;

                    // \brief ctor
                    // \param[in] buffer  pointer to the buffer to use for allocation
                    // \param[in] buffer_size  size of the buffer.  make sure they match.

                    FixedMemoryStorage(void* buffer, size_type buffer_size) :
                    m_buffer(buffer),
                    m_buffer_size(buffer_size),
                    m_bytes_allocated(0) {
                        // NOP
                    }

                    // \brief copy ctor
                    // \param[in] source  the source

                    FixedMemoryStorage(const FixedMemoryStorage& source) :
                    m_buffer(source.m_buffer),
                    m_buffer_size(source.m_buffer_size),
                    m_bytes_allocated(source.m_bytes_allocated) {
                        // NOP
                    }

                    // \brief the buffer size in this class

                    size_type buffer_size() const {
                        return m_buffer_size;
                    }

                    // \brief the amount of space remaining in buffer

                    size_type available() const {
                        return (m_buffer_size - m_bytes_allocated);
                    }

                    // \brief allocates a chunk of memory of requested size
                    // \param[in] n   size of chunk in bytes
                    //
                    // TODO are byte sizes in multiples of 4 or 8?

                    void* allocate(size_type num) {
                        const size_type remains = available();
                        if (remains < num) {
                            std::__throw_bad_alloc();
                        }
                        void* cursor = static_cast<void*> (static_cast<char*> (m_buffer) + m_bytes_allocated);
                        m_bytes_allocated += num;
                        return cursor;
                    }

                protected:
                    // \brief block of memory
                    void* const m_buffer;

                    // \brief buffer size
                    const size_type m_buffer_size;

                    // \brief the number of bytes allocated
                    size_type m_bytes_allocated;
                };

            }
        }
    }
}


#endif	/* FIXEDMEMORYSTORAGE_HPP */

