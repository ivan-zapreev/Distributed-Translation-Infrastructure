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
#define CSTYLEFILEREADER_HPP

#include <cstring>  // std::strlen
#include <cstdio>   // std::fopen std::fseek
#include <stdio.h>  // std::getline
#include <cstdlib>  // std::malloc

#include "common/utils/logging/logger.hpp"
#include "common/utils/exceptions.hpp"
#include "common/utils/text/string_utils.hpp"
#include "common/utils/file/afile_reader.hpp"

using namespace std;
using namespace uva::utils::text;

namespace uva {
    namespace utils {
        namespace file {

            /**
             * The file reader based on the simple C stream, should not use as
             * much memory as MemoryMappedFileReader and potentially is faster
             * than the C++ stream based reader.
             */
            class cstyle_file_reader : public afile_reader {
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
                cstyle_file_reader(const char * fileName)
                : afile_reader(), m_file_ptr(NULL), m_buff_ptr(NULL), m_buff_size(MAX_TEXT_PIECE_LENGTH) {
                    //Open file for reading
                    m_file_ptr = fopen(fileName, "r");

                    LOG_DEBUG << "Opened the file '"
                            << fileName << "' is_open: " << is_open()
                            << ", attempting to allocate " << m_buff_size
                            << " bytes for a buffer" << END_LOG;

                    //Allocate memory for the buffer
                    m_buff_ptr = (char*) malloc(m_buff_size);
                    memset(m_buff_ptr, 0, m_buff_size);

                    LOG_DEBUG << "Allocated " << m_buff_size << " bytes for the line buffer" << END_LOG;
                }

                /**
                 * The basic constructor
                 * @param file_name the file name
                 */
                cstyle_file_reader(const string & file_name) : cstyle_file_reader(file_name.c_str()) {
                }

                /**
                 * Allows to log the information about the instantiated file reader type
                 */
                virtual void log_reader_type_info() {
                    LOG_USAGE << "Using the <" << __FILENAME__ << "> file reader!" << END_LOG;
                }

                virtual ~cstyle_file_reader() {
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

                inline bool get_first_line(text_piece_reader& out) {
                    LOG_DEBUG3 << "Searching for a new line, m_file_ptr = " << m_file_ptr << END_LOG;

                    //First read the line from the file
                    ssize_t length = getline(&m_buff_ptr, &m_buff_size, m_file_ptr);

                    if (length != -1) {
                        //Remove the new line symbols, we do not need them!
                        while ((length != 0) &&
                                ((m_buff_ptr[length - 1] == '\n') || (m_buff_ptr[length - 1] == '\r'))) {
                            length = length - 1;
                        }

                        LOG_DEBUG3 << "Read " << length << " symbols: '" << m_buff_ptr << "' !" << END_LOG;
                        //Store the data into the text piece reader
                        out.set(m_buff_ptr, length);
                        LOG_DEBUG3 << "The line has been read!" << END_LOG;
                        return true;
                    } else {
                        LOG_DEBUG3 << "The end of file is reached or an error has occurred!" << END_LOG;
                        //If the end of file is reached or an error occurred -1 is returned 
                        return false;
                    }
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

#endif /* CSTYLEFILEREADER_HPP */

