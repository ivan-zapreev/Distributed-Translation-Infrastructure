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
#include "common/utils/text/string_utils.hpp"
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
            class afile_reader : public text_piece_reader {
            public:

                afile_reader() : text_piece_reader() {
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
                 * Allows to log the information about the instantiated file reader type
                 */
                virtual void log_reader_type_info() = 0;

                /**
                 * This method allows to reset the reading process and start reading
                 * the file from th first line again. The default implementation
                 * throws an exception.
                 */
                virtual void reset() {
                    THROW_EXCEPTION("Not implemented for this File reader type!");
                };

                /**
                 * This function searches forward for the first occurrence of the
                 * argument delimiter symbol.
                 */
                template<const char delim, const uint8_t delim_len = 1 >
                inline bool get_first(text_piece_reader& out) {
                    THROW_MUST_OVERRIDE();
                }

                /**
                 * This function searches backwards for the first occurrence of the
                 * argument delimiter symbol.
                 */
                template<const char delim, const uint8_t delim_len = 1 >
                inline bool get_last(text_piece_reader& out) {
                    THROW_MUST_OVERRIDE();
                }

                /**
                 * Each file reader implementation will need to override these method, if needed.
                 * The method is non-virtual for performance reasons!
                 */
                bool get_first_line(text_piece_reader& out) {
                    THROW_MUST_OVERRIDE();
                }

                /**
                 * Each file reader implementation will need to override these method, if needed.
                 * The method is non-virtual for performance reasons!
                 */
                bool get_first_space(text_piece_reader& out) {
                    THROW_MUST_OVERRIDE();
                }

                /**
                 * Each file reader implementation will need to override these method, if needed.
                 * The method is non-virtual for performance reasons!
                 */
                bool get_last_space(text_piece_reader& out) {
                    THROW_MUST_OVERRIDE();
                }

                /**
                 * Each file reader implementation will need to override these method, if needed.
                 * The method is non-virtual for performance reasons!
                 */
                bool get_first_tab(text_piece_reader& out) {
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
                virtual ~afile_reader() {
                    //Just close the file if it has not been closed yet
                    close();
                }
            };
        }
    }
}

#endif /* AFILEREADER_HPP */

