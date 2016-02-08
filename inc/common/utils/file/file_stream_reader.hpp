/* 
 * File:   FileStreamReader.hpp
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
 * Created on August 18, 2015, 8:42 PM
 */

#ifndef FILESTREAMREADER_HPP
#define	FILESTREAMREADER_HPP

#include <cstring>  // std::strlen
#include <fstream>  // std::ifstream

#include "common/utils/logging/logger.hpp"
#include "common/utils/exceptions.hpp"
#include "common/utils/string_utils.hpp"
#include "common/utils/file/text_piece_reader.hpp"

using namespace std;
using namespace uva::utils::text;

namespace uva {
    namespace utils {
        namespace file {

            /**
             * The file reader based on the simple ifstream, should not use as
             * much memory as MemoryMappedFileReader and is seemingly as fast
             * as the latter one on our applications.
             */
            class FileStreamReader : public AFileReader {
            private:
                //Stores the input file stream
                ifstream m_file_stream;
                //Stores the content of the last read file line
                char * m_curr_line;

            public:

                /**
                 * The basic constructor
                 * @param fileName the file name
                 */
                FileStreamReader(const char * fileName)
                : AFileReader(), m_file_stream(fileName, ifstream::in), m_curr_line(NULL) {
                    LOG_DEBUG << "Opened the file '"
                            << fileName << "' is_open: " << (bool) m_file_stream
                            << ", attempting to allocate " << MAX_TEXT_PIECE_LENGTH
                            << " bytes for a buffer" << END_LOG;
                    m_curr_line = new char[MAX_TEXT_PIECE_LENGTH];
                    LOG_DEBUG << "Allocated " << MAX_TEXT_PIECE_LENGTH << " bytes for the line buffer" << END_LOG;
                }
                
                /**
                 * Allows to log the information about the instantiated file reader type
                 */
                virtual void log_reader_type_info() {
                    LOG_USAGE << "Using the <" << __FILENAME__ << "> file reader!" << END_LOG;
                }

                virtual ~FileStreamReader() {
                    if (m_curr_line != NULL) {
                        delete[] m_curr_line;
                        m_curr_line = NULL;
                    }
                }

                virtual void reset() {
                    m_file_stream.clear();
                    m_file_stream.seekg(0, std::ios::beg);
                };

                inline bool get_first_line(TextPieceReader& out) {
                    LOG_DEBUG3 << "Searching for a new line!" << END_LOG;

                    //First read the line from the file
                    if (m_file_stream.getline(m_curr_line, MAX_TEXT_PIECE_LENGTH)) {

                        //Check that it was properly read
                        if (m_file_stream.bad()) {
                            LOG_ERROR << "Error while reading the new line in the file!" << END_LOG;
                            //If there was failure during reading the return a failed flag
                            return false;
                        } else {
                            LOG_DEBUG2 << "Read line '" << m_curr_line << "', length: "
                                    << SSTR(strlen(m_curr_line)) << END_LOG;

                            //The line was properly read, set the values into the output variable
                            out.set(m_curr_line, strlen(m_curr_line));

                            //The line was successfully read, return true
                            return true;
                        }
                    } else {
                        LOG_DEBUG2 << "Got end of file!" << END_LOG;
                        //This is the end of file or something...
                        return false;
                    }
                }

                /**
                 * This method is used to check if the file was successfully opened.
                 * @return true if the file is successfully opened otherwise false.
                 */
                virtual bool is_open() const {
                    return m_file_stream.is_open();
                }

                /**
                 * Checks if the file is present.
                 * @return true if it is
                 */
                virtual operator bool() const {
                    return (bool) m_file_stream;
                }

                /**
                 * This method should be used to close the file
                 */
                virtual void close() {
                    m_file_stream.close();
                };
            };
        }
    }
}

#endif	/* FILESTREAMREADER_HPP */

