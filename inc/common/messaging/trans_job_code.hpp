/* 
 * File:   job_result_code.hpp
 * Author: zapreevis
 *
 * Created on January 21, 2016, 3:07 PM
 */

#ifndef JOB_RESULT_CODE_HPP
#define	JOB_RESULT_CODE_HPP

namespace uva {
    namespace smt {
        namespace decoding {
            namespace common {
                namespace messaging {

                    /**
                     * Stores the translation job result codes, currently
                     * there is just two results possible, the job is
                     * done - OK; or there was some error - ERROR
                     */
                    enum trans_job_code {
                        RESULT_UNDEFINED = 0,
                        RESULT_OK = RESULT_UNDEFINED + 1,
                        RESULT_ERROR = RESULT_OK + 1,
                        RESULT_PARTIAL = RESULT_ERROR + 1,
                        size = RESULT_ERROR + 1
                    };
                }
            }
        }
    }
}

#endif	/* JOB_RESULT_CODE_HPP */

