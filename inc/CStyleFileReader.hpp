/* 
 * File:   FileCStreamReader.hpp
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
 * Created on September 16, 2015, 9:13 PM
 */

#ifndef CSTYLEFILEREADER_HPP
#define	CSTYLEFILEREADER_HPP

#include <cstring>  // std::strlen
#include <cstdio>   // std::fopen std::fseek
#include <stdio.h>  // std::getline
#include <cstdlib>  // std::malloc

#include "Globals.hpp"
#include "Logger.hpp"
#include "Exceptions.hpp"
#include "StringUtils.hpp"
#include "TextPieceReader.hpp"

using namespace std;
using namespace uva::smt::utils::text;

namespace uva {
    namespace smt {
        namespace file {

            /**
             * The file reader based on the simple C stream, should not use as
             * much memory as MemoryMappedFileReader and potentially is faster
             * than the C++ stream based reader.
             */
            class CStyleFileReader : public AFileReader {
            private:
                //Stores the input file stream
                FILE * m_file_ptr;
                //Stores the content of the last read file line
                char * m_buff_ptr;
                //Stores the buffer sizeß
                size_t m_buff_size;

            public:

                /**
                 * The basic constructor
                 * @param fileName the file name
                 */
                CStyleFileReader(const char * fileName)
                : AFileReader(), m_file_ptr(NULL), m_buff_ptr(NULL), m_buff_size(MAX_N_GRAM_STRING_LENGTH) {
                    //Open file for reading
                    m_file_ptr = fopen(fileName, "r");

                    LOG_DEBUG << "Opened the file '"
                            << fileName << "' is_open: " << is_open()
                            << ", attempting to allocate " << m_buff_size
                            << " bytes for a buffer" << END_LOG;

                    //Allocate memory for the buffer
                    m_buff_ptr = (char*) malloc(m_buff_size);

                    LOG_DEBUG << "Allocated " << m_buff_size << " bytes for the line buffer" << END_LOG;
                }
                
                /**
                 * Allows to log the information about the instantiated file reader type
                 */
                virtual void log_reader_type_usage_info() {
                    LOG_USAGE << "Using the <" << __FILE__ << "> file reader!" << END_LOG;
                }

                virtual ~CStyleFileReader() {
                    //Close the file if it is still open
                    close();
                    //Clear memory
                    if (m_buff_ptr != NULL) {
                        free(m_buff_ptr);
                        m_buff_ptr = NULL;
                    }
                }

                virtual void reset() {
                    if (m_file_ptr != NULL) {
                        fseek(m_file_ptr, 0, SEEK_SET);
                    }
                };

                virtual bool getLine(TextPieceReader& out) {
                    LOG_DEBUG3 << "Searching for a new line!" << END_LOG;

                    //First read the line from the file
                    ssize_t length = getline(&m_buff_ptr, &m_buff_size, m_file_ptr);

                    if (length != -1) {
                        //Remove the new line symbol, we do not need it!
                        if((length != 0) && (m_buff_ptr[length - 1] == '\n')) {
                            length = length - 1;
                        }
                        
                        LOG_DEBUG2 << "Read " << length << " symbols: '" << m_buff_ptr << "' !" << END_LOG;
                        //Store the data into the text piece reader
                        out.set(m_buff_ptr, length);
                        return true;
                    } else {
                        LOG_DEBUG2 << "The end of file is reached or an error has occurred!" << END_LOG;
                        //If the end of file is reached or an error occurred -1 is returned 
                        return false;
                    }
                }

                virtual bool getSpace(TextPieceReader& out) {
                    throw Exception("FileStreamReader::getSpace(TextPieceReader& out) must not be used!");
                }

                virtual bool getTab(TextPieceReader& out) {
                    throw Exception("FileStreamReader::getTab(TextPieceReader& out) must not be used!");
                }

                /**
                 * This method is used to check if the file was successfully opened.
                 * @return true if the file is successfully opened otherwise false.
                 */
                virtual bool is_open() const {
                    return (m_file_ptr != NULL);
                }

                /**
                 * Checks if the file is present.
                 * @return true if it is
                 */
                virtual operator bool() const {
                    return is_open();
                }

                /**
                 * This method should be used to close the file
                 */
                virtual void close() {
                    if (m_file_ptr != NULL) {
                        fclose(m_file_ptr);
                        m_file_ptr = NULL;
                    }
                };
            };
        }
    }
}

#endif	/* CSTYLEFILEREADER_HPP */

