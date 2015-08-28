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
             * The file reader based on the simple ifstream, should not use as
             * much memory as MemoryMappedFileReader but can be significantly
             * slower that the latter one!
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
                    LOG_DEBUG << SSTR(this) << ": Opened the file '"
                            << fileName << "' is_open: " << (bool) m_file_stream
                            << ", attempting to allocate " << MAX_N_GRAM_STRING_LENGTH
                            << " bytes for a buffer" << END_LOG;
                    m_curr_line = new char[MAX_N_GRAM_STRING_LENGTH];
                    LOG_DEBUG << SSTR(this) << ": Allocated " << MAX_N_GRAM_STRING_LENGTH << " bytes for the line buffer" << END_LOG;

                    LOG_INFO3 << "Using the <" << __FILE__ << "> file reader!" << END_LOG;
                }

                virtual ~FileStreamReader() {
                    if (m_curr_line != NULL) {
                        delete[] m_curr_line;
                        m_curr_line = NULL;
                    }
                }

                virtual bool getLine(TextPieceReader& out) {
                    LOG_DEBUG3 << SSTR(this) << ": Searching for a new line!" << END_LOG;

                    //First read the line from the file
                    m_file_stream.getline(m_curr_line, MAX_N_GRAM_STRING_LENGTH);

                    //Check that it was properly read
                    if (m_file_stream.bad()) {
                        LOG_ERROR << "Error while reading the new line in the file!" << END_LOG;
                        //If there was failure during reading the return a failed flag
                        return false;
                    } else {
                        LOG_DEBUG2 << SSTR(this) << ": Read line '" << m_curr_line << "'" << END_LOG;

                        //The line was properly read, set the values into the output variable
                        out.set(m_curr_line, strlen(m_curr_line));

                        //The line was successfully read, return false only
                        //if the pointer is null, then the file is read
                        return m_curr_line != NULL;
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

