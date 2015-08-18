/* 
 * File:   AFileReader.hpp
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
 * Created on August 18, 2015, 8:44 PM
 */

#ifndef AFILEREADER_HPP
#define	AFILEREADER_HPP

#include "Globals.hpp"
#include "Logger.hpp"
#include "Exceptions.hpp"
#include "StringUtils.hpp"
#include "TextPieceReader.hpp"

using namespace std;
using namespace uva::smt::utils::text;

namespace uva {
    namespace smt {
        namespace file {

            /**
             * This is an abstract base class for the file readers
             * we are going to be using to read model files
             */
            class AFileReader :  public TextPieceReader {
            public:
                
                AFileReader() : TextPieceReader() {}
                
                /**
                 * Allows to check if the file has been open
                 * @return true if the file is open otherwise false
                 */
                virtual bool is_open() const = 0;
                
                /**
                 * Allows to convert the file reader to a boolean
                 * @return true if the file exists, otherwise false
                 */
                virtual operator bool() const = 0;
                
                /**
                 * Allows to close the file
                 */
                virtual void close() {};
                
                /**
                 * The basic destructor, calls the close method
                 */
                virtual ~AFileReader(){
                    //Just close the file if it has not been closed yet
                    close();
                }
            };
        }
    }
}

#endif	/* AFILEREADER_HPP */

