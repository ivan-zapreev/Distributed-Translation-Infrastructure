/* 
 * File:   trans_job_status.hpp
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
 * Created on January 27, 2016, 9:03 AM
 */

#include <string>
#include <iostream>

#include "common/utils/Exceptions.hpp"
#include "common/utils/logging/Logger.hpp"

using namespace std;
using namespace uva::utils::logging;
using namespace uva::utils::exceptions;

#ifndef TRANS_JOB_STATUS_HPP
#define TRANS_JOB_STATUS_HPP

namespace uva {
    namespace smt {
        namespace decoding {
            namespace client {

                /**
                 * This class represents the translation job status. It is to be used on the client.
                 * It is needed to trace the client-side translation job status.
                 */
                class trans_job_status {
                public:

                    /**
                     * Stores the possible status values of the client-side translation job
                     */
                    enum values {
                        STATUS_UNDEFINED = 0, //The job has been created but not initialized, i.e. undefined
                        STATUS_REQ_INITIALIZED = STATUS_UNDEFINED + 1, //Initialized with the translation request
                        STATUS_REQ_SENT_GOOD = STATUS_REQ_INITIALIZED + 1, //The translation request is sent
                        STATUS_REQ_SENT_FAIL = STATUS_REQ_SENT_GOOD + 1, //The translation request failed to sent
                        STATUS_RES_RECEIVED = STATUS_REQ_SENT_FAIL + 1, //The translation response was received
                        size = STATUS_RES_RECEIVED + 1
                    };

                    /**
                     * The basic constructor that allows to initialize the value with the status
                     * @param status the status value to initialize with
                     */
                    trans_job_status(const values status) : m_status(status) {
                    }

                    /**
                     * The basic constructor that allows to initialize the value from an integer
                     * @param status_val the status value to initialize with
                     */
                    trans_job_status(const int32_t status_val) {
                        //Check that the conversion is possible
                        ASSERT_CONDITION_THROW((status_val < 0 || status_val >= values::size),
                                string("Improper status value: ") + to_string(status_val));
                        
                        //Set the status value
                        m_status = static_cast<values>(status_val);
                    }

                    /**
                     * The basic constructor that creates an undefined value
                     */
                    trans_job_status() : trans_job_status(STATUS_UNDEFINED) {
                    }

                    /**
                     * Overloading the assignment operator for the status
                     * @param status the status to set
                     */
                    void operator=(const values &status) {
                        m_status = status;
                    }

                    /**
                     * Overloading the equality operator for the status
                     * @param status the status to check equality with
                     */
                    bool operator==(const values &status) const {
                        return (m_status == status);
                    }

                    /**
                     * Overloading the comparison operator for the status
                     * @param status the status to compare with
                     */
                    bool operator<(const values &status) const {
                        return (m_status < status);
                    }

                    /**
                     * The operator allowing to convert the value to string
                     * @return the string representation of the code
                     */
                    operator string() const {
                        return str();
                    }

                    /**
                     * The operator allowing to convert the value to an integer
                     * @return the the integer value
                     */
                    operator int() const {
                        return m_status;
                    }

                    /**
                     * Allows to get the job status string for reporting
                     * @return the job status string
                     */
                    const char * const str() const;

                private:
                    //Stores the status value
                    values m_status;

                    //Stores the status to string mappings
                    static const char * const m_status_str[trans_job_status::size];

                };

                /**
                 * The stream output operator for the given translation job status instance
                 * @param os the output stream
                 * @param status the status to be output
                 * @return the output stream
                 */
                ostream& operator<<(ostream& os, const trans_job_status& status);

            }
        }
    }
}

#endif /* TRANS_JOB_STATUS_HPP */

