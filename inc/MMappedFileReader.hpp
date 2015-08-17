/* 
 * File:   MMappedFileReader.hpp
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

#ifndef MMAPPEDFILEREADER_HPP
#define	MMAPPEDFILEREADER_HPP

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

#include "Globals.hpp"
#include "Logger.hpp"
#include "Exceptions.hpp"
#include "StringUtils.hpp"
#include "MTextFilePiece.hpp"

using namespace std;
using namespace uva::smt::utils::text;

namespace uva {
    namespace smt {
        namespace file {

            /**
             * This is the file reader for the memory mapped file. It is supposed to provide fast memory reads from large files
             */
            class MMappedFileReader {
            private:
                //The pointer storing the pointer to the beginning of the mapped file
                char * m_fileBase;
                //The file descriptor of the mapped file
                int m_fileDesc;
                //The statistics structure for the mapped file
                struct stat m_fileStat;
                //The pointer to the unread remainder of the file
                const char * m_cursorPtr;
                //The remaining length of the file to read
                size_t m_restLen;

            public:

                /**
                 * The basic constructor
                 * @param fileName the file name
                 */
                MMappedFileReader(const char * fileName) : m_fileBase(NULL) {
                    m_fileDesc = open(fileName, O_RDONLY);

                    if (m_fileDesc >= 0) {
                        if (fstat(m_fileDesc, &m_fileStat) < 0) {
                            //Map the file into memory
                            m_fileBase = static_cast<char *> (mmap(NULL, m_fileStat.st_size, PROT_READ, MAP_PRIVATE | MAP_POPULATE, m_fileDesc, 0));
                            m_restLen = m_fileStat.st_size;
                        }
                    }
                }

                /**
                 * This method is used to check if the file was successfully opened.
                 * @return true if the file is successfully opened otherwise false.
                 */
                inline bool is_open() {
                    return (m_fileBase != NULL);
                };

                /**
                 * This method should be used to close the file
                 */
                inline void close() {
                    if (is_open()) {
                        // Release the memory (unnecessary because the program exits).
                        munmap(m_fileBase, m_fileStat.st_size);
                        // Close the file descriptor
                        ::close(m_fileDesc);
                    }
                };

                /**
                 * The basic destructor
                 */
                virtual ~MMappedFileReader() {
                    //Just close the file if it has not been closed yet
                    close();
                };

                /**
                 * This function reads a line from the mapped file
                 * @param out the out parameter - the read line 
                 * @return true if a line was read, otherwise false (end of file)
                 */
                inline bool getline(MTextFilePiece& out) {
                    //The next line begins where we stopped
                    out.m_beginPtr = m_cursorPtr;

                    //Search for the next new line symbol in the remainder of the file
                    const char * newLineCharPtr = static_cast<const char *> (memchr(m_cursorPtr, '\n', m_restLen));

                    //Check if we found a pointer to the new line
                    if (newLineCharPtr) {
                        //Compute the line length
                        size_t lineLen = newLineCharPtr - m_cursorPtr;

                        //Store the pointer to the remaining of the file
                        m_cursorPtr = newLineCharPtr + 1;
                        //Store the remaining length of the file
                        m_restLen -= (lineLen + 1);

                        //Set the resulting length of the line
                        out.m_len = lineLen;

                        //For Windows-format text files remove the '\r' as well
                        if (lineLen && '\r' == out.m_beginPtr[lineLen - 1])
                            out.m_len--;
                    } else {
                        //If the pointer is not found then the length of the
                        //last file line is the length of the file remains
                        out.m_len = m_restLen;

                        //If the remaining file length is zero then return file
                        if (!m_restLen) {
                            return false;
                        } else {
                            //If there was something left then now we read it all.
                            //Set the remaining length to zero, as there is nothing left.
                            m_restLen = 0;
                        }
                    }

                    //Return true as we successfully read a new line
                    return true;
                }

            };
        }
    }
}

#endif	/* MMAPPEDFILEREADER_HPP */

