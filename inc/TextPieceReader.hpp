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

#include <cstring>      // std::memchr
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
            static const size_t MAX_STRING_LENGTH = 2048;
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

            protected:

                /**
                 * Allows to get the pointer to the beginning of the text
                 * @return the pointer to the beginning of the text
                 */
                inline void * getBeginPtr() const {
                    return (void *) m_beginPtr;
                }

                /**
                 * Allows to get the length of the text
                 * @return the length of the text
                 */
                inline size_t getLen() const {
                    return m_len;
                }

            public:

                /**
                 * The basic constructor initializes empty text
                 */
                TextPieceReader()
                : m_beginPtr(NULL), m_len(0), m_is_gen_str(true), m_str(), m_cursorPtr(NULL), m_restLen(0) {
                }

                /**
                 * The constructor.
                 * @param beginPtr the pointer to the begin of the text
                 * @param len the length of the text
                 */
                TextPieceReader(const void * beginPtr, const size_t len) : TextPieceReader() {
                    set(beginPtr, len);
                }

                /**
                 * The copy constructor.
                 * @param other the const reference to the object to copy from
                 */
                TextPieceReader(const TextPieceReader & other) {
                    this->m_beginPtr = other.m_beginPtr;
                    this->m_len = other.m_len;
                    this->m_is_gen_str = other.m_is_gen_str;
                    this->m_str = other.m_str;
                    this->m_cursorPtr = other.m_cursorPtr;
                    this->m_restLen = other.m_restLen;
                }

                /**
                 * Allows to get the pointer to the beginning of the text
                 * This is a C string that is returned BUT there is no \0
                 * termination and it can be Gb long!
                 * @return the pointer to the beginning of the text
                 */
                inline const char * getBeginCStr() {
                    return m_beginPtr;
                }

                /**
                 * Allows to get the pointer to the remainder of the text
                 * This is a C string that is returned BUT there is no \0
                 * termination and it can be Gb long!
                 * @return the pointer to the remainder of the text
                 */
                inline const char * getRestCStr() {
                    return m_cursorPtr;
                }

                /**
                 * Allows to set the text
                 * @param beginPtr the pointer to the beginning of the text
                 * @param len the length of the text
                 */
                inline void set(const void * beginPtr, const size_t len) {
                    m_beginPtr = static_cast<const char *> (beginPtr);
                    m_cursorPtr = m_beginPtr;
                    m_is_gen_str = true;
                    m_len = len;
                    m_restLen = m_len;

                    LOG_DEBUG2 << SSTR(this) << ": Setting the data to BasicTextPiece: m_beginPtr = "
                            << SSTR((void*) m_beginPtr) << ", m_cursorPtr = "
                            << SSTR((void*) m_cursorPtr) << ", m_is_gen_str = "
                            << m_is_gen_str << ", m_len = " << SSTR(m_len)
                            << ", m_restLen = " << SSTR(m_restLen) << END_LOG;
                }

                /**
                 * This function reads a line from the mapped file
                 * @param out the out parameter - the read line 
                 * @return true if a line was read, otherwise false (end of file)
                 */
                inline bool getNext(TextPieceReader& out, const char delim) {
                    const char * out_m_beginPtr = NULL;
                    size_t out_m_len = 0;

                    //The next line begins where we stopped
                    out_m_beginPtr = m_cursorPtr;

                    //Search for the next new line symbol in the remainder of the file
                    const char * charPtr = static_cast<const char *> (memchr(m_cursorPtr, delim, m_restLen));

                    LOG_DEBUG3 << SSTR(this) << ": Searching for the character got: " << SSTR((void *) charPtr) << END_LOG;

                    //Check if we found a pointer to the new line
                    if (charPtr != NULL) {
                        //Compute the line length
                        size_t lineLen = charPtr - m_cursorPtr;

                        LOG_DEBUG3 << SSTR(this) << ": The substring length is " << SSTR(lineLen) << END_LOG;

                        //Store the pointer to the remaining of the file
                        m_cursorPtr = charPtr + 1;
                        //Store the remaining length of the file
                        m_restLen -= (lineLen + 1);

                        LOG_DEBUG3 << SSTR(this) << ": Resetting m_cursorPtr = "
                                << SSTR((void *) m_cursorPtr) << ", m_restLen = "
                                << m_restLen << END_LOG;

                        //Set the resulting length of the line
                        out_m_len = lineLen;

                        //For Windows-format text files remove the '\r' as well
                        //in case we were looking for the end of file
                        if (lineLen && delim == '\n' && '\r' == out_m_beginPtr[lineLen - 1]) {
                            out_m_len--;
                            LOG_DEBUG3 << SSTR(this) << ": A \\\\r detected, resetting m_restLen = "
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
                    LOG_DEBUG3 << SSTR(this) << ": Searching for a new line!" << END_LOG;
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
                    LOG_DEBUG3 << SSTR(this) << ": Searching for a space!" << END_LOG;
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
                    LOG_DEBUG3 << SSTR(this) << ": Searching for a tab!" << END_LOG;
                    return getNext(out, '\t');
                }

                /**
                 * Allows to get the character at the given index, if
                 * the index stays within the text length bounds.
                 * @param idx the character index
                 * @return 
                 */
                inline char operator[](size_t idx) {
                    if (idx < m_len) {
                        return m_beginPtr[idx];
                    } else {
                        stringstream msg;
                        msg << "The improper index '" << idx
                                << "' must be within [0, " << m_len << "]!";
                        throw Exception(msg.str());
                    }
                }

                /**
                 * The comparison operator implementation
                 * @param other a c_string to compare with
                 */
                inline bool operator==(const char * other) const {
                    const size_t len = strlen(other);
                    if (len != m_len) {
                        return false;
                    } else {
                        if (m_len <= MAX_STRING_LENGTH) {
                            return !strncmp(m_beginPtr, other, m_len);
                        } else {
                            return false;
                        }
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
                    if ((m_len <= MAX_STRING_LENGTH)) {
                        return this->operator==(other.c_str());
                    } else {
                        return false;
                    }
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
                    if (m_is_gen_str) {
                        if (m_len > 0) {
                            if (m_len <= MAX_STRING_LENGTH) {
                                char data[m_len + 1];
                                strncpy(data, m_beginPtr, m_len);
                                data[m_len] = '\0';
                                m_str = data;
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
             * @return the resulting string
             */
            template<TModelLevel N>
            inline string tokensToString(const TextPieceReader tokens[N], const TModelLevel level) {
                stringstream data;
                data << "[ ";
                for (int i = 0; i < min<TModelLevel>(level, N); i++) {
                    data << tokens[i].str() << " ";
                }
                data << "]";
                return data.str();
            }

        }
    }
}


#endif	/* MTEXTFILEPIECE_HPP */

