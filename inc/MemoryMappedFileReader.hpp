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

#include "Globals.hpp"
#include "Logger.hpp"
#include "Exceptions.hpp"
#include "StringUtils.hpp"
#include "BasicTextPiece.hpp"

using namespace std;
using namespace uva::smt::utils::text;

namespace uva {
    namespace smt {
        namespace file {

            /**
             * This is the file reader for the memory mapped file. It is supposed to provide fast memory reads from large files
             */
            class MemoryMappedFileReader : public BasicTextPiece {
            private:
                //The file descriptor of the mapped file
                int m_fileDesc;

            public:

                /**
                 * The basic constructor
                 * @param fileName the file name
                 */
                MemoryMappedFileReader(const char * fileName) : BasicTextPiece() {
                    m_fileDesc = open(fileName, O_RDONLY);

                    if (m_fileDesc >= 0) {
                        //The statistics structure for the mapped file
                        struct stat fileStat;
                        if (fstat(m_fileDesc, &fileStat) < 0) {
                            //Get the file length
                            const size_t len = fileStat.st_size;
                            //Map the file into memory
                            void * beginPtr = mmap(NULL, len, PROT_READ, MAP_PRIVATE | MAP_POPULATE, m_fileDesc, 0);

                            //Set the data to the base class
                            BasicTextPiece::set(beginPtr, len);
                        }
                    }
                }

                /**
                 * This method is used to check if the file was successfully opened.
                 * @return true if the file is successfully opened otherwise false.
                 */
                inline bool is_open() const {
                    return (BasicTextPiece::getBeginPtr() != NULL);
                };

                /**
                 * Checks if the file is present.
                 * @return true if it is
                 */
                explicit inline operator bool() const {
                    return is_open();
                }

                /**
                 * This method should be used to close the file
                 */
                inline void close() {
                    if (is_open()) {
                        // Release the memory (unnecessary because the program exits).
                        munmap(BasicTextPiece::getBeginPtr(), BasicTextPiece::getLen());
                        // Close the file descriptor
                        ::close(m_fileDesc);
                    }
                };

                /**
                 * The basic destructor
                 */
                virtual ~MemoryMappedFileReader() {
                    //Just close the file if it has not been closed yet
                    close();
                };
            };
        }
    }
}

#endif	/* MMAPPEDFILEREADER_HPP */

