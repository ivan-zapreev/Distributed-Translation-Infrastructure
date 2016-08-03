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

            //Declare the convert type alias
            using utf8_codec = codecvt_utf8<wchar_t>;

            template<size_t NUM_UTF8_CHARS>
            struct wbuff_struct {
                wchar_t buffer[NUM_UTF8_CHARS];
                int64_t num_chars;
            };

            /**
             * Allows to convert a wide string into a simple string, using utf8
             * @param wstr the wide string
             * @return the simple string in utf8
             */
            static inline string wstring_to_utf8_string(wstring & wstr) {
                //Setup the converter to convert from wide to simple string
                wstring_convert<utf8_codec, wchar_t> utf8_string_converter;

                //Do the conversion
                return utf8_string_converter.to_bytes(wstr);
            }

            /**
             * Allows to convert a null character terminated buffer of
             * wide characters into a utf8 encoded simple string.
             * @param buffer the null terminated buffer of wide characters
             * @return a utf8 encoded simple string
             */
            static inline string wchar_buff_to_utf8_str(wchar_t * buffer) {
                //Convert from the wide string to a simple string
                wstring wchunk(buffer);

                return wstring_to_utf8_string(wchunk);
            }

            /**
             * Allows to read from the stream in utf8 character chunks and process the text.
             * This function begins reading from the beginning of the stream. It throws if
             * the stream is empty or the stream length could not be determined.
             * @param NUM_UTF8_CHARS the maximum number of utf8 characters to read per chunk
             * @param m_input the input stream to read utf8 from.
             * @param process the processor function to be called on each read chunk. Must not throw!
             */
            template<size_t NUM_UTF8_CHARS>
            static inline void process_utf8_chunks(istream & stream, utf8_chunk_processor process_func) {
                //Switch to wide locale independent utf-8 characters
                wbuffer_convert < utf8_codec> buffer_converver(stream.rdbuf());
                wistream wstream(&buffer_converver);
                setlocale(LC_ALL, "C");

                //Make the buffer size one symbol longer, to store the NULL character
                constexpr size_t BUFF_SIZE = NUM_UTF8_CHARS + 1;

                //Read the input into the chunks, counting them
                vector<wbuff_struct<BUFF_SIZE> *> chunks;

                //Declare the variable to count characters
                int64_t num_chars = 0;
                do {
                    //Create a buffer
                    wbuff_struct<BUFF_SIZE> * storage = new wbuff_struct<BUFF_SIZE>();
                    //Read data into the buffer
                    wstream.read(storage->buffer, NUM_UTF8_CHARS);
                    //Store the number of read characters
                    storage->num_chars = wstream.gcount();
                    LOG_DEBUG1 << "Read a chunk of " << storage->num_chars << " utf-8 characters" << END_LOG;
                    //Check if we did read something
                    if (storage->num_chars > 0) {
                        //Add the null character
                        storage->buffer[storage->num_chars] = L'\0';
                        //Store the read buffer
                        chunks.push_back(storage);
                        //Count the read characters
                        num_chars += wstream.gcount();
                    } else {
                        //Delete the buffer if there was nothing read
                        delete storage;
                    }
                } while (!wstream.eof());

                LOG_DEBUG << "Got " << num_chars << " UTF-8 characters to process" << END_LOG;

                //Compute the number of chunks needed
                const size_t num_chunks = chunks.size();
                //Define the chunk index variable
                size_t chunk_idx = 0;

                LOG_DEBUG << "Got " << num_chunks << " chunks to process" << END_LOG;

                //Read from file and send response messages
                for (auto iter = chunks.begin(); iter != chunks.end(); ++iter) {
                    //Convert from the wide character buffer to a simple string
                    string chunk = wchar_buff_to_utf8_str((*iter)->buffer);
                    LOG_DEBUG1 << "Chunk: " << chunk_idx << " > " << chunk << END_LOG;

                    //Process the read chunk
                    process_func(chunk, num_chunks, chunk_idx);

                    //Increment the text piece index
                    ++chunk_idx;
                    //Delete the buffer
                    delete * iter;
                }

                LOG_DEBUG << "Finished processing " << chunk_idx << "/"
                        << num_chunks << " chunks." << END_LOG;
            }
        }
    }
}

#endif /* UTF8_UTILS_HPP */

