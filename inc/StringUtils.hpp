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

#include "Logger.hpp"
#include "Globals.hpp"

using uva::smt::logging::Logger;
using uva::smt::tries::TModelLevel;

namespace uva {
    namespace smt {
        namespace utils {
            namespace text {

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
                static inline string arrayToString(const T values[N]) {
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
                static inline string vectorToString(const vector<T> &values) {
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
                static inline string tokensToString(const vector<string> &values) {
                    return vectorToString<string>(values);
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
                    while (end < std::string::npos) {
                        elems.emplace_back(data.substr(start, end - start));
                        start = end + 1;
                        end = data.find_first_of(delim, start);
                    }
                    if (start < std::string::npos) {
                        elems.emplace_back(data.substr(start));
                    }
                }

                /**
                 * This method build an N-Gram from a string, which is nothing more than
                 * just taking a string and tokenizing it with the given delimiter. In
                 * addition this method will test if the resulting N-gram has exactly
                 * the specified number of elements. Will also clean the ngram vector
                 * before filling it in. The order in which the N-gram elements are stored
                 * are the same in which they are present in the given line.
                 * @param line the line of code to convert into an N-gram
                 * @param n the expected maximum value of N
                 * @param delim the delimiter to parse the string into
                 * @param ngram the output parameter that will be filled in with the N-gram values
                 * @throws Exception in case the resulting N-gram has the number elements other than expected
                 */
                static inline void buildNGram(const string & line, const TModelLevel n, const char delim, vector<string> & ngram) throw (Exception) {
                    //First clean the vector
                    ngram.clear();
                    //Tokenise the line
                    tokenize(line, delim, ngram);
                    //Check that the number of words in the N-gram is proper
                    if (ngram.size() < 1 || ngram.size() > n) {
                        stringstream msg;
                        msg << "The line '" << line << "' is not with in [0, " << n << "] as expected!";
                        throw Exception(msg.str());
                    }
                }

                /**
                 * This function can be used to trim the string
                 * @param str the string to be trimmed, it is an in/out parameter
                 * @param whitespace the white spaces to be trimmed, the default value is " \t" 
                 */
                inline void trim(std::string& str,
                        const std::string& whitespace = WHITESPACES) {
                    LOG_DEBUG3 << "Trimming the string '" << str << "', with white spaces " << END_LOG;
                    if (str != "") {
                        const size_t strBegin = str.find_first_not_of(whitespace);
                        LOG_DEBUG3 << "First not of whitespaces pos: " << strBegin << END_LOG;

                        if (strBegin == std::string::npos) {
                            str = ""; // no content
                        } else {
                            const size_t strEnd = str.find_last_not_of(whitespace);
                            LOG_DEBUG3 << "Last not of whitespaces pos: " << strEnd << END_LOG;
                            const size_t strRange = strEnd - strBegin + 1;
                            LOG_DEBUG3 << "Need a substring: [" << strBegin << ", " << (strBegin + strRange - 1) << "]" << END_LOG;

                            str = str.substr(strBegin, strRange);
                        }
                    }
                    LOG_DEBUG3 << "The trimmed result is '" << str << "'" << END_LOG;
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
                    LOG_DEBUG3 << "Reducing the string '" << str << "', with white spaces" << END_LOG;
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
                    LOG_DEBUG3 << "The reduced result is '" << str << "'" << END_LOG;
                }

#define valid_digit(c) ((c) >= '0' && (c) <= '9')

                //
                // Simple and fast atof (ascii to float) function.
                //
                // - Executes about 5x faster than standard MSCRT library atof().
                // - An attractive alternative if the number of calls is in the millions.
                // - Assumes input is a proper integer, fraction, or scientific format.
                // - Matches library atof() to 15 digits (except at extreme exponents).
                // - Follows atof() precedent of essentially no error checking.
                //
                // 09-May-2009 Tom Van Baak (tvb) www.LeapSecond.com
                // See:http://www.leapsecond.com/tools/fast_atof.c
                //

                template<typename T>
                void fast_stoT_1(T & r, const char *p) {
                    int frac;
                    T sign, value, scale;

                    // Get sign, if any.

                    sign = 1.0;
                    if (*p == '-') {
                        sign = -1.0;
                        p += 1;

                    } else if (*p == '+') {
                        p += 1;
                    }

                    // Get digits before decimal point or exponent, if any.

                    for (value = 0.0; valid_digit(*p); p += 1) {
                        value = value * 10.0 + (*p - '0');
                    }

                    // Get digits after decimal point, if any.

                    if (*p == '.') {
                        double pow10 = 10.0;
                        p += 1;
                        while (valid_digit(*p)) {
                            value += (*p - '0') / pow10;
                            pow10 *= 10.0;
                            p += 1;
                        }
                    }

                    // Handle exponent, if any.

                    frac = 0;
                    scale = 1.0;
                    if ((*p == 'e') || (*p == 'E')) {
                        unsigned int expon;

                        // Get sign of exponent, if any.

                        p += 1;
                        if (*p == '-') {
                            frac = 1;
                            p += 1;

                        } else if (*p == '+') {
                            p += 1;
                        }

                        // Get digits of exponent, if any.

                        for (expon = 0; valid_digit(*p); p += 1) {
                            expon = expon * 10 + (*p - '0');
                        }
                        if (expon > 308) expon = 308;

                        // Calculate scaling factor.

                        while (expon >= 50) {
                            scale *= 1E50;
                            expon -= 50;
                        }
                        while (expon >= 8) {
                            scale *= 1E8;
                            expon -= 8;
                        }
                        while (expon > 0) {
                            scale *= 10.0;
                            expon -= 1;
                        }
                    }

                    // Return signed and scaled floating point result.

                    r = sign * (frac ? (value / scale) : (value * scale));
                }

                // See: http://pastebin.com/dHP1pgQ4

                template<typename T>
                bool fast_stoT_2(T & r, const char *p) {
                    r = 0.0;
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
                        r = (r * 10.0) + (*p - '0');
                        ++p;
                        ++c;
                    }

                    // Get the digits after decimal point
                    if (*p == '.') {
                        T f = 0.0;
                        T scale = 1.0;
                        ++p;
                        while (*p >= '0' && *p <= '9') {
                            f = (f * 10.0) + (*p - '0');
                            ++p;
                            scale *= 10.0;
                            ++c;
                        }
                        r += f / scale;
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
                        if (!neg && e > (uint) std::numeric_limits<T>::max_exponent10) {
                            e = (uint) std::numeric_limits<T>::max_exponent10;
                        } else if (e < (uint) std::numeric_limits<T>::min_exponent10) {
                            e = (uint) std::numeric_limits<T>::max_exponent10;
                        }
                        // SECOND CHECK:
                        if (c == 0) {
                            return false;
                        } // we got no  exponent! this was not intended!!

                        T scaleE = 1.0;
                        // Calculate scaling factor.

                        while (e >= 50) {
                            scaleE *= 1E50;
                            e -= 50;
                        }
                        //while (e >=  8) { scaleE *= 1E8;  e -=  8; }
                        while (e > 0) {
                            scaleE *= 10.0;
                            e -= 1;
                        }

                        if (negE) {
                            r /= scaleE;
                        } else {
                            r *= scaleE;
                        }
                    }

                    // POST CHECK:
                    if (*p != '\0') {
                        return false;
                    } // if next character is not the terminating character

                    // Apply sign to number
                    if (neg) {
                        r = -r;
                    }

                    return true;
                }

                //See: http://tinodidriksen.com/2011/05/28/cpp-convert-string-to-double-speed/
                //NOTE: This method does not support exponent part!

                template<typename T>
                void fast_stoT_3(T & r, const char *p) {
                    r = 0.0;
                    bool neg = false;
                    if (*p == '-') {
                        neg = true;
                        ++p;
                    }
                    while (*p >= '0' && *p <= '9') {
                        r = (r * 10.0) + (*p - '0');
                        ++p;
                    }
                    if (*p == '.') {
                        double f = 0.0;
                        int n = 0;
                        ++p;
                        while (*p >= '0' && *p <= '9') {
                            f = (f * 10.0) + (*p - '0');
                            ++p;
                            ++n;
                        }
                        r += f / std::pow(10.0, n);
                    }
                    if (neg) {
                        r = -r;
                    }
                }
            }
        }
    }
}

#endif	/* STRINGUTILS_HPP */

