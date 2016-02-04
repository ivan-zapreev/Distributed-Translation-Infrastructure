/* 
 * File:   AFileReader.hpp
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
 * Created on August 18, 2015, 8:44 PM
 */

#ifndef AFILEREADER_HPP
#define AFILEREADER_HPP

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
             * This is an abstract base class for the file readers
             * we are going to be using to read model files
             */
            class AFileReader : public TextPieceReader {
            public:

                AFileReader() : TextPieceReader() {
                }

                /**
                 * Allows to check if the file has been open
                 * @return true if the file is open otherwise false
                 */
                virtual bool is_open() const = 0;

                /**
                 * Allows to convert the file reader to a boolean
                 * @return true if the file exists, otherwise false
                 */
                virtual operator bool() const = 0;

                /**
                 * This method creates a string for the "file exists" information
                 * ToDo: Get rid of the file name parameter, the file name should be known by the class instance!
                 * @param fname the file name
                 * @param isPresent true if the file is present
                 * @return the resulting string to be print
                 */
                inline string get_file_exists_string(string const & fname) {
                    string result = (this->operator bool() ? "is present" : "is missing");
                    return fname + " (" + result + ")";
                }

                /**
                 * Allows to log the information about the instantiated file reader type
                 */
                virtual void log_reader_type_usage_info() = 0;

                /**
                 * This method allows to reset the reading process and start reading
                 * the file from th first line again. The default implementation
                 * throws an exception.
                 */
                virtual void reset() {
                    throw Exception("Not implemented for this File reader type!");
                };

                /**
                 * Each file reader implementation will need to override these method, if needed.
                 * The method is non-virtual for performance reasons!
                 */
                bool get_first_line(TextPieceReader& out) {
                    THROW_MUST_OVERRIDE();
                }

                /**
                 * Each file reader implementation will need to override these method, if needed.
                 * The method is non-virtual for performance reasons!
                 */
                bool get_first_space(TextPieceReader& out) {
                    THROW_MUST_OVERRIDE();
                }

                /**
                 * Each file reader implementation will need to override these method, if needed.
                 * The method is non-virtual for performance reasons!
                 */
                bool get_last_space(TextPieceReader& out) {
                    THROW_MUST_OVERRIDE();
                }

                /**
                 * Each file reader implementation will need to override these method, if needed.
                 * The method is non-virtual for performance reasons!
                 */
                bool get_first_tab(TextPieceReader& out) {
                    THROW_MUST_OVERRIDE();
                }

                /**
                 * Allows to close the file
                 */
                virtual void close() {
                };

                /**
                 * The basic destructor, calls the close method
                 */
                virtual ~AFileReader() {
                    //Just close the file if it has not been closed yet
                    close();
                }
            };
        }
    }
}

#endif /* AFILEREADER_HPP */

