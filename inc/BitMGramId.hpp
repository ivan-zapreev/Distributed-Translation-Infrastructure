/* 
 * File:   BitMGramId.hpp
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
 * Created on September 14, 2015, 10:36 AM
 */

#ifndef BITGRAMID_HPP
#define	BITGRAMID_HPP

#include <string>       // std::string

#include "Globals.hpp"
#include "Exceptions.hpp"

#include "TextPieceReader.hpp"
#include "HashingUtils.hpp"
#include "MathUtils.hpp"
#include "MGrams.hpp"

using namespace std;
using namespace uva::utils::math::log2;
using namespace uva::utils::math::bits;
using namespace uva::smt::hashing;
using namespace uva::smt::logging;
using namespace uva::smt::file;
using namespace uva::smt::tries::mgrams::M_Gram_Id;

namespace uva {
    namespace smt {
        namespace tries {
            namespace mgrams {

                namespace __Bit_M_Gram_Id{

                    //The memory in bits needed to store different M-gram id types in
                    //the M-gram id byte arrays
                    //These are the minimum values, if used then we use fewest memory
                    //but the data is not byte aligned. Therefore some bit copying operations
                    //are not done efficiently. Perhaps it is worth trying to round these
                    //values up to full bytes, this can improve performance @ some memory costs.

                    //The number of bites needed to store a 2-gram id type
                    //Possible id types: 32^2 = 1,024
                    //The number of bits needed to store the type is log_2(1,024) = 10
                    static const uint8_t M_GRAM_2_ID_TYPE_LEN_BITS = 10;
                    //The number of bites needed to store a 3-gram id type
                    //Possible id types: 32^3 = 32,768
                    //The number of bits needed to store the type is log_2(32,768) = 15
                    static const uint8_t M_GRAM_3_ID_TYPE_LEN_BITS = 15;
                    //The number of bites needed to store a 4-gram id type
                    //Possible id types: 32^4 = 1,048,576
                    //The number of bits needed to store the type is log_2(1,048,576) = 20
                    static const uint8_t M_GRAM_4_ID_TYPE_LEN_BITS = 20;
                    //The number of bites needed to store a 5-gram id type
                    //Possible id types: 32^5 = 33,554,432
                    //The number of bits needed to store the type is log_2(33,554,432) = 25
                    static const uint8_t M_GRAM_5_ID_TYPE_LEN_BITS = 25;
                    //The number of bites needed to store a 6-gram id type
                    //Possible id types: 32^6 = 1,073,741,824
                    //The number of bits needed to store the type is log_2(1,073,741,824) = 30
                    static const uint8_t M_GRAM_6_ID_TYPE_LEN_BITS = 30;

                    //The length of the M-gram id types in bits depending on the M-Gram level starting from 2.
                    static constexpr uint8_t M_GRAM_ID_TYPE_LEN_BITS[] = {
                        0, 0,
                        M_GRAM_2_ID_TYPE_LEN_BITS,
                        M_GRAM_3_ID_TYPE_LEN_BITS,
                        M_GRAM_4_ID_TYPE_LEN_BITS,
                        M_GRAM_5_ID_TYPE_LEN_BITS,
                        M_GRAM_6_ID_TYPE_LEN_BITS
                    };

                    //Stores the maximum number of bits up to and including M-grams
                    //of level 6.  We use sizeof (TShortId) as each wordId is of type
                    //TShortId, and the maximum number of bits is thus defined by the
                    //number of word_ids in the M-gram and their max size in bytes.
                    static constexpr uint8_t M_GRAM_MAX_ID_LEN_BYTES[] = {
                        0, 0,
                        2 * sizeof (TShortId) + NUM_BYTES_4_BITS(M_GRAM_2_ID_TYPE_LEN_BITS), // 2 TShortId values for 2 word ids, plus the memory needed to store type
                        3 * sizeof (TShortId) + NUM_BYTES_4_BITS(M_GRAM_3_ID_TYPE_LEN_BITS), // 3 TShortId values for 3 word ids, plus the memory needed to store type
                        4 * sizeof (TShortId) + NUM_BYTES_4_BITS(M_GRAM_4_ID_TYPE_LEN_BITS), // 4 TShortId values for 4 word ids, plus the memory needed to store type
                        5 * sizeof (TShortId) + NUM_BYTES_4_BITS(M_GRAM_5_ID_TYPE_LEN_BITS), // 5 TShortId values for 5 word ids, plus the memory needed to store type
                        6 * sizeof (TShortId) + NUM_BYTES_4_BITS(M_GRAM_6_ID_TYPE_LEN_BITS) // 6 TShortId values for 6 word ids, plus the memory needed to store type
                    };
                }                
                
                /**
                 * The bit-compressed implementation of the M-gram id class
                 * 
                 * WARNING: Storing bytes instead of bits does not really have any visible
                 * impact on memory consumption but has a big impact on performance!
                 */
                struct Bit_M_Gram_Id {

                    /**
                     * This method allows to re-initialize this class with a new M-gram id for the given M-gram.
                     * a) If there was no memory allocated for the M-gram id then there will be allocated as much
                     * as needed to store the given id.
                     * b) If there was memory allocated then no re-allocation will be done, then it is assumed
                     * theat this instance was created with the one argument constructor of this class allocated
                     * maximum needed memory for this level. Then the argument M-gram level must be smaller or
                     * equal to  the level this object was created with.
                     * @param word_ids the pointer to the array of word ids
                     * @param begin_idx the M-gram to create the id for
                     * @param num_word_ids the number of word ids
                     * @param m_p_gram_id the pointer to the data storage to be initialized
                     * @return true if the M-gram id could be created, otherwise false
                     */
                    static void create_m_gram_id(const TShortId * word_ids,
                            const uint8_t begin_idx, const uint8_t num_word_ids,
                            T_Gram_Id_Storage_Ptr & m_p_gram_id);

                    /**
                     * The basic constructor that allocates maximum memory
                     * needed to store the M-gram id of the given level.
                     * @param level the level of the M-grams this object will store id for.
                     * @param m_p_gram_id the pointer to initialize
                     */
                    static inline void allocate_bit_m_gram_id(const TModelLevel level, T_Gram_Id_Storage_Ptr & m_p_gram_id) {
                        //Do the sanity check for against overflows
                        if (DO_SANITY_CHECKS && (level > M_GRAM_LEVEL_6)) {
                            stringstream msg;
                            msg << "Bit_M_Gram_Id: Unsupported m-gram level: "
                                    << SSTR(level) << ", must be within ["
                                    << SSTR(M_GRAM_LEVEL_2) << ", "
                                    << SSTR(M_GRAM_LEVEL_6) << "], see M_GRAM_MAX_ID_LEN_BYTES array!";
                            throw Exception(msg.str());
                        }

                        //Allocate maximum memory that could be needed to store the given M-gram level id
                        allocate_m_gram_id(m_p_gram_id, __Bit_M_Gram_Id::M_GRAM_MAX_ID_LEN_BYTES[level]);
                    }

                    /**
                     * Allows to compare two M-Gram ids of a fixed M-gram level
                     * @param m_p_gram_id_one the first M-gram id
                     * @param m_p_gram_id_two the second M-gram id
                     * @return Negative value if one is smaller than two
                     *         Zero if one is equal to two
                     *         Positive value if one is larger than two
                     */
                    template<TModelLevel M_GRAM_LEVEL>
                    static int compare(const T_Gram_Id_Storage_Ptr & m_p_gram_id_one, const T_Gram_Id_Storage_Ptr & m_p_gram_id_two);

                    /**
                     * This is a fore-declaration of the function that can compare two M-gram ids of the same given level
                     * @param one the first M-gram to compare
                     * @param two the second M-gram to compare
                     * @param level the M-grams' level M
                     * @return true if the first M-gram is "smaller" than the second, otherwise false
                     */
                    static bool is_equal_m_grams_id(const T_Gram_Id_Storage_Ptr & one, const T_Gram_Id_Storage_Ptr & two, const TModelLevel level);

                    /**
                     * This is a fore-declaration of the function that can compare two M-gram ids of the same given level
                     * @param one the first M-gram to compare
                     * @param two the second M-gram to compare
                     * @param level the M-grams' level M
                     * @return true if the first M-gram is "smaller" than the second, otherwise false
                     */
                    static bool is_less_m_grams_id(const T_Gram_Id_Storage_Ptr & one, const T_Gram_Id_Storage_Ptr & two, const TModelLevel level);

                    /**
                     * This is a fore-declaration of the function that can compare two M-gram ids of the same given level
                     * @param one the first M-gram to compare
                     * @param two the second M-gram to compare
                     * @param level the M-grams' level M
                     * @return true if the first M-gram is "larger" than the second, otherwise false
                     */
                    static bool is_more_m_grams_id(const T_Gram_Id_Storage_Ptr & one, const T_Gram_Id_Storage_Ptr & two, const TModelLevel level);
                };
            }
        }
    }
}


#endif	/* COMPMGRAMID_HPP */

