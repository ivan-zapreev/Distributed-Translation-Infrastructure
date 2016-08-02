/* 
 * File:   utf8_utils.hpp
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
 * Created on August 2, 2016, 1:20 PM
 */

#ifndef UTF8_UTILS_HPP
#define UTF8_UTILS_HPP

#include <locale>  // std::setlocale std::wbuffer_convert
#include <codecvt> // std::codecvt_utf8
#include <string>  // std::string
#include <vector>  // std::vector
#include <cmath>   // std::ceil

#include "common/utils/logging/logger.hpp"
#include "common/utils/exceptions.hpp"

using namespace uva::utils::logging;
using namespace uva::utils::exceptions;

namespace uva {
    namespace utils {
        namespace text {

            /**
             * Define the function type for the utf8 chunk processor
             * This function must not throw!
             * @param chunk the string storing the read chunk
             * @param num_chunks the total number of chunks to send 
             * @param chunk_idx the current chunk index starting with 0.
             */
            typedef function<void(const string & chunk, const size_t num_chunks, const size_t chunk_idx) > utf8_chunk_processor;

            /**
             * Allows to read from the stream in utf8 character chunks and process the text.
             * This function begins reading from the beginning of the stream. It throws if
             * the stream is empty or the stream length could not be determined.
             * @param NUM_UTF8_CHARS the maximum number of utf8 characters to read per chunk
             * @param m_input the input stream to read utf8 from. In case of a file stream
             *                must have been opened with "ios::ate" to allow for the file
             *                length computation.
             * @param process the processor function to be called on each read chunk. Must not throw!
             */
            template<size_t NUM_UTF8_CHARS>
            static inline void process_utf8_chunks(istream & stream, utf8_chunk_processor process_func) {
                //Switch to wide locale independent utf-8 characters
                wbuffer_convert < codecvt_utf8<wchar_t>> conv(stream.rdbuf());
                wistream wide_stream(&conv);
                setlocale(LC_ALL, "C");

                //Move to beginning of the stream, just in case 
                wide_stream.seekg(0, wide_stream.beg);

                //Read the input into the chunks, counting them
                vector<wchar_t *> chunks;

                //Declare the variable to count characters
                int64_t num_chars = 0;
                do {
                    //Create a buffer
                    wchar_t * buffer = new wchar_t[NUM_UTF8_CHARS];
                    //Read data into the buffer
                    wide_stream.read(buffer, NUM_UTF8_CHARS);
                    //Check if we did read something
                    if (wide_stream.gcount() > 0) {
                        //Store the read buffer
                        chunks.push_back(buffer);
                        //Count the read characters
                        num_chars += wide_stream.gcount();
                    } else {
                        //Delete the buffer if there was nothing read
                        delete[] buffer;
                    }
                } while (wide_stream);

                LOG_DEBUG << "Got " << num_chars << " UTF-8 characters to process" << END_LOG;

                //Compute the number of chunks needed
                const size_t num_chunks = ceil(((double) num_chars) / NUM_UTF8_CHARS);
                //Define the chunk index variable
                size_t chunk_idx = 0;

                LOG_DEBUG << "Got " << num_chunks << " chunks to process" << END_LOG;

                //Setup the converter to convert back from wide to simple string
                using convert_type = std::codecvt_utf8<wchar_t>;
                std::wstring_convert<convert_type, wchar_t> converter;

                //Read from file and send response messages
                for (auto iter = chunks.begin(); iter != chunks.end(); ++iter) {
                    //Convert from the wide string to a simple string
                    wstring wchunk(*iter);
                    std::string chunk = converter.to_bytes(wchunk);
                    //Process the red chunk
                    process_func(chunk, num_chunks, chunk_idx);
                    //Increment the text piece index
                    ++chunk_idx;
                    //Delete the buffer
                    delete[] * iter;
                }

                LOG_DEBUG << "Finished processing " << chunk_idx << "/" << num_chunks << " chunks." << END_LOG;
            }
        }
    }
}

#endif /* UTF8_UTILS_HPP */

