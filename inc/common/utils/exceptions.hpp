/* 
 * File:   exceptions.hpp
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
 * Created on April 18, 2015, 1:31 PM
 */
#ifndef EXCEPTIONS_HPP
#define	EXCEPTIONS_HPP

#include <exception>    // std::exception
#include <string>       // std::string

#include "common/utils/logging/logger.hpp"

using namespace std;
using namespace uva::utils::logging;

namespace uva {
    namespace utils {
        namespace exceptions {

            //Enables all sorts of internal sanity checks,
            //e.g. sets the collision detection on and off.
            constexpr bool DO_SANITY_CHECKS = false;

#define THROW_EXCEPTION(text) { \
    stringstream msg; \
    if (logger::get_reporting_level() >= debug_levels_enum::INFO3) { \
        msg << __FILENAME__ << "::" << __FUNCTION__ \
            << "(...) " << __LINE__ << " : " << (text); \
    } else { \
        msg << __FILENAME__ << ": " << (text); \
    } \
    LOG_DEBUG << "<THROWING> " << msg.str() << END_LOG; \
    throw uva_exception(msg.str()); \
}
            
#define THROW_MUST_OVERRIDE() THROW_EXCEPTION("Must be overridden in the sub class!")
#define THROW_MUST_NOT_CALL() THROW_EXCEPTION("Must not be called, is not needed!")
#define THROW_NOT_IMPLEMENTED() THROW_EXCEPTION("This functionality is not yet implemented!")
#define ASSERT_CONDITION_THROW(CONDITION, MESSAGE) \
    if(CONDITION) { \
        THROW_EXCEPTION(MESSAGE); \
    }
#define ASSERT_SANITY_THROW(CONDITION, MESSAGE) \
ASSERT_CONDITION_THROW(DO_SANITY_CHECKS && (CONDITION), MESSAGE);

            /**
             * This is an application exception class that is capable of storing an error message
             */
            class uva_exception : public exception {
            private:
                //The error message to be stored
                string msg;

            public:

                explicit uva_exception(const char * message) : exception(), msg(message) {
                }

                explicit uva_exception(const string &message) : exception(), msg(message) {
                }

                /**
                 * The copy constructor
                 * @param other the other exception to copy from
                 */
                uva_exception(uva_exception const & other) {
                    this->msg = other.msg;
                }

                /**
                 * This method returns the stored message
                 * @return the reference to a constant error message string
                 */
                string const & get_message() const throw () {
                    return msg;
                }

                /** Destructor.
                 * Virtual to allow for subclassing.
                 */
                virtual ~uva_exception() throw () {
                }

                /** Returns a pointer to the (constant) error description.
                 *  @return A pointer to a \c const \c char*. The underlying memory
                 *          is in posession of the \c Exception object. Callers \a must
                 *          not attempt to free the memory.
                 */
                virtual const char* what() const throw () {
                    return msg.c_str();
                }
            };
        }
    }
}
#endif	/* EXCEPTIONS_HPP */

