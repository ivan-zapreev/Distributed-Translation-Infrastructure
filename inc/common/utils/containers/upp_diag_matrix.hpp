/* 
 * File:   upper_triangular_matrix.hpp
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
 * Created on February 15, 2016, 11:58 AM
 */

#ifndef UPP_DIAG_MATRIX_HPP
#define UPP_DIAG_MATRIX_HPP


#include <string>       // std::stringstream
#include <cstdlib>      // std::qsort std::bsearch
#include <algorithm>    // std::sort std::abs
#include <functional>   // std::function

#include "common/utils/logging/logger.hpp"
#include "common/utils/exceptions.hpp"

using namespace std;
using namespace uva::utils::exceptions;
using namespace uva::utils::logging;

namespace uva {
    namespace utils {
        namespace containers {

            /**
             * This class represents the square matrix that should save on
             * memory when used with e.g. upper/low diagonal matrix rows.
             * Note 1: This class is thread safe as long as you do not try
             * to work with the same element from different threads.
             * Note 2: This class assumes the proper acces to the matrix elements.
             * I.e. matrix indexes are not checked, writing the the elements below
             * the diagonal will also result in broken data.
             */
            template<typename element_type>
            class upp_diag_matrix {
            public:
                //Declare the pointer to the lement type
                typedef element_type * element_type_ptr;

                //Stores the minimum dimension index
                static constexpr int32_t m_min_idx = 0;

                //Stores the maximum dimension index
                const int32_t m_max_idx;

                /**
                 * The basic constructor
                 * @param dimension the dimension of the matrix, it will be a square upper diagonal matrix.
                 */
                upp_diag_matrix(const size_t dim) : m_max_idx(dim - 1), m_dim(dim) {
                    LOG_DEBUG2 << "Creating the upper diagonal matrix of " << m_dim << " dimensions" << END_LOG;

                    //Compute the number of elements to be used
                    const size_t num_elements = (dim - 1) * dim / 2 + dim;

                    LOG_DEBUG2 << "Allocating " << num_elements << " elements." << END_LOG;

                    //First allocate the number of elements we need
                    m_elems = new element_type[num_elements]();

                    //Allocate the row pointers array
                    m_rows = new element_type_ptr[dim]();

                    //Initialize the row pointers array
                    m_rows[0] = m_elems;
                    for (size_t idx = 1; idx < dim; ++idx) {
                        const size_t offset = dim - idx;
                        LOG_DEBUG2 << "Internal row: " << idx << " offset: " << offset << END_LOG;
                        m_rows[idx] = m_rows[idx - 1] + offset;
                    }

                    LOG_DEBUG2 << "upp_diag_matrix create is finished" << END_LOG;
                }

                /**
                 * The basic destructor
                 */
                ~upp_diag_matrix() {
                    //Delete the row reference array
                    if (m_rows != NULL) {
                        delete[] m_rows;
                        m_rows = NULL;
                    }
                    //Delete the elements array
                    if (m_elems != NULL) {
                        delete[] m_elems;
                        m_elems = NULL;
                    }
                }

                /**
                 * Allows to retrieve the reference to the dimension of this square matrix
                 * @return the reference to the dimension of the scquare matrix
                 */
                inline const size_t & get_dim() const {
                    return m_dim;
                }

                /**
                 * Allows to access the matrix row with the given index
                 * @param idx the row index
                 * @return the pointer to the matrix row array
                 */
                inline element_type * operator[](size_t idx) const {
                    //Return the row
                    return m_rows[idx];
                }
            private:
                //Stores the dimension of the square matrix
                const size_t m_dim;
                //Stores the array of matrix elements
                element_type * m_elems;
                //Stores the array of pointers to matrix rows
                element_type_ptr * m_rows;
            };

            template<typename element_type>
            constexpr int32_t upp_diag_matrix<element_type>::m_min_idx;
        }
    }
}

#endif /* UPPER_TRIANGULAR_MATRIX_HPP */

