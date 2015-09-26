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

#include <cstring>      // std::memchr std::strncpy
#include <algorithm>    // std::min

#include "Globals.hpp"
#include "Logger.hpp"
#include "Exceptions.hpp"

using namespace std;
using namespace uva::smt::tries;
using namespace uva::smt::exceptions;
using namespace uva::smt::logging;

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
                const char * m_cursorPtr;
                //The remaining length of the file to read
                size_t m_restLen;

            public:

                /**
                 * The basic constructor initializes empty text
                 */
                TextPieceReader()
                : m_beginPtr(NULL), m_len(0), m_is_gen_str(true), m_str(""), m_cursorPtr(NULL), m_restLen(0) {
                }

                /**
                 * The constructor.
                 * @param beginPtr the pointer to the begin of the text
                 * @param len the length of the text
                 */
                explicit TextPieceReader(void * beginPtr, const size_t len)
                : m_beginPtr(NULL), m_len(0), m_is_gen_str(true), m_str(""), m_cursorPtr(NULL), m_restLen(0) {
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
                    m_cursorPtr = other.m_cursorPtr;
                    m_restLen = other.m_restLen;
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
                    m_cursorPtr = m_beginPtr;
                    m_restLen = m_len;

                    LOG_DEBUG3 << "Setting the data to BasicTextPiece: m_beginPtr = "
                            << SSTR(static_cast<const void*> (m_beginPtr)) << ", m_cursorPtr = "
                            << SSTR(static_cast<const void*> (m_cursorPtr)) << ", m_is_gen_str = "
                            << m_is_gen_str << ", m_len = " << SSTR(m_len)
                            << ", m_restLen = " << SSTR(m_restLen) << END_LOG;
                }

                /**
                 * Allows to get the pointer to the beginning of the text
                 * This is a C string that is returned BUT there is no \0
                 * termination and it can be Gb long!
                 * @return the pointer to the beginning of the text
                 */
                inline const char * getBeginCStr() const {
                    return m_beginPtr;
                }

                /**
                 * Allows to get the pointer to the remainder of the text
                 * This is a C string that is returned BUT there is no \0
                 * termination and it can be Gb long!
                 * @return the pointer to the remainder of the text
                 */
                inline const char * getRestCStr() const {
                    return m_cursorPtr;
                }

                /**
                 * Allows to get the pointer to the beginning of the text
                 * @return the pointer to the beginning of the text
                 */
                inline const void * getBeginPtr() const {
                    return (void *) m_beginPtr;
                }

                /**
                 * Allows to get the length of the text
                 * @return the length of the text
                 */
                inline size_t getLen() const {
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
                 * This function reads a line from the mapped file
                 * @param out the out parameter - the read line 
                 * @return true if a line was read, otherwise false (end of file)
                 */
                inline bool getNext(TextPieceReader& out, const char delim) {
                    //The next line begins where we stopped
                    const char * out_m_beginPtr = m_cursorPtr;

                    LOG_DEBUG3 << SSTR(static_cast<const void *> (out_m_beginPtr)) << END_LOG;

                    //The next line length is first zero
                    size_t out_m_len = 0;

                    //Search for the next new line symbol in the remainder of the file
                    const char * charPtr = static_cast<const char *> (memchr(m_cursorPtr, delim, m_restLen));

                    LOG_DEBUG4 << "Searching for the character got: " << SSTR(static_cast<const void *> (charPtr)) << END_LOG;

                    //Check if we found a pointer to the new line
                    if (charPtr != NULL) {
                        //Compute the line length
                        size_t lineLen = charPtr - m_cursorPtr;

                        LOG_DEBUG4 << "The substring length is " << SSTR(lineLen) << END_LOG;

                        //Store the pointer to the remaining of the file
                        m_cursorPtr = charPtr + 1;
                        //Store the remaining length of the file
                        m_restLen -= (lineLen + 1);

                        LOG_DEBUG4 << "Resetting m_cursorPtr = "
                                << SSTR(static_cast<const void *> (m_cursorPtr))
                                << ", m_restLen = " << m_restLen << END_LOG;

                        //Set the resulting length of the line
                        out_m_len = lineLen;

                        //For Windows-format text files remove the '\r' as well
                        //in case we were looking for the end of file
                        if (lineLen && delim == '\n' && '\r' == out_m_beginPtr[lineLen - 1]) {
                            out_m_len--;
                            LOG_DEBUG4 << "A \\\\r detected, resetting m_restLen = "
                                    << m_restLen << END_LOG;
                        }
                    } else {
                        //If the pointer is not found then the length of the
                        //last file line is the length of the file remains
                        out_m_len = m_restLen;

                        //If the remaining file length is zero then return file
                        if (!m_restLen) {
                            return false;
                        } else {
                            //If there was something left then now we read it all.
                            //Set the remaining length to zero, as there is nothing left.
                            m_restLen = 0;
                        }
                    }

                    //Set the return data
                    out.set(out_m_beginPtr, out_m_len);

                    //Return true as we successfully read a new line
                    return true;
                }

                /**
                 * Allows to check if there is something left to read
                 * @return true if there is yet something to read, otherwise false
                 */
                inline bool hasMore() {
                    return m_restLen > 0;
                }

                /**
                 * This function, from the current position, searches for the end of line
                 * or until the end of the text and then sets the data about the found
                 * region into the provided output parameter.
                 * @param out the out parameter - the read line 
                 * @return true if a line was read, otherwise false (end of file)
                 */
                virtual bool getLine(TextPieceReader& out) {
                    LOG_DEBUG4 << "Searching for a new line!" << END_LOG;
                    return getNext(out, '\n');
                }

                /**
                 * This function, from the current position, searches for the space char
                 * or until the end of the text and then sets the data about the found
                 * region into the provided output parameter.
                 * @param out the out parameter - the read line 
                 * @return true if a line was read, otherwise false (end of file)
                 */
                virtual bool getSpace(TextPieceReader& out) {
                    LOG_DEBUG4 << "Searching for a space!" << END_LOG;
                    return getNext(out, ' ');
                }

                /**
                 * This function, from the current position, searches for the tab char
                 * or until the end of the text and then sets the data about the found
                 * region into the provided output parameter.
                 * @param out the out parameter - the read line 
                 * @return true if a line was read, otherwise false (end of file)
                 */
                virtual bool getTab(TextPieceReader& out) {
                    LOG_DEBUG4 << "Searching for a tab!" << END_LOG;
                    return getNext(out, '\t');
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
            inline string tokensToString(const TextPieceReader tokens[N], const TModelLevel begin_idx, const TModelLevel end_idx) {
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

            /**
             * This function allows to convert the BasicTextFileReader elements tokens into a array string representation. 
             * @param tokens the tokens to print
             * @return the resulting string
             */
            template<TModelLevel N = M_GRAM_LEVEL_MAX>
            inline string tokens_to_string(const TextPieceReader tokens[N], const TModelLevel level) {
                const TModelLevel num_tokens = min<TModelLevel>(level, N);
                LOG_DEBUG4 << "Appending " << SSTR(num_tokens) << "tokens" << END_LOG;
                return tokensToString(tokens, 0, num_tokens - 1);
            };
        }
    }
}


#endif	/* MTEXTFILEPIECE_HPP */

