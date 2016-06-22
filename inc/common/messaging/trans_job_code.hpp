/* 
 * File:   trans_job_code.hpp
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
 * Created on January 21, 2016, 3:07 PM
 */

#ifndef JOB_RESULT_CODE_HPP
#define JOB_RESULT_CODE_HPP

#include <string>
#include <iostream>

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"

using namespace std;
using namespace uva::utils::logging;
using namespace uva::utils::exceptions;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace common {
                namespace messaging {

                    /**
                     * This class represents the translation job result code. This class is used on client and server side.
                     * It represents the server-side status of a translation job and a translation taks.
                     */
                    class trans_job_code {
                    public:

                        /**
                         * Stores the translation job result codes, currently
                         * there is just two results possible, the job is
                         * done - OK; or there was some error - ERROR
                         */
                        enum values {
                            RESULT_UNDEFINED = 0,
                            RESULT_OK = RESULT_UNDEFINED + 1,
                            RESULT_ERROR = RESULT_OK + 1,
                            RESULT_CANCELED = RESULT_ERROR + 1,
                            RESULT_PARTIAL = RESULT_CANCELED + 1,
                            size = RESULT_PARTIAL + 1
                        };

                        /**
                         * The basic constructor that allows to initialize the value with the code
                         * @param code the code value to initialize with
                         */
                        trans_job_code(const values code) : m_code(code) {
                        }

                        /**
                         * The basic constructor that allows to initialize the value from an integer
                         * @param code_val the code value to initialize with
                         */
                        trans_job_code(const int32_t code_val) {
                            //Check that the conversion is possible
                            ASSERT_SANITY_THROW((code_val < values::RESULT_UNDEFINED || code_val >= values::size),
                                    string("Improper code value: ") + to_string(code_val));
                            
                            //Set the code value
                            m_code = static_cast<values>(code_val);
                        }

                        /**
                         * The basic constructor that creates an undefined value
                         */
                        trans_job_code() : trans_job_code(RESULT_UNDEFINED) {
                        }

                        /**
                         * Overloading the assignment operator for the code
                         * @param code the code to set
                         */
                        inline void operator=(const values &code) {
                            m_code = code;
                        }

                        /**
                         * Overloading the equality operator for the code
                         * @param code the code to check equality with
                         */
                        inline bool operator==(const values &code) const {
                            return (m_code == code);
                        }

                        /**
                         * Overloading the comparison operator for the code
                         * @param code the code to compare with
                         */
                        inline bool operator<(const values &code) const {
                            return (m_code < code);
                        }

                        /**
                         * The operator allowing to convert the value to string
                         * @return the string representation of the code
                         */
                        inline operator string() const {
                            return str();
                        }

                        /**
                         * The operator allowing to convert the value to an integer
                         * @return the the integer value
                         */
                        inline operator int() const {
                            return m_code;
                        }

                        /**
                         * Allows to get the job code string for reporting
                         * @return the job code string
                         */
                        const char * const str() const;

                        /**
                         * Returns the stored code value
                         * @return the stored code value
                         */
                        inline values val() const {
                            return m_code;
                        }

                    private:
                        //Stores the code value
                        values m_code;

                        //Stores the status to string mappings
                        static const char * const m_code_str[values::size];
                    };

                    /**
                     * The stream output operator for the given translation job code instance
                     * @param os the output stream
                     * @param code the code to be output
                     * @return the output stream
                     */
                    ostream& operator<<(ostream& os, const trans_job_code& code);
                }
            }
        }
    }
}

#endif /* JOB_RESULT_CODE_HPP */

