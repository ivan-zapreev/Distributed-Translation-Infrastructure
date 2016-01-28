/* 
 * File:   StringUtils.hpp
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
 * Created on July 27, 2015, 2:21 PM
 */

#ifndef STRINGUTILS_HPP
#define	STRINGUTILS_HPP

#include <string>  // std::string
#include <vector>  // std::vector
#include <sstream> // std::stringstream
#include <cstddef> // std::size_t
#include <limits>  // std::numeric_limits
#include <cstring> // std::memchr

#include "common/utils/logging/logger.hpp"
#include "common/utils/exceptions.hpp"

using namespace uva::utils::logging;
using namespace uva::utils::exceptions;

namespace uva {
    namespace utils {
        namespace text {

#ifdef __APPLE__

            /*
             * Reverse of memchr()
             * Find the last occurrence of 'c' in the buffer 's' of size 'n'.
             * 
             * Copyright (c) 2007 Todd C. Miller <Todd.Miller@courtesan.com>
             *
             * Permission to use, copy, modify, and distribute this software for any
             * purpose with or without fee is hereby granted, provided that the above
             * copyright notice and this permission notice appear in all copies.
             *
             * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
             * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
             * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
             * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
             * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
             * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
             * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
             */
            static inline void * memrchr(const void * s, const char c, size_t n) {
                if (n != 0) {
                    const unsigned char * cp = (unsigned char *) s + n;
                    do {
                        if (*(--cp) == (unsigned char) c)
                            return ((void *) cp);
                    } while (--n != 0);
                }
                return ((void *) 0);
            }
#endif

            //All the possible Whitespaces, including unicode, to be imagined, are to be used for trimming and reduce
            //const string WHITESPACES = "\u0020\u00A0\u1680\u2000\u2001\u2002\u2003\u2004\u2005\u2006\u2007\u2008\u2009\u200A\u202F\u205F\u3000 \t\f\v\n\r"
            //In the ARPA format we should only consider the basic asci delimiters!
            const string WHITESPACES = "\t\f\v\n\r ";

            /**
             * This function allows to convert an array of values to a string representation.
             * @param values the array of values to print
             * @return the resulting string
             */
            template<typename T, size_t N>
            static inline string array_to_string(const T values[N]) {
                stringstream data;
                data << "[ ";
                for (size_t idx = 0; idx < N; idx++) {
                    data << values[idx] << " ";
                }
                data << "]";
                return data.str();
            }

            /**
             * This function allows to convert a vector of values to a string representation.
             * @param values the vector of values to print
             * @return the resulting string
             */
            template<typename T>
            static inline string vector_to_string(const vector<T> &values) {
                stringstream data;
                data << "[ ";
                for (typename vector<T>::const_iterator it = values.cbegin(); it != values.cend(); ++it) {
                    data << *it << " ";
                }
                data << "]";
                return data.str();
            }

            /**
             * This function just takes the N-Gram tokens and puts them together in one string.
             * @param tokens the tokens to put together
             * @return the resulting string
             */
            static inline string tokens_to_string(const vector<string> &values) {
                return vector_to_string<string>(values);
            }

            /**
             * Tokenise a given string into a vector of strings
             * @param s the string to tokenise
             * @param delim the delimiter
             * @param elems the output array
             */
            static inline void tokenize(const std::string &data, const char delim, vector<string> & elems) {
                elems.clear();
                size_t start = 0;
                size_t end = data.find_first_of(delim);
                while (end <= std::string::npos) {
                    elems.emplace_back(data.substr(start, end - start));
                    if (end != std::string::npos) {
                        start = end + 1;
                        end = data.find_first_of(delim, start);
                    } else {
                        break;
                    }
                }
            }

            /**
             * This function can be used to trim the string
             * @param str the string to be trimmed, it is an in/out parameter
             * @param whitespace the white spaces to be trimmed, the default value is " \t" 
             */
            inline void trim(std::string& str,
                    const std::string& whitespace = WHITESPACES) {
                LOG_DEBUG4 << "Trimming the string '" << str << "', with white spaces " << END_LOG;
                if (str != "") {
                    const size_t strBegin = str.find_first_not_of(whitespace);
                    LOG_DEBUG4 << "First not of whitespaces pos: " << strBegin << END_LOG;

                    if (strBegin == std::string::npos) {
                        str = ""; // no content
                    } else {
                        const size_t strEnd = str.find_last_not_of(whitespace);
                        LOG_DEBUG4 << "Last not of whitespaces pos: " << strEnd << END_LOG;
                        const size_t strRange = strEnd - strBegin + 1;
                        LOG_DEBUG4 << "Need a substring: [" << strBegin << ", " << (strBegin + strRange - 1) << "]" << END_LOG;

                        str = str.substr(strBegin, strRange);
                    }
                }
                LOG_DEBUG4 << "The trimmed result is '" << str << "'" << END_LOG;
            }

            /**
             * This is a reduce function that first will trim the string and then
             * reduce the sub-ranges within the string.
             * @param str the string to be reduced, is an in/out parameter
             * @param fill the filling symbol to be used within the string instead of ranges, by default " "
             * @param whitespace the white spaces to be reduced, by default " \t"
             */
            inline void reduce(std::string& str,
                    const std::string& fill = " ",
                    const std::string& whitespace = WHITESPACES) {
                LOG_DEBUG4 << "Reducing the string '" << str << "', with white spaces" << END_LOG;
                if (str != "") {
                    // trim first
                    trim(str, whitespace);

                    // replace sub ranges
                    size_t beginSpace = str.find_first_of(whitespace);
                    while (beginSpace != std::string::npos) {
                        const size_t endSpace = str.find_first_not_of(whitespace, beginSpace);
                        const size_t range = endSpace - beginSpace;

                        str.replace(beginSpace, range, fill);

                        const size_t newStart = beginSpace + fill.length();
                        beginSpace = str.find_first_of(whitespace, newStart);
                    }
                }
                LOG_DEBUG4 << "The reduced result is '" << str << "'" << END_LOG;
            }

#define valid_digit(c) ((c) >= '0' && (c) <= '9')

            // See: http://pastebin.com/dHP1pgQ4

            /**
             * This function tries to read from a piece of memory and interpret
             * it as a float or double or other decimal type given as template
             * parameter. This function is more efficient than atof and it does
             * not check for the lengh of the input, e.g. does no rely on the
             * \0 terminating symbol in general. It reads until it gets something
             * that it can not interpret as a part of a decimal. Then it stops!
             * 
             * WARNING: This function does at least one symbol look ahead and
             * does not check on the \0 of the c string or its length. This is
             * perfectly good when reading ARPA file floats, if the file format
             * is correct. We can not generally rely on \0 as the input string
             * is not guaranteed to have it! Yet we could try to take into account
             * the string length, but this might cost performance!
             * 
             * ToDo: Try to impose the string length limit and test the performance
             * on reading large ARPA files!
             * 
             * @param res the type to read into
             * @param p the pointer to read from,
             * @return true if the function thinks it successfully parsed the
             * input, otherwise false.
             */
            inline bool fast_s_to_f(float & res, const char *p) {
                uint32_t int_part = 0.0;
                int c = 0; // counter to check how many numbers we got!

                // Get the sign!
                bool neg = false;
                if (*p == '-') {
                    neg = true;
                    ++p;
                } else if (*p == '+') {
                    neg = false;
                    ++p;
                }

                // Get the digits before decimal point
                while (valid_digit(*p)) {
                    int_part = (int_part * 10) + (*p - '0');
                    ++p;
                    ++c;
                }
                res = int_part;

                // Get the digits after decimal point
                if (*p == '.') {
                    uint32_t dec_part = 0;
                    uint32_t scale = 1;
                    ++p;
                    while (valid_digit(*p)) {
                        dec_part = (dec_part * 10) + (*p - '0');
                        ++p;
                        scale *= 10;
                        ++c;
                    }
                    res += dec_part * (1.0f / scale);
                }

                // FIRST CHECK:
                if (c == 0) {
                    return false;
                } // we got no dezimal places! this cannot be any number!


                // Get the digits after the "e"/"E" (exponenet)
                if (*p == 'e' || *p == 'E') {
                    unsigned int e = 0;

                    bool negE = false;
                    ++p;
                    if (*p == '-') {
                        negE = true;
                        ++p;
                    } else if (*p == '+') {
                        negE = false;
                        ++p;
                    }
                    // Get exponent
                    c = 0;
                    while (valid_digit(*p)) {
                        e = (e * 10) + (*p - '0');
                        ++p;
                        ++c;
                    }
                    if (!neg && e > (uint) std::numeric_limits<float>::max_exponent10) {
                        e = (uint) std::numeric_limits<float>::max_exponent10;
                    } else if (e < (uint) std::numeric_limits<float>::min_exponent10) {
                        e = (uint) std::numeric_limits<float>::max_exponent10;
                    }
                    // SECOND CHECK:
                    if (c == 0) {
                        return false;
                    } // we got no  exponent! this was not intended!!

                    uint32_t scale_exp = 1.0;
                    // Calculate scaling factor.
                    while (e > 0) {
                        scale_exp *= 10;
                        e -= 1;
                    }

                    if (negE) {
                        res *= 1.0f / scale_exp;
                    } else {
                        res *= scale_exp;
                    }
                }

                // Apply sign to number
                if (neg) {
                    res = -res;
                }

                return true;
            }
        }
    }
}

#endif	/* STRINGUTILS_HPP */

