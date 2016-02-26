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
#define STRINGUTILS_HPP

#include <locale>  //std::tolower
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

            //A forward declaration of the parsinig function
            static inline bool fast_s_to_f(float & res, const char *p);

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

            //In the asci whitespaces delimiters!
            const string UTF8_ASCII_WHITESPACES = u8"\t\f\v\n\r ";
            //Stores the known ASCII delimiters
            const string UTF8_ASCII_PUNCTUATIONS = u8".,?!/'\"`@#$%^&*()[]{}-_+=*<>~|\\;:";
            //Stores the ASCII space char
            const char ASCII_SPACE_CHAR = ' ';
            //Stores the utf8 space string
            const string UTF8_SPACE_STRING = u8" ";
            //Stores the utf8 empty string
            const string UTF8_EMPTY_STRING = u8"";
            //Stores the utf8 new line string
            const string UTF8_NEW_LINE_STRING = u8"\n";

            /**
             * This function allows to convert an array of values to a string representation.
             * @param values the array of values to print
             * @return the resulting string
             */
            template<typename T>
            static inline string array_to_string(const size_t N, const T *values,
                    const string& delim = UTF8_SPACE_STRING) {
                stringstream data;
                data << u8"[ ";
                for (size_t idx = 0; idx < N; idx++) {
                    data << values[idx] << ((idx != (N - 1)) ? delim : u8"");
                }
                data << u8" ]";
                return data.str();
            }

            /**
             * This function allows to convert an array of values to a string representation.
             * @param values the array of values to print
             * @return the resulting string
             */
            template<typename T, size_t N>
            static inline string array_to_string(const T values[N],
                    const string& delim = UTF8_SPACE_STRING) {
                return array_to_string(N, values, delim);
            }

            /**
             * This function allows to convert a vector of values to a string representation.
             * @param values the vector of values to print
             * @return the resulting string
             */
            template<typename T>
            static inline string vector_to_string(const vector<T> &values) {
                stringstream data;
                data << u8"[ ";
                for (typename vector<T>::const_iterator it = values.cbegin(); it != values.cend(); ++it) {
                    data << *it << UTF8_SPACE_STRING;
                }
                data << u8"]";
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
             * Tokenise a given string into a a bunch of strings each of which will be parsed into a float
             * @param data the string to tokenise
             * @param elems the array to fill the data into
             * @param num_elems the actual number of elements
             * @param delim the delimiter string storing the token delimiters, default is UTF8_SPACE_STRING
             */
            template<size_t MAX_NUM_ELEMS>
            static inline void tokenize_s_t_f(const std::string &data,
                    float elems[MAX_NUM_ELEMS], size_t & num_elems,
                    const string& delim = UTF8_SPACE_STRING) {
                size_t start = 0;
                size_t end = data.find_first_of(delim);

                //Initialize the number of elements with zero
                num_elems = 0;

                //Search for the delimiter and make tokens
                while (end <= std::string::npos) {
                    //Check that the number of elements is not exceeded
                    ASSERT_CONDITION_THROW((num_elems > MAX_NUM_ELEMS),
                            string("Exceeding the maximum allowed number of elements: ")
                            + to_string(MAX_NUM_ELEMS) + string(" when parsing: ") + data);

                    //Parse the next token into the float
                    string str = data.substr(start, end - start);
                    ASSERT_CONDITION_THROW(!fast_s_to_f(elems[num_elems++], str.c_str()),
                            string("Could not parse the token: ") + str);

                    if (end != std::string::npos) {
                        start = end + 1;
                        end = data.find_first_of(delim, start);
                    } else {
                        break;
                    }
                }
            }

            /**
             * Tokenise a given string into a vector of strings
             * @param data the string to tokenise
             * @param elems the vector to fill the data into
             * @param delim the delimiter string storing the token delimiters, default is UTF8_SPACE_STRING
             */
            static inline void tokenize(const std::string &data,
                    vector<string> & elems,
                    const string& delim = UTF8_SPACE_STRING) {
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
             * Allows to surround the given punctuation symbols in the string by the fill in symbols.
             * After this is done the string might need reduction.
             * @param str the string in which the delimiters are to be surrounded by the fill symbols
             * @param fill the fill symbols to put around delimiters, default is just a single space
             * @param puncts the punctuations to surround with the fill symbols, default are ASCII_PUNCTUATIONS
             * @return the reference to the (resulting) string
             */
            static inline std::string& punctuate(std::string& str,
                    const std::string& fill = UTF8_SPACE_STRING,
                    const std::string& puncts = UTF8_ASCII_PUNCTUATIONS) {
                LOG_DEBUG4 << "Tokenizing the string '" << str << "'" << END_LOG;
                //Find the delimiters and surround them with fill symbols
                size_t delim_pos = str.find_first_of(puncts);
                while (delim_pos != std::string::npos) {
                    //Surround the delimiter with two fill elements
                    str.replace(delim_pos, 1, fill + str[delim_pos] + fill);
                    //Search for the next delimiter, skip the positions we just put our new values into
                    delim_pos = str.find_first_of(puncts, delim_pos + 2 * fill.length() + 1);
                }

                LOG_DEBUG4 << "The string after tokenizing is '" << str << "'" << END_LOG;
                return str;
            }

            /**
             * Allows to convert the string to lower case 
             * @param str [in/out] the string to convert
             * @return the reference to the (converted) string 
             */
            static inline std::string& to_lower(std::string& str) {
                LOG_DEBUG4 << "Lowercasing the string '" << str << "'" << END_LOG;
                for (unsigned int i = 0; i < str.length(); ++i) {
                    str[i] = tolower(str[i]);
                }
                LOG_DEBUG4 << "The string after lowercasing is '" << str << "'" << END_LOG;
                return str;
            }

            /**
             * This function can be used to trim the string
             * @param str the string to be trimmed, it is an in/out parameter
             * @param whitespace the white spaces to be trimmed, the default value is UTF8_ASCII_WHITESPACES
             * @return the reference to the trimmed string
             */
            static inline std::string& trim(std::string& str,
                    const std::string& whitespace = UTF8_ASCII_WHITESPACES) {
                LOG_DEBUG4 << "Trimming the string '" << str << "', with white spaces " << END_LOG;
                if (str != UTF8_EMPTY_STRING) {
                    const size_t begin_pos = str.find_first_not_of(whitespace);
                    LOG_DEBUG4 << "First not of whitespaces pos: " << begin_pos << END_LOG;

                    if (begin_pos == std::string::npos) {
                        str = UTF8_EMPTY_STRING; // no content
                    } else {
                        const size_t end_pos = str.find_last_not_of(whitespace);
                        LOG_DEBUG4 << "Last not of whitespaces pos: " << end_pos << END_LOG;
                        const size_t range_len = end_pos - begin_pos + 1;
                        LOG_DEBUG4 << "Need a substring: [" << begin_pos << ", " << end_pos << "]" << END_LOG;

                        str = str.substr(begin_pos, range_len);
                    }
                }
                LOG_DEBUG4 << "The trimmed result is '" << str << "'" << END_LOG;
                return str;
            }

            /**
             * This is a reduce function that first will trim the string and then
             * reduce the sub-ranges within the string.
             * @param str the string to be reduced, is an in/out parameter
             * @param fill the filling symbol to be used within the string instead of ranges, by default UTF8_SPACE_STRING
             * @param whitespace the white spaces to be reduced, by default UTF8_ASCII_WHITESPACES
             */
            static inline std::string& reduce(std::string& str,
                    const std::string& fill = UTF8_SPACE_STRING,
                    const std::string& whitespace = UTF8_ASCII_WHITESPACES) {
                LOG_DEBUG4 << "Reducing the string '" << str << "', with white spaces" << END_LOG;
                if (str != UTF8_EMPTY_STRING) {
                    // trim first
                    trim(str, whitespace);

                    // replace sub ranges
                    size_t begin_pos = str.find_first_of(whitespace);
                    while (begin_pos != std::string::npos) {
                        LOG_DEBUG4 << "The first whitespace position is: " << begin_pos << END_LOG;
                        const size_t end_pos = str.find_first_not_of(whitespace, begin_pos + 1);
                        LOG_DEBUG4 << "The first next non-whitespace position is: " << end_pos << END_LOG;
                        const size_t length = end_pos - begin_pos;

                        str.replace(begin_pos, length, fill);
                        LOG_DEBUG4 << "Replacement [" << begin_pos << "," << end_pos << "] with '"
                                << fill << "' results in: '" << str << "'" << END_LOG;

                        const size_t next_pos = begin_pos + fill.length();
                        begin_pos = str.find_first_of(whitespace, next_pos);
                    }
                }
                LOG_DEBUG4 << "The reduced result is '" << str << "'" << END_LOG;
                return str;
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
            static inline bool fast_s_to_f(float & res, const char *p) {
                //Store the original ptr
                const char * original = p;

                uint_fast32_t int_part = 0.0;
                uint_fast32_t c = 0; // counter to check how many numbers we got!

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
                    uint_fast32_t dec_part = 0;
                    uint_fast32_t scale = 1;
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
                    uint_fast64_t e = 0;

                    LOG_DEBUG4 << "Got exponent in: " << original << END_LOG;

                    bool negE = false;
                    ++p;
                    if (*p == '-') {
                        negE = true;
                        ++p;
                        LOG_DEBUG4 << "Exponent is Negative" << END_LOG;
                    } else if (*p == '+') {
                        negE = false;
                        ++p;
                        LOG_DEBUG4 << "Exponent is Positive" << END_LOG;
                    }

                    // Get exponent
                    c = 0;
                    while (valid_digit(*p)) {
                        e = (e * 10) + (*p - '0');
                        ++p;
                        ++c;
                    }
                    LOG_DEBUG4 << "Exponent value is: " << e << END_LOG;
                    
                    static constexpr uint max_exponent10 = std::numeric_limits<float>::min_exponent10;
                    static constexpr uint min_exponent10 = abs(std::numeric_limits<float>::min_exponent10);

                    if (!neg && e > max_exponent10) {
                        e = max_exponent10;
                    } else if (e > min_exponent10 ) {
                        e = min_exponent10;
                    }

                    LOG_DEBUG4 << "Adjusted exponent value is: " << e << END_LOG;

                    // SECOND CHECK:
                    if (c == 0) {
                        return false;
                    } // we got no  exponent! this was not intended!!

                    double scale_exp = 1.0;
                    // Calculate scaling factor.
                    while (e > 0) {
                        scale_exp *= 10;
                        e -= 1;
                    }

                    LOG_DEBUG4 << "Exponent multiplier is: " << scale_exp << END_LOG;

                    if (negE) {
                        res *= 1.0d / scale_exp;
                    } else {
                        res *= scale_exp;
                    }
                    
                    LOG_DEBUG4 << "The result is: " << ((neg) ? -res : res) << END_LOG;
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

#endif /* STRINGUTILS_HPP */

