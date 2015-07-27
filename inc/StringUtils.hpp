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

namespace uva {
    namespace smt {
        namespace utils {
            namespace string {

                /**
                 * This function can be used to trim the string
                 * @param str the string to be trimmed, it is an in/out parameter
                 * @param whitespace the white spaces to be trimmed, the default value is " \t" 
                 */
                inline void trim(const std::string& str,
                                 const std::string& whitespace = " \t") {
                    const auto strBegin = str.find_first_not_of(whitespace);
                    if (strBegin == std::string::npos) {
                        str = ""; // no content
                    } else {
                        const auto strEnd = str.find_last_not_of(whitespace);
                        const auto strRange = strEnd - strBegin + 1;

                        str = str.substr(strBegin, strRange);
                    }
                }

                /**
                 * This is a reduce function that first will trim the string and then
                 * reduce the sub-ranges within the string.
                 * @param str the string to be reduced, is an in/out parameter
                 * @param fill the filling symbol to be used within the string instead of ranges, by default " "
                 * @param whitespace the white spaces to be reduced, by default " \t"
                 */
                inline void reduce(const std::string& str,
                                   const std::string& fill = " ",
                                   const std::string& whitespace = " \t") {
                    // trim first
                    trim(str, whitespace);

                    // replace sub ranges
                    auto beginSpace = str.find_first_of(whitespace);
                    while (beginSpace != std::string::npos) {
                        const auto endSpace = str.find_first_not_of(whitespace, beginSpace);
                        const auto range = endSpace - beginSpace;

                        str.replace(beginSpace, range, fill);

                        const auto newStart = beginSpace + fill.length();
                        beginSpace = str.find_first_of(whitespace, newStart);
                    }
                }
            }
        }
    }
}

#endif	/* STRINGUTILS_HPP */

