/* 
 * File:   MemoryMappedFileReader.hpp
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
 * Created on August 17, 2015, 8:15 PM
 */

#ifndef MEMORYMAPPEDFILEREADER_HPP
#define	MEMORYMAPPEDFILEREADER_HPP

#include <string>       //std::string
#include <unistd.h> 
#include <fcntl.h>      // std::open
#include <unistd.h>     // std::close
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <inttypes.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdint.h>
#include <cstring>
#include <errno.h> 

#include "common/utils/logging/logger.hpp"
#include "common/utils/exceptions.hpp"
#include "common/utils/string_utils.hpp"
#include "common/utils/file/text_piece_reader.hpp"
#include "common/utils/file/afile_reader.hpp"

using namespace std;
using namespace uva::utils::text;

namespace uva {
    namespace utils {
        namespace file {

            //The value of the undefined file descriptor
            static const int UNDEFINED_FILE_DESCRIPTOR = -1;

            /**
             * This is the file reader for the memory mapped file. It is supposed to provide fast memory reads from large files.
             * 
             * For more information on memory mapped files read: https://en.wikipedia.org/wiki/Memory-mapped_file
             * 
             * A possible benefit of memory-mapped files is a "lazy loading", thus using small amounts of RAM even
             * for a very large file. Trying to load the entire contents of a file that is significantly larger
             * than the amount of memory available can cause severe thrashing as the operating system reads from
             * disk into memory and simultaneously writes pages from memory back to disk. Memory-mapping may not
             * only bypass the page file completely, but the system only needs to load the smaller page-sized
             * sections as data is being edited, similarly to demand paging scheme used for programs.
             * 
             * The memory mapping process is handled by the virtual memory manager, which is the same subsystem
             * responsible for dealing with the page file. Memory mapped files are loaded into memory one entire
             * page at a time. The page size is selected by the operating system for maximum performance. Since
             * page file management is one of the most critical elements of a virtual memory system, loading page
             * sized sections of a file into physical memory is typically a very highly optimized system function.
             * 
             * Here is also some nice explanation from: http://stackoverflow.com/questions/1972765/mmap-problem-allocates-huge-amounts-of-memory
             * 
             * Mapping the file into memory  is different to actually reading the file into memory.
             * Were you to read it in, you would have to transfer the entire contents into memory.
             * By mapping it, you let the operating system handle it. If you attempt to read or write
             * to a location in that memory area, the OS will load the relevant section for you first.
             * It will not load the entire file unless the entire file is needed.
             * 
             * That is where you get your performance gain. If you map the entire file but only change
             * one byte then unmap it, you'll find that there's not much disk I/O at all.
             * 
             * Of course, if you touch every byte in the file, then yes, it will all be loaded at some
             * point but not necessarily in physical RAM all at once. But that's the case even if you
             * load the entire file up front. The OS will swap out parts of your data if there's not
             * enough physical memory to contain it all, along with that of the other processes in the system.
             * 
             * The main advantages of memory mapping are:
             * 
             * 1) You defer reading the file sections until they're needed (and, if they're never needed,
             * they don't get loaded). So there's no big upfront cost as you load the entire file.
             * It amortises the cost of loading.
             * 
             * 2) The writes are automated, you don't have to write out every byte. Just close it and the
             * OS will write out the changed sections. I think this also happens when the memory is swapped
             * out as well (in low physical memory situations), since your buffer is simply a window onto the file.
             */
            class MemoryMappedFileReader : public AFileReader {
            private:

                //The file descriptor of the mapped file
                int m_fileDesc;

            public:

                /**
                 * The basic constructor
                 * @param fileName the file name
                 */
                MemoryMappedFileReader(const char * fileName) : AFileReader(), m_fileDesc(0) {
                    m_fileDesc = open(fileName, O_RDONLY);
                    LOG_DEBUG << "Opened the file '" << fileName << "' descriptor: " << SSTR(m_fileDesc) << END_LOG;

                    if (m_fileDesc != UNDEFINED_FILE_DESCRIPTOR) {
                        // set the errno to default value 
                        errno = 0;
                        //The statistics structure for the mapped file
                        struct stat fileStat;
                        if (fstat(m_fileDesc, &fileStat) < 0) {
                            LOG_ERROR << "Could not get the file '" << fileName << "' statistics after loading! ERROR: " << strerror(errno) << END_LOG;
                            //close the file
                            close();
                        } else {
                            //Get the file length
                            const size_t len = fileStat.st_size;
                            LOG_INFO << "Opened the file '" << fileName << "' size: " << SSTR(len) << " bytes." << END_LOG;

                            //Map the file into memory
                            //NOTE: We populate the map as it is then immediately moved to
                            //the RAM and reading the file is then faster and it does not
                            //give a false impression of that we use too much memory in Trie.
                            //  MAP_POPULATE Populate (prefault) page tables for a mapping.
                            //For a file mapping, this causes read-ahead on the file. Later
                            //accesses to the mapping will not be blocked by page faults.
#if __APPLE__
                            void * beginPtr = mmap(NULL, len, PROT_READ, MAP_PRIVATE, m_fileDesc, 0);
#else
                            void * beginPtr = mmap(NULL, len, PROT_READ, MAP_PRIVATE | MAP_POPULATE, m_fileDesc, 0);
#endif
                            LOG_DEBUG << "Memory mapping the file '" << fileName << "' gave: " << SSTR(beginPtr) << " pointer." << END_LOG;

                            //Set the data to the base class
                            TextPieceReader::set(beginPtr, len);
                        }
                    }
                }

                /**
                 * Allows to log the information about the instantiated file reader type
                 */
                virtual void log_reader_type_usage_info() {
                    LOG_USAGE << "Using the <" << __FILENAME__ << "> file reader!" << END_LOG;
                }

                inline bool get_first_line(TextPieceReader& out) {
                    return TextPieceReader::get_first_line(out);
                }
                
                /**
                 * This method is used to check if the file was successfully opened.
                 * @return true if the file is successfully opened otherwise false.
                 */
                virtual bool is_open() const {
                    return (TextPieceReader::get_begin_ptr() != NULL);
                };

                /**
                 * Checks if the file is present.
                 * @return true if it is
                 */
                explicit virtual operator bool() const {
                    return m_fileDesc != UNDEFINED_FILE_DESCRIPTOR;
                }

                /**
                 * This method should be used to close the file
                 */
                virtual void close() {
                    // Release the memory (unnecessary because the program exits).
                    const void * filePtr = TextPieceReader::get_begin_ptr();
                    const size_t len = TextPieceReader::length();
                    if (filePtr != NULL) {
                        LOG_DEBUG << "Releasing the Memory Mapped File memory: ptr = " <<
                                SSTR(filePtr) << ", len = " << len << END_LOG;
                        //Release the memory
                        int err = munmap(const_cast<void*> (filePtr), len);
                        LOG_DEBUG << "Result of munmap is: " << err << END_LOG;
                        //Re-set the internals!
                        set(NULL, 0);
                    }
                    // Close the file descriptor
                    if (m_fileDesc != UNDEFINED_FILE_DESCRIPTOR) {
                        LOG_DEBUG << "Closing the Memory Mapped File file!" << END_LOG;
                        ::close(m_fileDesc);
                        m_fileDesc = UNDEFINED_FILE_DESCRIPTOR;
                    }
                };
            };
        }
    }
}

#endif	/* MMAPPEDFILEREADER_HPP */

