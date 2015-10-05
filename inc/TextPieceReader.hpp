/* 
 * File:   TextPieceReader.hpp
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
 * Created on August 17, 2015, 10:47 PM
 */

#ifndef TEXTPIECEREADER_HPP
#define	TEXTPIECEREADER_HPP

#include <string.h>     // std::memrchr
#include <cstring>      // std::memchr std::strncpy
#include <algorithm>    // std::min

#include "Globals.hpp"
#include "Logger.hpp"
#include "Exceptions.hpp"
#include "StringUtils.hpp"

using namespace std;
using namespace uva::smt::tries;
using namespace uva::smt::exceptions;
using namespace uva::smt::logging;
using namespace uva::smt::utils::text;

namespace uva {
    namespace smt {
        namespace file {

            //The maximum length of the text that will be managed by this class as a string convertable
            static const size_t MAX_N_GRAM_STRING_LENGTH = 2048;
            //The text is too large string to be used in conversion
            static const string TEXT_TOO_LARGE_STR("<text-too-large>");
            //This stores the NOTHING string to be used in conversion
            static const string TEXT_NOTHING_STR("<NULL>");

            /**
             * This basic storage class that stores the pointer to pre-allocated memory
             * plus the length of this piece. This class can be used to represent a
             * piece of text, a line or a word or any arbitrary piece of memory.
             * Note that, the string here is not necessarily \0 terminated and the
             * text memory can be Gb large! Also the memory is not managed by the class.
             */
            class TextPieceReader {
            private:
                //The pointer to the first text character.
                //The text is NOT necessarily \0 terminated and can be Gb large!
                const char * m_beginPtr;
                //The length of the line
                size_t m_len;

                //Contains true if the string is to be generated, otherwise false
                mutable bool m_is_gen_str;
                //The string representation of the given text piece
                mutable string m_str;

                //The pointer to the unread remainder of the file
                const char * m_cursor_ptr;
                //The remaining length of the file to read
                size_t m_rest_len;

            public:

                /**
                 * The basic constructor initializes empty text
                 */
                TextPieceReader()
                : m_beginPtr(NULL), m_len(0), m_is_gen_str(true), m_str(""), m_cursor_ptr(NULL), m_rest_len(0) {
                }

                /**
                 * The constructor.
                 * @param beginPtr the pointer to the begin of the text
                 * @param len the length of the text
                 */
                explicit TextPieceReader(void * beginPtr, const size_t len)
                : m_beginPtr(NULL), m_len(0), m_is_gen_str(true), m_str(""), m_cursor_ptr(NULL), m_rest_len(0) {
                    set(beginPtr, len);
                }

                /**
                 * The copy constructor.
                 * @param other the const reference to the object to copy from
                 */
                TextPieceReader(const TextPieceReader & other) {
                    m_beginPtr = other.m_beginPtr;
                    m_len = other.m_len;
                    m_is_gen_str = other.m_is_gen_str;
                    m_str = other.m_str;
                    m_cursor_ptr = other.m_cursor_ptr;
                    m_rest_len = other.m_rest_len;
                }

                /**
                 * Allows to set the text
                 * @param beginPtr the pointer to the beginning of the text
                 * @param len the length of the text
                 */
                inline void set(const void * beginPtr, const size_t len) {
                    m_beginPtr = static_cast<const char *> (beginPtr);
                    m_len = len;
                    m_is_gen_str = true;
                    m_str.clear();
                    m_cursor_ptr = m_beginPtr;
                    m_rest_len = m_len;

                    LOG_DEBUG3 << "Setting the data to BasicTextPiece: m_beginPtr = "
                            << SSTR(static_cast<const void*> (m_beginPtr)) << ", m_cursorPtr = "
                            << SSTR(static_cast<const void*> (m_cursor_ptr)) << ", m_is_gen_str = "
                            << m_is_gen_str << ", m_len = " << SSTR(m_len)
                            << ", m_restLen = " << SSTR(m_rest_len) << END_LOG;
                }

                /**
                 * Allows to get the pointer to the beginning of the text
                 * This is a C string that is returned BUT there is no \0
                 * termination and it can be Gb long!
                 * @return the pointer to the beginning of the text
                 */
                inline const char * get_begin_c_str() const {
                    return m_beginPtr;
                }

                /**
                 * Allows to get the pointer to the remainder of the text
                 * This is a C string that is returned BUT there is no \0
                 * termination and it can be Gb long!
                 * @return the pointer to the remainder of the text
                 */
                inline const char * get_rest_c_str() const {
                    return m_cursor_ptr;
                }

                /**
                 * Allows to get the pointer to the beginning of the text
                 * @return the pointer to the beginning of the text
                 */
                inline const void * get_begin_ptr() const {
                    return (void *) m_beginPtr;
                }

                /**
                 * Allows to get the length of the text
                 * @return the length of the text
                 */
                inline size_t length() const {
                    return m_len;
                }

                /**
                 * This method allows to copy the string of one text piece into another.
                 * The copying process re-sets the internal cursor and remaining length to read.
                 * @param other the element to copy from
                 * @param limit the maximum length allowed to be copied from the source (other)
                 * if the source length is larger - an exception will be raised!
                 */
                template<const size_t LEN_LIMIT>
                inline void copy_string(const TextPieceReader& other) {
                    if (DO_SANITY_CHECKS && (LEN_LIMIT < other.m_len)) {
                        stringstream msg;
                        msg << "Unable to copy the string data to the target, "
                                << "maximum copy length is " << LEN_LIMIT
                                << ", the source length is " << other.m_len << "!";
                        throw Exception(msg.str());
                    } else {
                        //Copy the data
                        (void) strncpy(const_cast<char *> (m_beginPtr),
                                other.m_beginPtr, (size_t) other.m_len);
                        //Re-set the other members using the available standard method
                        set(m_beginPtr, other.m_len);
                    }
                }

                /**
                 * This function searches forward for the first occurrence of the
                 * argument delimiter symbol.
                 * @param out the out parameter - the substring until the first next
                 * found delimiter or the entire string if the delimiter was not found
                 * @return true if a line was read, otherwise false (end of file)
                 */
                template<const char delim>
                inline bool get_first(TextPieceReader& out) {
                    //The next piece begins where we stopped
                    const char * out_m_begin_ptr = m_cursor_ptr;

                    LOG_DEBUG3 << SSTR(static_cast<const void *> (out_m_begin_ptr)) << END_LOG;

                    //The found piece length is first zero
                    size_t out_m_len = 0;

                    //Search for the next new delimiter from the front
                    const char * char_ptr = static_cast<const char *> (memchr(m_cursor_ptr, delim, m_rest_len));

                    LOG_DEBUG4 << "Forward searching for the character got: "
                            << SSTR(static_cast<const void *> (char_ptr)) << END_LOG;

                    //Check if we found something
                    if (char_ptr != NULL) {
                        //Compute the length
                        size_t found_piece_length = char_ptr - m_cursor_ptr;

                        LOG_DEBUG4 << "The substring length is " << SSTR(found_piece_length) << END_LOG;

                        //Store the pointer to the remaining text piece
                        m_cursor_ptr = char_ptr + 1;
                        //Store the remaining length
                        m_rest_len -= (found_piece_length + 1);

                        LOG_DEBUG4 << "Resetting m_cursor_ptr = "
                                << SSTR(static_cast<const void *> (m_cursor_ptr))
                                << ", m_rest_len = " << m_rest_len << END_LOG;

                        //Set the resulting length
                        out_m_len = found_piece_length;

                        //If we are looking for end of line, for Windows-format strings, remove the '\r' as well
                        if ((delim == '\n') && found_piece_length && (out_m_begin_ptr[found_piece_length - 1]) == '\r') {
                            out_m_len--;
                            LOG_DEBUG4 << "A \\\\r detected, resetting out_m_len = " << out_m_len << END_LOG;
                        }
                    } else {
                        //If the pointer is not found then the length if the entire remaining length
                        out_m_len = m_rest_len;

                        //If the remaining length is zero then return false as there is nothing to return
                        if (!m_rest_len) {
                            return false;
                        } else {
                            //If there was something left then now we read it all.
                            //Set the remaining length to zero, as there is nothing left.
                            m_rest_len = 0;
                        }
                    }

                    //Set the return data
                    out.set(out_m_begin_ptr, out_m_len);

                    //Return true as we successfully found a delimited substring
                    return true;
                }

                /**
                 * This function searches backwards for the first occurrence of the
                 * argument delimiter symbol.
                 * @param out the out parameter - the substring from the first next
                 * found delimiter till the end of the string or the entire string
                 * if the delimiter was not found
                 * @return true if a line was read, otherwise false (end of file)
                 */
                template<const char delim>
                inline bool get_last(TextPieceReader& out) {
                    //The found piece length is first zero
                    size_t out_m_len = 0;
                    
                    LOG_DEBUG3 << "Start searching for a new delimiter, m_cursor_ptr: "
                            << SSTR(static_cast<const void *> (m_cursor_ptr))
                            << ", m_rest_len: " << m_rest_len << END_LOG;

                    //Search for the next new delimiter from the end
                    const char * out_m_begin_ptr = static_cast<const char *> (memrchr(m_cursor_ptr, delim, m_rest_len));

                    LOG_DEBUG4 << "Backward searching for the character got: "
                            << SSTR(static_cast<const void *> (out_m_begin_ptr)) << END_LOG;

                    //Check if we found something
                    if (out_m_begin_ptr != NULL) {
                        //Store the current remaining length 
                        out_m_len = m_rest_len;

                        //Set the new remaining length and move the begin pointer past the delimiter
                        m_rest_len = (out_m_begin_ptr++ - m_cursor_ptr);

                        LOG_DEBUG4 << "Resetting m_cursor_ptr = "
                                << SSTR(static_cast<const void *> (m_cursor_ptr))
                                << ", m_rest_len = " << m_rest_len << END_LOG;

                        //If we are looking for end of line, for Windows-format strings, remove the '\r' as well
                        if ((delim == '\n') && m_rest_len && (m_cursor_ptr[m_rest_len - 1] == '\r')) {
                            m_rest_len--;
                            LOG_DEBUG4 << "A \\\\r detected, resetting m_rest_len = " << m_rest_len << END_LOG;
                        }

                        //Set the resulting length for the found piece
                        out_m_len -= (out_m_begin_ptr - m_cursor_ptr);
                    } else {
                        //If the pointer is not found then the length if the entire remaining length
                        out_m_len = m_rest_len;
                        //Also the pointer should then point to the beginning of the text we have
                        out_m_begin_ptr = m_cursor_ptr;

                        //If the remaining length is zero then return false as there is nothing to return
                        if (!m_rest_len) {
                            return false;
                        } else {
                            //If there was something left then now we read it all.
                            //Set the remaining length to zero, as there is nothing left.
                            m_rest_len = 0;
                        }
                    }

                    //Set the return data
                    out.set(out_m_begin_ptr, out_m_len);

                    //Return true as we successfully found a delimited substring
                    return true;
                }

                /**
                 * Allows to check if there is something left to read
                 * @return true if there is yet something to read, otherwise false
                 */
                inline bool has_more() {
                    return m_rest_len > 0;
                }

                /**
                 * This function, searches forward for the first end of line char
                 * or until the end of the text and then sets the data about the found
                 * region into the provided output parameter.
                 * @param out the out parameter - the read line 
                 * @return true if data was read, otherwise false
                 */
                virtual bool get_first_line(TextPieceReader& out) {
                    LOG_DEBUG4 << "Searching forward for a new line!" << END_LOG;
                    return get_first<'\n'>(out);
                }

                /**
                 * This function, searches forward for the first end of space char
                 * or until the end of the text and then sets the data about the found
                 * region into the provided output parameter.
                 * @param out the out parameter - the read line 
                 * @return true if data was read, otherwise false
                 */
                virtual bool get_first_space(TextPieceReader& out) {
                    LOG_DEBUG4 << "Searching forward for a space!" << END_LOG;
                    return get_first<' '>(out);
                }

                /**
                 * This function, from the current position, searches for the space char
                 * or until the beginning of the text and then sets the data about the found
                 * region into the provided output parameter.
                 * @param out the out parameter - the read line 
                 * @return true if data was read, otherwise false
                 */
                virtual bool get_last_space(TextPieceReader& out) {
                    LOG_DEBUG4 << "Searching backward for a space!" << END_LOG;
                    return get_last<' '>(out);
                }

                /**
                 * This function, searches forward for the first end of tab char
                 * or until the end of the text and then sets the data about the found
                 * region into the provided output parameter.
                 * @param out the out parameter - the read line 
                 * @return true if data was read, otherwise false
                 */
                virtual bool get_first_tab(TextPieceReader& out) {
                    LOG_DEBUG4 << "Searching forward for a tab!" << END_LOG;
                    return get_first<'\t'>(out);
                }

                /**
                 * Allows to get the character at the given index, if
                 * the index stays within the text length bounds.
                 * @param idx the character index
                 * @return 
                 */
                inline char operator[](size_t idx) {
                    if (DO_SANITY_CHECKS && (idx >= m_len)) {
                        stringstream msg;
                        msg << "The improper index '" << idx
                                << "' must be within [0, " << m_len << "]!";
                        throw Exception(msg.str());
                    } else {
                        return m_beginPtr[idx];
                    }
                }

                /**
                 * The comparison operator implementation
                 * @param other text piece to compare with
                 */
                inline bool operator==(const TextPieceReader & other) const {
                    if (other.m_len == m_len) {
                        return !strncmp(m_beginPtr, other.m_beginPtr, m_len);
                    } else {
                        return false;
                    }
                }

                /**
                 * The comparison operator implementation
                 * @param other text piece to compare with
                 */
                inline bool operator!=(const TextPieceReader & other) const {
                    return !this->operator==(other);
                }

                /**
                 * The comparison operator implementation
                 * @param other a c_string to compare with
                 */
                inline bool operator==(const char * other) const {
                    const size_t len = strlen(other);
                    if (len == m_len) {
                        return !strncmp(m_beginPtr, other, m_len);
                    } else {
                        return false;
                    }
                }

                /**
                 * The comparison operator implementation
                 * @param other a c_string to compare with
                 */
                inline bool operator!=(const char * other) const {
                    return !this->operator==(other);
                }

                /**
                 * The comparison operator implementation
                 * @param other a c_string to compare with
                 */
                inline bool operator==(const string & other) const {
                    return this->operator==(other.c_str());
                }

                /**
                 * The comparison operator implementation
                 * @param other a c_string to compare with
                 */
                inline bool operator!=(const string & other) const {
                    return !this->operator==(other);
                }

                /**
                 * Allows to convert the line to string object
                 * @return the resulting line
                 */
                inline const string & str() const {
                    LOG_DEBUG4 << "m_is_gen_str = " << m_is_gen_str << END_LOG;
                    if (m_is_gen_str) {
                        LOG_DEBUG4 << "m_len = " << m_len << END_LOG;
                        if (m_len > 0) {
                            if (m_len <= MAX_N_GRAM_STRING_LENGTH) {
                                LOG_DEBUG4 << "m_beginPtr = " << SSTR(static_cast<const void *> (m_beginPtr))
                                        << ", m_len = " << SSTR(m_len) << END_LOG;
                                m_str.assign(m_beginPtr, m_len);
                            } else {
                                m_str = TEXT_TOO_LARGE_STR;
                            }
                        } else {
                            m_str = TEXT_NOTHING_STR;
                        }
                        m_is_gen_str = false;
                    }
                    return m_str;
                }
            };

            /**
             * Overloading the output operator for the ostream
             * @param output the stream to print to
             * @param val the value to print
             * @return the output stream
             */
            inline ostream& operator<<(ostream &output, const TextPieceReader & val) {
                return output << val.str();
            };

            /**
             * This function allows to convert the BasicTextFileReader elements tokens into a array string representation. 
             * @param tokens the tokens to print
             * @param from_idx the from index
             * @param to_idx the to index
             * @return the resulting string
             */
            template<TModelLevel N = M_GRAM_LEVEL_MAX>
            inline string tokens_to_string(const TextPieceReader tokens[N], const TModelLevel begin_idx, const TModelLevel end_idx) {
                stringstream data;
                data << "[ ";
                LOG_DEBUG4 << "Appending tokens from idx: " << SSTR(begin_idx) << " to idx: " << SSTR(end_idx) << END_LOG;
                for (int i = begin_idx; i <= end_idx; i++) {
                    LOG_DEBUG4 << "Appending token [" << SSTR(i) << "] = '"
                            << tokens[i].str() << "' to the string!" << END_LOG;
                    data << tokens[i].str() << " ";
                }
                LOG_DEBUG4 << "Done appending tokens!" << END_LOG;
                data << "]";
                return data.str();
            };
        }
    }
}


#endif	/* MTEXTFILEPIECE_HPP */

