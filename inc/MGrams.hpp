/* 
 * File:   MGrams.hpp
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
 * Created on September 7, 2015, 9:42 PM
 */

#ifndef MGRAMS_HPP
#define	MGRAMS_HPP

#include <string>       // std::string

#include "Globals.hpp"
#include "Exceptions.hpp"

#include "TextPieceReader.hpp"
#include "HashingUtils.hpp"
#include "MathUtils.hpp"

#include "BasicWordIndex.hpp"
#include "CountingWordIndex.hpp"
#include "OptimizingWordIndex.hpp"

using namespace std;
using namespace uva::utils::math::log2;
using namespace uva::utils::math::bits;
using namespace uva::smt::hashing;
using namespace uva::smt::logging;
using namespace uva::smt::file;
using namespace uva::smt::tries::dictionary;

namespace uva {
    namespace smt {
        namespace tries {
            namespace mgrams {

                //Various M-gram levels
                const static TModelLevel M_GRAM_LEVEL_UNDEF = 0u;
                const static TModelLevel M_GRAM_LEVEL_1 = 1u;
                const static TModelLevel M_GRAM_LEVEL_2 = 2u;
                const static TModelLevel M_GRAM_LEVEL_3 = 3u;
                const static TModelLevel M_GRAM_LEVEL_4 = 4u;
                const static TModelLevel M_GRAM_LEVEL_5 = 5u;
                const static TModelLevel M_GRAM_LEVEL_6 = 6u;
                const static TModelLevel M_GRAM_LEVEL_7 = 7u;

                /**
                 * This class is used to store the N-Gram data of the back-off Language Model.
                 */
                template<typename WordIndexType>
                class T_M_Gram {
                public:

                    //Stores the unknown word masks for the probability computations,
                    //up to and including 8-grams:
                    // 10000000, 01000000, 00100000, 00010000,
                    // 00001000, 00000100, 00000010, 00000001
                    static constexpr uint8_t UNK_WORD_MASKS[] = {
                        0x80, //0: 10000000
                        0x40, //1: 01000000
                        0x20, //2: 00100000
                        0x10, //3: 00010000
                        0x08, //4: 00001000
                        0x04, //5: 00000100
                        0x02, //6: 00000010
                        0x01 //7: 00000001
                    };

                    //Stores the unknown word masks for the probability computations,
                    //up to and including 8-grams:
                    // 00000000,
                    // 00000001, 00000011, 00000111, 00001111,
                    // 00011111, 00111111, 01111111, 11111111
                    static constexpr uint8_t PROB_UNK_MASKS[] = {
                        0x00, //0: 00000000
                        0x01, //1: 00000001
                        0x03, //2: 00000011
                        0x07, //3: 00000111
                        0x0F, //4: 00001111
                        0x1F, //5: 00011111
                        0x3F, //6: 00111111
                        0x7F, //7: 01111111
                        0xFF //8: 11111111
                    };

                    //Stores the unknown word masks for the back-off weight computations,
                    //up to and including 8-grams:
                    // 00000000,
                    // 00000010, 00000110, 00001110, 00011110,
                    // 00111110, 01111110, 11111110
                    static constexpr uint8_t BACK_OFF_UNK_MASKS[] = {
                        0x00, //0: 00000000
                        0x02, //1: 00000010
                        0x06, //2: 00000110
                        0x0E, //3: 00001110
                        0x1E, //4: 00011110
                        0x3E, //5: 00111110
                        0x7E, //6: 01111110
                        0xFE //7: 11111110
                    };

                    //The maximum supported level of the m-gram
                    static constexpr TModelLevel MAX_LEVEL = sizeof (UNK_WORD_MASKS);

                    //The index of the last m-gram word in the tokens and word ids array
                    static constexpr TModelLevel END_WORD_IDX = MAX_LEVEL - 1;

                    //Stores the m-gram probability, the log_10 probability of the N-Gram Must be a negative value
                    TLogProbBackOff m_prob;

                    //Stores the m-gram log_10 back-off weight (probability) of the N-gram can be 0 is the probability is not available
                    TLogProbBackOff m_back_off;

                    //Stores, if needed, the m-gram's context i.e. for "w1 w2 w3" -> "w1 w2"
                    TextPieceReader m_context;

                    //The data structure to store the N-gram word ids
                    TShortId m_word_ids[MAX_LEVEL] = {};

                    //Stores the m-gram level, the number of meaningful elements in the tokens, the value of m for the m-gram
                    TModelLevel m_used_level;

                    /**
                     * The basic constructor
                     * @param word_index the used word index
                     */
                    T_M_Gram(WordIndexType & word_index) : m_word_index(word_index) {
                    }

                    /**
                     * Allows to prepare the M-gram for being queried. 
                     */
                    inline void prepare_for_querying() {
                        //Store the word ids and the unknown word flags and pre-compute the m-gram hash values
                        store_m_gram_ids_and_hashes<true, false>();
                    }

#define HASH_LEVEL_IDX(level) ( 2 * ((level) - 1) )
#define BO_HASH_LEVEL_IDX(level) ( HASH_LEVEL_IDX(level) + 1 )

#define HASH_LEVEL_VALUE_IDX(level) ( MAX_LEVEL - (level) )
#define BO_HASH_LEVEL_VALUE_IDX(level) ( HASH_LEVEL_VALUE_IDX(level) - 1 )

                    /**
                     * Allows to prepare the M-gram for being used for adding it to the trie
                     * This includes registering the one gram in the word index
                     */
                    inline void prepare_for_adding() {
                        //If we have a unigram then add it to the index otherwise get the word ids
                        if (m_used_level == M_GRAM_LEVEL_1) {
                            //Get the word id and its hash
                            m_word_ids[END_WORD_IDX] = m_word_index.register_word(get_end_token());
                            //Set the hash value of the e-gram to be equal to its id
                            m_hash_values[HASH_LEVEL_IDX(MAX_LEVEL)][HASH_LEVEL_VALUE_IDX(M_GRAM_LEVEL_1)] = m_word_ids[END_WORD_IDX];
                            //Note: There is no back-off hash value for one-gram
                        } else {
                            //Store the word ids without the unknown word flags and pre-compute the m-gram hash values
                            store_m_gram_ids_and_hashes<false, true>();
                        }
                    }

                    /**
                     * Allows to compute the hash for the given sub-m-gram that is
                     * defined by the level and whether it is a back-off m-gram or not
                     * @param is_back_off true if this is a back-off case
                     * @param curr_level the level of the sub-mgram
                     * @return the resulting hash value
                     */
                    template<bool is_tokens, bool is_back_off, TModelLevel curr_level>
                    inline uint64_t get_hash() const {
                        if (is_tokens) {
                            constexpr TModelLevel begin_idx = (MAX_LEVEL - curr_level);
                            constexpr TModelLevel end_idx = END_WORD_IDX;
                            if (is_back_off) {
                                return hash_tokens < begin_idx - 1, end_idx - 1 > ();
                            } else {
                                return hash_tokens<begin_idx, end_idx> ();
                            }
                        } else {
                            return hash_word_ids<is_back_off, curr_level>();
                        }
                    }

                    /**
                     * Gets the word hash for the end word of the back-off M-Gram
                     * @return the word hash for the end word of the back-off M-Gram
                     */
                    inline TShortId get_back_off_end_word_id() const {
                        //The word ids are always aligned to the end of the array
                        //so the end word id for the back off m-gram is fixed!
                        return m_word_ids[END_WORD_IDX - 1];
                    }

                    /**
                     * Gets the word hash for the last word in the M-gram
                     * @return the word hash for the last word in the M-gram
                     */
                    inline TShortId get_end_word_id() const {
                        //The word ids are always aligned to the end of the array
                        //so the end word id for the probability m-gram is fixed!
                        return m_word_ids[END_WORD_IDX];
                    }

                    /**
                     * Allows to check if the given back-off sub-m-gram contains 
                     * an unknown word for the given current level.
                     */
                    template<bool is_back_off, TModelLevel curr_level>
                    inline bool has_no_unk_words() const {
                        uint8_t level_flags = (m_unk_word_flags & ((is_back_off) ? BACK_OFF_UNK_MASKS[curr_level] : PROB_UNK_MASKS[curr_level]));

                        LOG_DEBUG << "The " << ((is_back_off) ? "back-off" : "probability")
                                << " level: " << curr_level << " unknown word flags are: "
                                << bitset<NUM_BITS_IN_UINT_8>(level_flags) << ", originals are: "
                                << bitset<NUM_BITS_IN_UINT_8>(m_unk_word_flags) << END_LOG;

                        return (level_flags == 0);
                    }

                    /**
                     * Converts the given tokens to ids and stores it in
                     * m_gram_word_ids. The ids are aligned to the beginning
                     * of the m_gram_word_ids[N-1] array.
                     * @param is_unk_flags if true then the unk word flags will be stored, otherwise not
                     * @param m_gram the m-gram tokens to convert to hashes
                     * @paam unk_word_flags the variable into which the word flags will be stored.
                     */
                    template<bool is_unk_flags, bool is_max_level_only>
                    inline void store_m_gram_ids_and_hashes() {
                        if (is_unk_flags) {
                            //Re-initialize the flags with zero
                            m_unk_word_flags = 0;
                        }

                        LOG_DEBUG1 << "Computing ids for the words of a " << SSTR(m_used_level) << "-gram:" << END_LOG;

                        //Store the reference to the needed array level, for regular and back-off m-gram hashes
                        uint64_t(&hash_values)[MAX_LEVEL] = m_hash_values[HASH_LEVEL_IDX(MAX_LEVEL)];
                        uint64_t(&bo_hash_values)[MAX_LEVEL] = m_hash_values[BO_HASH_LEVEL_IDX(MAX_LEVEL)];

                        //Get the word ids and compute the hashes
                        TModelLevel idx = MAX_LEVEL;
                        do {
                            //Decrement the index
                            idx--;

                            //Get the word id, or UNKNOWN_WORD_ID if the word is unknown
                            m_word_ids[idx] = m_word_index.get_word_id(m_tokens[idx]);
                            LOG_DEBUG1 << "wordId('" << m_tokens[idx].str() << "') = " << SSTR(m_word_ids[idx]) << END_LOG;

                            //If needed set the unknown word flag
                            if (is_unk_flags && (m_word_ids[idx] == WordIndexType::UNKNOWN_WORD_ID)) {
                                m_unk_word_flags |= UNK_WORD_MASKS[idx];
                            }

                            //Compute the max level hash with the intermediate values
                            if (idx == HASH_LEVEL_VALUE_IDX(M_GRAM_LEVEL_1)) {
                                //The first top-level sub 1-gram hash is equal to the end word hash
                                hash_values[idx] = m_word_ids[idx];
                            } else {
                                if (idx == BO_HASH_LEVEL_VALUE_IDX(M_GRAM_LEVEL_1)) {
                                    //The first top-level back-off 1-gram hash is the
                                    //id of the word word preceeding the last one
                                    bo_hash_values[idx] = m_word_ids[idx];
                                } else {
                                    //The next top-level back-off m-gram hash is the combination of the
                                    //next previous word and the previous top-level back-off m-gram hash
                                    bo_hash_values[idx] = combine_hash(bo_hash_values[idx + 1], m_word_ids[idx]);
                                }
                                //The top-level sub m-gram hash is the back-off hash combined with the end word hash
                                hash_values[idx] = combine_hash(hash_values[idx + 1], m_word_ids[idx]);
                            }
                            LOG_DEBUG2 << "hash_values[" << (uint32_t) idx << "] = " << hash_values[idx] << END_LOG;
                            LOG_DEBUG2 << "bo_hash_values[" << (uint32_t) idx << "] = " << bo_hash_values[idx] << END_LOG;
                        } while (idx != begin_word_index());

                        if (is_unk_flags) {
                            LOG_DEBUG << "The query unknown word flags are: " << bitset<NUM_BITS_IN_UINT_8>(m_unk_word_flags) << END_LOG;
                        }
                    }

                    /**
                     * For the given N-gram allows to give the string of the object
                     * for which the probability is computed, e.g.:
                     * N-gram = "word1" -> result = "word1"
                     * N-gram = "word1 word2 word3" -> result = "word3 | word1  word2"
                     * @return the resulting string
                     */
                    inline string get_mgram_prob_str() const {
                        if (m_used_level == M_GRAM_LEVEL_1) {
                            const TextPieceReader & token = get_end_token();
                            return token.str().empty() ? "<empty>" : token.str();
                        } else {
                            if (m_used_level == M_GRAM_LEVEL_UNDEF) {
                                return "<none>";
                            } else {
                                string result = m_tokens[END_WORD_IDX].str() + " |";
                                for (TModelLevel idx = begin_word_index(); idx != END_WORD_IDX; idx++) {
                                    result += string(" ") + m_tokens[idx].str();
                                }
                                return result;
                            }
                        }
                    }

                    /**
                     * Tokenise a given piece of text into a set of text peices.
                     * The text piece should be a M-gram piece - a space separated
                     * string of words.
                     * @param text the piece of text to tokenise
                     * @param gram the gram container to put data into
                     */
                    inline void set_m_gram_from_text(TextPieceReader &text) {
                        m_curr_index = END_WORD_IDX;

                        //Read the tokens one by one backwards and decrement the index
                        while (text.get_last_space(m_tokens[m_curr_index--]));

                        //Set the read m-gram level, do "END_WORD_IDX - 1" as
                        //the value of m_curr_index is decreased unconditionally
                        m_used_level = ((END_WORD_IDX - 1) - m_curr_index);
                    }

                    /**
                     * Allows to start a new M-gram with the given level
                     * @param level the level of the M-gram we are starting
                     */
                    inline void start_new_m_gram(const TModelLevel level) {
                        m_curr_index = (MAX_LEVEL - level);
                    }

                    /**
                     * Returns the reference to the next new token of the m-gram
                     * @return the reference to the next new token of the m-gram
                     */
                    inline TextPieceReader & get_next_new_token() {
                        if (DO_SANITY_CHECKS && (m_curr_index > END_WORD_IDX)) {
                            stringstream msg;
                            msg << "The next token does not exist, exceeded the maximum of " << SSTR(MAX_LEVEL) << " elements!";
                            throw Exception(msg.str());
                        }
                        return m_tokens[m_curr_index++];
                    }

                    /**
                     * Return the last token of the read m-gram
                     * @return the reference to the last token of the m-gram
                     */
                    inline const TextPieceReader & get_end_token() const {
                        return m_tokens[END_WORD_IDX];
                    }

                    /**
                     * If the context is set to be all the m-gram tokens, then this method
                     * allows to exclude the last token from it. It also takes care of the
                     * space in between. Note that, this method must only be called if the
                     * context is initialized and it he m-gram level m is greater than one. 
                     */
                    inline void exclude_last_token_from_context() {
                        //The reduction factor for length is the length of the last m-gram's token plus
                        //one character which is the space symbol located between m-gram tokens.
                        const size_t reduction = (get_end_token().length() + 1);
                        m_context.set(m_context.get_begin_ptr(), m_context.length() - reduction);
                    }

                    /**
                     * The basic to string conversion operator for the m-gram
                     */
                    inline operator string() const {
                        LOG_DEBUG4 << "Appending " << SSTR(m_used_level) << "tokens" << END_LOG;
                        return tokens_to_string(m_tokens, MAX_LEVEL - m_used_level, END_WORD_IDX);
                    };

                    /**
                     * Allows to get the word index used in this m-gram
                     * @return the word index
                     */
                    inline WordIndexType & get_word_index() const {
                        return m_word_index;
                    }

                private:
                    //Stores the m-gram idx
                    TModelLevel m_curr_index;

                    //Stores the m-gram tokens
                    TextPieceReader m_tokens[MAX_LEVEL];

                    //Unknown word bit flags
                    uint8_t m_unk_word_flags = 0;

                    //Stores the reference to the used word index
                    WordIndexType & m_word_index;

                    //Stores the pre-computes hash values for the m-gram
                    //This is a 2D array with twice the number of levels
                    //for regular and back-off m-gram hashes. The second
                    //index is for sub-m-gram hashes.
                    uint64_t m_hash_values[2 * MAX_LEVEL][MAX_LEVEL];

                    /**
                     * Gives the start word index
                     * @return the start word index.
                     */
                    inline TModelLevel begin_word_index() const {
                        return (MAX_LEVEL - m_used_level);
                    }

                    /**
                     * This function allows to compute the hash of the sub M-Gram
                     * tokens starting from and including the word on the given index,
                     * and until and including the word of the given index. It
                     * assumes, which should hold, that the memory pointed by the
                     * tokens is continuous.
                     * @param begin_idx  the index of the first word in tokens array
                     * @param end_idx the index of the last word in tokens array
                     * @return the hash value of the given token
                     */
                    template<TModelLevel begin_idx, TModelLevel end_idx>
                    inline uint64_t hash_tokens() const {
                        LOG_DEBUG3 << "Hashing tokens begin_idx: " << begin_idx << ", end_idx: " << end_idx << END_LOG;

                        //Compute the length of the gram tokens in memory, including spaces between
                        const char * beginFirstPtr = m_tokens[begin_idx].get_begin_c_str();
                        const TextPieceReader & last = m_tokens[end_idx];
                        const char * beginLastPtr = last.get_begin_c_str();
                        const size_t totalLen = (beginLastPtr - beginFirstPtr) + last.length();
                        LOG_DEBUG3 << "Hashing tokens length: " << totalLen << END_LOG;

                        //If the sanity check is on then test that the memory is continuous
                        //Compute the same length but with a longer iterative algorithms
                        if (DO_SANITY_CHECKS) {
                            //Compute the exact length
                            size_t exactTotalLen = (end_idx - begin_idx); //The number of spaces in between tokens
                            for (TModelLevel idx = begin_idx; idx <= end_idx; idx++) {
                                exactTotalLen += m_tokens[idx].length();
                            }
                            //Check that the exact and fast computed lengths are the same
                            if (exactTotalLen != totalLen) {
                                stringstream msg;
                                msg << "The memory allocation for M-gram tokens is not continuous: totalLen (" <<
                                        SSTR(totalLen) << ") != exactTotalLen (" << SSTR(exactTotalLen) << ")";
                                throw Exception(msg.str());
                            }
                        }

                        //Compute the hash using the gram tokens with spaces with them
                        return compute_hash(beginFirstPtr, totalLen);
                    }

                    /**
                     * This function allows to compute the hash of the sub M-Gram
                     * word ids starting from and including the word on the given index,
                     * and until and including the word of the given index.
                     * @param is_back_off true if we need hash for back-off m-gram otherwise false
                     * @param curr_level the m-gram level we need hash for
                     * @return the resulting hash value
                     */
                    template<bool is_back_off, TModelLevel curr_level>
                    inline uint64_t hash_word_ids() const {
                        LOG_DEBUG3 << "Hashing tokens is_back_off: " << is_back_off << ", curr_level: " << curr_level << END_LOG;
                        if (is_back_off) {
                            return m_hash_values[BO_HASH_LEVEL_IDX(MAX_LEVEL)][BO_HASH_LEVEL_VALUE_IDX(curr_level)];
                        } else {
                            return m_hash_values[HASH_LEVEL_IDX(MAX_LEVEL)][HASH_LEVEL_VALUE_IDX(curr_level)];
                        }
                    }
                };

                template<typename WordIndexType>
                constexpr TModelLevel T_M_Gram<WordIndexType>::MAX_LEVEL;

                template<typename WordIndexType>
                constexpr TModelLevel T_M_Gram<WordIndexType>::END_WORD_IDX;

                template<typename WordIndexType>
                constexpr uint8_t T_M_Gram<WordIndexType>::UNK_WORD_MASKS[];

                template<typename WordIndexType>
                constexpr uint8_t T_M_Gram<WordIndexType>::PROB_UNK_MASKS[];

                template<typename WordIndexType>
                constexpr uint8_t T_M_Gram<WordIndexType>::BACK_OFF_UNK_MASKS[];

                //Make sure that there will be templates instantiated, at least for the given parameter values
                template class T_M_Gram<BasicWordIndex>;
                template class T_M_Gram<CountingWordIndex>;
                template class T_M_Gram<TOptBasicWordIndex>;
                template class T_M_Gram<TOptCountWordIndex>;

                /**
                 * The compressed implementation of the M-gram id class
                 * Made in form of a namespace for the sake of minimizing the
                 * memory consumption
                 */
                namespace M_Gram_Id {
                    //define the basic type block for the M-gram id
                    typedef uint8_t T_Gram_Id_Storage;

                    //Define the basic type as an alias for the compressed M-Gram id
                    typedef T_Gram_Id_Storage * T_Gram_Id_Storage_Ptr;

                    /**
                     * The basic constructor that allocates maximum memory
                     * needed to store the M-gram id of the given level.
                     * @param level the level of the M-grams this object will store id for.
                     * @param m_p_gram_id the pointer to initialize
                     */
                    static inline void allocate_m_gram_id(T_Gram_Id_Storage_Ptr & m_p_gram_id, uint8_t size) {
                        //Allocate maximum memory that could be needed to store the given M-gram level id
                        m_p_gram_id = new uint8_t[size];
                        LOG_DEBUG3 << "Allocating a M_Gram_Id: " << (void*) m_p_gram_id << " of size " << (uint32_t) size << END_LOG;
                    }

                    /**
                     * Allows to destroy the M-Gram id if it is not NULL.
                     * @param m_p_gram_id the M-gram id pointer to destroy
                     */
                    static inline void destroy(T_Gram_Id_Storage_Ptr & m_p_gram_id) {
                        if (m_p_gram_id != NULL) {
                            LOG_DEBUG3 << "Deallocating a M_Gram_Id: " << (void*) m_p_gram_id << END_LOG;
                            delete[] m_p_gram_id;
                        }
                    }
                }
            }
        }
    }
}

#endif	/* MGRAM_HPP */

