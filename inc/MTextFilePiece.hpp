/* 
 * File:   MTextFilePiece.hpp
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

#ifndef MTEXTFILEPIECE_HPP
#define	MTEXTFILEPIECE_HPP

#include <cstring>

using namespace std;

namespace uva {
    namespace smt {
        namespace file {

            /**
             * This basic storage class that stores the pointer to the new memory-
             * mapped file piece plus the length of this piece. This class can be
             * used to represent a line or a word or any arbitrary piece of memory.
             * Note that, the string here is not \0 terminated!
             */
            struct MTextFilePiece {
            public:
                //The pointer to the first line character, NOT necessarily \0 terminated!!!!
                const char * m_beginPtr;
                //The length of the line
                size_t m_len;

                /**
                 * The comparison operator implementation
                 * @param other a c_string to compare with
                 */
                inline bool operator==(const char * other) const {
                    const size_t len = strlen(other);
                    if (len != m_len) {
                        return false;
                    } else {
                        return !strncmp(m_beginPtr, other, m_len);
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
                    return this->operator!=(other.c_str());
                }

                /**
                 * Allows to convert the line to string object
                 * @return the resulting line
                 */
                inline string str() const {
                    char data[m_len + 1];
                    strncpy(data, m_beginPtr, m_len);
                    data[m_len] = '\0';
                    return string(data);
                }
            };

            /**
             * Overloading the output operator for the ostream
             * @param output the stream to print to
             * @param val the value to print
             * @return the output stream
             */
            inline ostream& operator<<(ostream &output, const MTextFilePiece & val) {
                return output << val.str();
            };
        }
    }
}


#endif	/* MTEXTFILEPIECE_HPP */

