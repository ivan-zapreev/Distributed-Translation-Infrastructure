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
             * Allows to get the number of byte until the end of the unicode character.
             * The first byte of the character is given as an argument
             * @param lb the lower byte of an utf8 character
             * @return the number of bytes of the character yet to read
             */
            static inline uint32_t num_bytes(unsigned char lb) {
                if ((lb & 0x80) == 0) {
                    //The lead bit is zero, this is one byte UTF8 character
                    return 0;
                } else {
                    if ((lb & 0xE0) == 0xC0) {
                        // 110x xxxx, this is a two byte ASCII UTF8 character
                        return 1;
                    } else {
                        if ((lb & 0xF0) == 0xE0) {
                            // 1110 xxxx, this is a three byte ASCII UTF8 character
                            return 2;
                        } else {
                            if ((lb & 0xF8) == 0xF0) {
                                // 1111 0xxx, this is a four byte ASCII UTF8 character
                                return 3;
                            } else {
                                THROW_EXCEPTION(string("Unrecognized UTF8 begin byte: ") + to_string(uint64_t(lb)));
                            }
                        }
                    }
                }
            }

            //Defines the maximum number of the bytes needed for storing an UTF8 character
            static constexpr size_t MAX_NUM_UTF8_CHAR_BYTES = 4;

            /**
             * Allows to convert a stream of bytes into an vector of chunks storing
             * a certain amoung of UTF8 chars. Each chunk is null terminated.
             * The maximum amoung of bytes to be used by a chunk is given by NUM_ASCII_CHARS
             * @param NUM_ASCII_CHARS the number of meaningful byte per chunk,
             *                        excluding the added null termination symbol.
             *                        Must be > 4
             * @param stream the stream to read from
             * @param chunks the reference to the vector of chunks to be build
             */
            template<size_t BUFF_SIZE>
            static inline void stream_to_utf8_chunks(istream & stream,
                    vector<char *> & chunks) {
                //Perform sanity check
                ASSERT_SANITY_THROW((BUFF_SIZE <= MAX_NUM_UTF8_CHAR_BYTES),
                        string("The ASCII buffer must be > ") +
                        to_string(MAX_NUM_UTF8_CHAR_BYTES) + string(" characters"));

                //The actual buffer size is including the zero termination character
                constexpr size_t ACT_BUFF_SIZE = BUFF_SIZE + 1;

                //Define the buffer to be used
                char * buff = new char[ACT_BUFF_SIZE]();
                //Stores the index of the next empty byte in the buffer
                size_t buff_idx = 0;

                //Iterate until we break
                while (true) {
                    //Check if the number of elements is such that a new UTF8 might not fit
                    if (buff_idx + MAX_NUM_UTF8_CHAR_BYTES > BUFF_SIZE) {
                        //Set the null termination character
                        buff[buff_idx] = '\0';
                        //Push the current buffer into the vector
                        chunks.push_back(buff);
                        //Create a new buffer
                        buff = new char[ACT_BUFF_SIZE]();
                        //Re-set the buffer size
                        buff_idx = 0;
                    }

                    //Read the next character from the stream
                    if (stream.get(buff[buff_idx])) {
                        //Get the number of byte 
                        const uint32_t nb = num_bytes(buff[buff_idx]);
                        //Increment the index as we read a new byte
                        ++buff_idx;
                        //Read the remaining character bytes
                        for (size_t idx = 0; idx < nb; ++idx) {
                            if (!stream.get(buff[buff_idx++])) {
                                THROW_EXCEPTION(string("The stream is expected to contain a ") +
                                        to_string(nb + 1) + string(" byte UTF8 symbol by we ") +
                                        string("reached the end of stream!"));
                            }
                        }
                    } else {
                        //There is characters stored in the buffer
                        if (buff_idx > 0) {
                            //If the current buffer stores some data
                            //Set the null termination character
                            buff[buff_idx] = '\0';
                            //Push the current buffer into the vector
                            chunks.push_back(buff);
                        } else {
                            //If there is nothing stored then just deallocate the buffer
                            delete[] buff;
                        }
                        break;
                    }
                }
            }

            /**
             * Allows to read from the stream in utf8 character chunks and process the text.
             * This function begins reading from the beginning of the stream. It throws if
             * the stream is empty or the stream length could not be determined.
             * @param BUFF_SIZE the maximum number of ASCII characters to read per chunk
             * @param m_input the input stream to read utf8 characters from.
             * @param process the processor function to be called on each read chunk. Must not throw!
             */
            template<size_t BUFF_SIZE>
            static inline void process_utf8_chunks(istream & stream, utf8_chunk_processor process_func) {
                //Declare a vector to store the chunks
                vector<char * > chunks;

                //Transform the stream text into the utf8 chunks
                stream_to_utf8_chunks<BUFF_SIZE>(stream, chunks);

                //Compute the number of chunks needed
                const size_t num_chunks = chunks.size();
                //Define the chunk index variable
                size_t chunk_idx = 0;

                LOG_DEBUG << "Got " << num_chunks << " chunks to process" << END_LOG;

                //Read from file and send response messages
                for (auto iter = chunks.begin(); iter != chunks.end(); ++iter) {
                    //Convert from the wide character buffer to a simple string
                    string chunk(*iter);
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

