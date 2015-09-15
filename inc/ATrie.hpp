/* 
 * File:   ATrie.hpp
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
 * Created on September 3, 2015, 2:59 PM
 */

#ifndef ATRIE_HPP
#define	ATRIE_HPP

#include <string>       // std::string

#include "Globals.hpp"
#include "Exceptions.hpp"
#include "Logger.hpp"
#include "MathUtils.hpp"

#include "TextPieceReader.hpp"
#include "AWordIndex.hpp"
#include "MGrams.hpp"
#include "BitmapHashCache.hpp"

using namespace std;
using namespace uva::smt::logging;
using namespace uva::smt::file;
using namespace uva::smt::tries::caching;
using namespace uva::smt::tries::dictionary;
using namespace uva::smt::tries::mgrams;
using namespace uva::utils::math::bits;

namespace uva {
    namespace smt {
        namespace tries {

            /**
             * This structure is used to define the trivial probability/
             * back-off pari to be stored for M-grams with 1 <= M < N
             * @param prob stores the probability
             * @param back_off stores the back-off
             */
            typedef struct {
                TLogProbBackOff prob;
                TLogProbBackOff back_off;
            } TProbBackOffEntry;

            /**
             * This data structure is to be used to return the N-Gram query result.
             * It contains the computed Back-Off language model probability and
             * potentially additional meta data for the decoder
             * @param prob the computed Back-Off language model probability as log_${LOG_PROB_WEIGHT_BASE}
             */
            typedef struct {
                TLogProbBackOff prob;
            } TQueryResult;

            /**
             * This is a common abstract class for a Trie implementation.
             * The purpose of having this as a template class is performance optimization.
             * @param N - the maximum level of the considered N-gram, i.e. the N value
             */
            template<TModelLevel N>
            class ATrie {
            public:
                //The offset, relative to the M-gram level M for the mgram mapping array index
                const static TModelLevel MGRAM_IDX_OFFSET = 2;

                //Will store the the number of M levels such that 1 < M < N.
                const static TModelLevel NUM_M_GRAM_LEVELS = N - MGRAM_IDX_OFFSET;

                //Will store the the number of M levels such that 1 < M <= N.
                const static TModelLevel NUM_M_N_GRAM_LEVELS = N - 1;

                //Compute the N-gram index in in the arrays for M and N grams
                static const TModelLevel N_GRAM_IDX_IN_M_N_ARR = N - MGRAM_IDX_OFFSET;

                // Stores the undefined index array value
                static const TShortId UNDEFINED_ARR_IDX = 0;

                // Stores the undefined index array value
                static const TShortId FIRST_VALID_CTX_ID = UNDEFINED_ARR_IDX + 1;

                /**
                 * The basic constructor
                 * @param _wordIndex the word index to be used
                 * @param is_birmap_hash_cache - allows to enable the bitmap hash cache for the M-grams.
                 * the latter records the hashes of all the M-gram present in the tries and then before
                 * querying checks if the hash of the queries M-gram is present if not then we do an
                 * immediate back-off, otherwise we search for the M-gram. This is an experimental feature
                 * therefore it the parameter's default is false. Also this feature is to be used with
                 * the multi-level context index tries which require a lot of searching when looking
                 * for an M-gram.
                 */
                explicit ATrie(AWordIndex * _pWordIndex, bool is_birmap_hash_cache = false)
                : m_word_index_ptr(_pWordIndex), m_query_ptr(NULL),
                m_is_birmap_hash_cache(is_birmap_hash_cache),
                m_unk_word_flags(0) {
                    LOG_INFO3 << "Collision detections are: "
                            << (DO_SANITY_CHECKS ? "ON" : "OFF")
                            << " !" << END_LOG;
                }

                /**
                 * This method can be used to provide the N-gram count information
                 * That should allow for pre-allocation of the memory
                 * @param counts the array of N-Gram counts counts[0] is for 1-Gram
                 */
                virtual void pre_allocate(const size_t counts[N]) {
                    if (m_word_index_ptr != NULL) {
                        m_word_index_ptr->reserve(counts[0]);
                        Logger::updateProgressBar();
                    }

                    //Pre-allocate the bitmap-hash caches if needed
                    if (m_is_birmap_hash_cache) {
                        for (size_t idx = 0; idx < NUM_M_N_GRAM_LEVELS; ++idx) {
                            m_bitmap_hash_cach[idx].pre_allocate(counts[idx + 1]);
                        }
                    }
                };

                /**
                 * Is to be used from the sub-classes from the add_X_gram methods.
                 * This method allows to register the given M-gram in internal high
                 * level caches if present.
                 * @param gram the M-gram to cache
                 */
                inline void register_m_gram_cache(const T_M_Gram &gram) {
                    if (m_is_birmap_hash_cache && (gram.level > M_GRAM_LEVEL_1)) {
                        m_bitmap_hash_cach[gram.level - MGRAM_IDX_OFFSET].add_m_gram(gram);
                    }
                }

                /**
                 * This method adds a 1-Gram (word) to the trie.
                 * It it snot guaranteed that the parameter will be checked to be a 1-Gram!
                 * @param gram the 1-Gram data
                 */
                virtual void add_1_gram(const T_M_Gram &gram) = 0;

                /**
                 * This method adds a M-Gram (word) to the trie where 1 < M < N
                 * @param gram the M-Gram data
                 * @throws Exception if the level of this M-gram is not such that  1 < M < N
                 */
                virtual void add_m_gram(const T_M_Gram & gram) = 0;

                /**
                 * This method adds a N-Gram (word) to the trie where
                 * It it snot guaranteed that the parameter will be checked to be a N-Gram!
                 * @param gram the N-Gram data
                 */
                virtual void add_n_gram(const T_M_Gram & gram) = 0;

                /**
                 * This method allows to check if post processing should be called after
                 * all the X level grams are read. This method is virtual.
                 * @param level the level of the X-grams that were finished to be read
                 */
                virtual bool is_post_grams(const TModelLevel level) {
                    return false;
                }

                /**
                 * This method should be called after all the X level grams are read.
                 * @param level the level of the X-grams that were finished to be read
                 */
                void post_grams(const TModelLevel level) {
                    switch (level) {
                        case M_GRAM_LEVEL_1:
                            this->post_1_grams();
                            break;
                        case N:
                            this->post_n_grams();
                            break;
                        default:
                            this->post_m_grams(level);
                            break;
                    }
                };

                /**
                 * Returns the maximum length of the considered N-Grams
                 * @return the maximum length of the considered N-Grams
                 */
                TModelLevel get_max_level() const {
                    return N;
                }

                /**
                 * This method will get the N-gram in a form of a vector, e.g.:
                 *      [word1 word2 word3 word4 word5]
                 * and will compute and return the Language Model Probability for it
                 * @param m_gram the given M-Gram we are going to query!
                 * @param result the output parameter containing the the result
                 *               probability and possibly some additional meta
                 *               data for the decoder.
                 */
                void query(const T_M_Gram & m_gram, TQueryResult & result) {
                    const TModelLevel level = m_gram.level;

                    //Check the number of elements in the N-Gram
                    if (DO_SANITY_CHECKS && ((level < M_GRAM_LEVEL_1) || (level > N))) {
                        stringstream msg;
                        msg << "An improper N-Gram size, got " << level << ", must be between [1, " << N << "]!";
                        throw Exception(msg.str());
                    } else {
                        //Store the pointer to the M-gram
                        m_query_ptr = &m_gram;

                        //Transform the given M-gram into word hashes.
                        store_m_gram_word_ids(m_gram);

                        //Store unknown word flags
                        store_unk_word_flags();

                        //Go on with a recursive procedure of computing the N-Gram probabilities
                        //get_probability(level, result.prob);

                        //Compute the probability in the loop fashion, should be faster that recursion.
                        //Also do not iterate if we already got a zero log probability, from that moment
                        //on the value can get only smaller, so we can stop with away.
                        result.prob = ZERO_PROB_WEIGHT;
                        TModelLevel curr_level = level;
                        while ((curr_level != 0) && (result.prob > ZERO_LOG_PROB_WEIGHT)) {
                            //Try to compute the next probability with decreased level
                            if (cache_check_add_prob_value(curr_level, result.prob)) {
                                //If the probability has been finally computed stop
                                break;
                            } else {
                                //Decrease the level
                                curr_level--;
                                //Get the back_off 
                                cache_check_add_back_off_weight(curr_level, result.prob);
                            }
                        }
                        //Make the probability to be non smaller than zero
                        result.prob = max(result.prob, ZERO_LOG_PROB_WEIGHT);

                        LOG_DEBUG << "The computed log_" << LOG_PROB_WEIGHT_BASE << " probability is: " << result.prob << END_LOG;
                    }
                }

                /**
                 * Allows to retrieve the stored word index, if any
                 * @return the pointer to the stored word index or NULL if none
                 */
                inline AWordIndex * get_word_index() {
                    return m_word_index_ptr;
                }

                /**
                 * The basic class destructor
                 */
                virtual ~ATrie() {
                    if (m_word_index_ptr != NULL) {
                        delete m_word_index_ptr;
                    }
                };

            protected:
                //Stores the reference to the word index to be used
                AWordIndex * m_word_index_ptr;

                //The temporary data structure to store the N-gram word ids
                TShortId m_tmp_word_ids[N];
                //Stores the pointer to the queries m-gram
                const T_M_Gram * m_query_ptr;

                //Stores a flag of whether we should use the birmap hash cache
                const bool m_is_birmap_hash_cache;

                /**
                 * Gets the word hash for the end word of the back-off M-Gram
                 * @return the word hash for the end word of the back-off M-Gram
                 */
                inline const TShortId & get_back_off_end_word_id() {
                    return m_tmp_word_ids[N - 2];
                }

                /**
                 * Gets the word hash for the last word in the M-gram
                 * @return the word hash for the last word in the M-gram
                 */
                inline const TShortId & get_end_word_id() {
                    return m_tmp_word_ids[N - 1];
                }

                /**
                 * Converts the given tokens to ids and stores it in
                 * m_gram_word_ids. The ids are aligned to the beginning
                 * of the m_gram_word_ids[N-1] array.
                 * @param m_gram the m-gram tokens to convert to hashes
                 */
                inline void store_m_gram_word_ids(const T_M_Gram & m_gram) {
                    if (DO_SANITY_CHECKS && (m_word_index_ptr == NULL)) {
                        throw Exception("The m_p_word_index is not set!");
                    }

                    //The start index depends on the value M of the given M-Gram
                    TModelLevel idx = N - m_gram.level;
                    LOG_DEBUG1 << "Computing hashes for the words of a " << SSTR(m_gram.level) << "-gram:" << END_LOG;
                    for (TModelLevel i = 0; i < m_gram.level; i++) {
                        //Do not check whether the word was found or not, if it was not then the id is UNKNOWN_WORD_ID
                        m_word_index_ptr->get_word_id(m_gram.tokens[i].str(), m_tmp_word_ids[idx]);
                        LOG_DEBUG1 << "wordId('" << m_gram.tokens[i].str() << "') = " << SSTR(m_tmp_word_ids[idx]) << END_LOG;
                        idx++;
                    }
                }

                /**
                 * This method will be called after all the 1-grams are read.
                 * The default implementation of this method is present.
                 * If overridden must be called from the child method.
                 */
                virtual void post_1_grams() {
                };

                /**
                 * This method will be called after all the M-grams are read.
                 * The default implementation of this method is present.
                 * If overridden must be called from the child method.
                 * @param level the level of the M-grams that were finished to be read
                 */
                virtual void post_m_grams(const TModelLevel level) {
                };

                /**
                 * This method will be called after all the N-grams are read.
                 * The default implementation of this method is present.
                 * If overridden must be called from the child method.
                 */
                virtual void post_n_grams() {
                };

                bool cache_check_add_prob_value(const TModelLevel level, TLogProbBackOff & prob) {
                    LOG_DEBUG << "cache_check_add_prob_value(" << level << ") = " << prob << END_LOG;
                    
                    //Try getting the probability value.
                    //1. If the level is one go on: we can get smth
                    //even if the 1-Gram consists of just an unknown word.
                    //2. If the context length is more then one and there is
                    //an unknown word in the gram then it makes no sense to do
                    //searching as there are no M-grams with <unk> in them
                    if ((level == M_GRAM_LEVEL_1) ||
                            ((level > M_GRAM_LEVEL_1) && has_no_unk_words<false>(level)
                            && is_bitmap_hash_cache<false>(level))) {
                        //Let's look further, may be we will find something!
                        LOG_DEBUG1 << "All pre-checks are passed, calling add_prob_value(level, prob)!" << END_LOG;
                        return add_prob_value(level, prob);
                    } else {
                        LOG_DEBUG << "Could try to get probs but it will not be "
                                << "successful due to the present unk words! "
                                << "Thus backing off right away!" << END_LOG;
                        //We already know we need to back-off
                        return false;
                    }
                }

                void cache_check_add_back_off_weight(const TModelLevel level, TLogProbBackOff & prob) {
                    LOG_DEBUG << "cache_check_add_back_off_weight(" << level << ") = " << prob << END_LOG;
                    
                    //Try getting the back-off weight.
                    //1. If the context length is one go on: we can get smth
                    //even if the 1-Gram consists of just an unknown word.
                    //2. If the context length is more then one and there is
                    //an unknown word in the gram then it makes no sense to do
                    //searching as there are no M-grams with <unk> in them
                    if ((level == M_GRAM_LEVEL_1) ||
                            ((level > M_GRAM_LEVEL_1) && has_no_unk_words<true>(level)
                            && is_bitmap_hash_cache<true>(level))) {
                        //Let's look further, we definitely get some back-off weight or zero!
                        LOG_DEBUG1 << "All pre-checks are passed, calling add_back_off_weight(level, prob)!" << END_LOG;
                        add_back_off_weight(level, prob);
                    } else {
                        LOG_DEBUG << "Could try to back off but it will not be "
                                << "successful due to the present unk words! Thus "
                                << "the back-off weight is zero!" << END_LOG;
                    }
                }

                /**
                 * This function should implement the computation of the
                 * N-Gram probabilities in the Back-Off Language Model. The
                 * N-Gram hashes can be obtained from the _wordHashes member
                 * variable of the class.
                 * @param level the M-gram level for which the probability is to be computed
                 * @param prob [out] the reference to the probability to be found/computed
                 */
                inline void get_probability(const TModelLevel level, TLogProbBackOff & prob) {
                    //If the probability is not zero then go on with computing the
                    //back-off. Otherwise it does not make sense to compute further
                    //as we will only get lower probability and we are already at 0!
                    if (prob > ZERO_LOG_PROB_WEIGHT) {
                        //Try getting the probability value.
                        //1. If the level is one go on: we can get smth
                        //even if the 1-Gram consists of just an unknown word.
                        //2. If the context length is more then one and there is
                        //an unknown word in the gram then it makes no sense to do
                        //searching as there are no M-grams with <unk> in them
                        if ((level == M_GRAM_LEVEL_1) ||
                                ((level > M_GRAM_LEVEL_1) && has_no_unk_words<false>(level)
                                && is_bitmap_hash_cache<false>(level))) {
                            get_prob_value(level, prob);
                        } else {
                            LOG_DEBUG << "Could try to get probs but it will not be "
                                    << "successful due to the present unk words! "
                                    << "Thus backing off right away!" << END_LOG;
                            //We already know we need to back-off
                            get_back_off_prob(level - 1, prob);
                        }
                    }
                }

                /**
                 * This function should be called in case we can not get the probability for
                 * the given M-gram and we want to compute it's back-off probability instead
                 * @param level the length of the context for the M-gram for which we could
                 * not get the probability from the trie.
                 * @param prob [out] the reference to the probability to be found/computed
                 */
                inline void get_back_off_prob(const TModelLevel level, TLogProbBackOff & prob) {
                    //Compute the lover level probability
                    get_probability(level, prob);

                    LOG_DEBUG1 << "getProbability(" << level
                            << ") = " << prob << END_LOG;

                    //If the probability is not zero then go on with computing the
                    //back-off. Otherwise it does not make sense to compute back-off
                    //as we will only get lower probability and we are already at 0!
                    if (prob > ZERO_LOG_PROB_WEIGHT) {
                        //If there will be no back-off found then we already have it set to zero
                        TLogProbBackOff back_off = ZERO_BACK_OFF_WEIGHT;

                        //Try getting the back-off weight.
                        //1. If the context length is one go on: we can get smth
                        //even if the 1-Gram consists of just an unknown word.
                        //2. If the context length is more then one and there is
                        //an unknown word in the gram then it makes no sense to do
                        //searching as there are no M-grams with <unk> in them
                        if ((level == M_GRAM_LEVEL_1) ||
                                ((level > M_GRAM_LEVEL_1) && has_no_unk_words<true>(level)
                                && is_bitmap_hash_cache<true>(level))) {
                            (void) get_back_off_weight(level, back_off);
                        } else {
                            LOG_DEBUG << "Could try to back off but it will not be "
                                    << "successful due to the present unk words! Thus "
                                    << "using the zero back-off weight right away!" << END_LOG;
                        }

                        LOG_DEBUG1 << "The " << level << " probability = " << back_off
                                << " + " << prob << " = " << (back_off + prob) << END_LOG;

                        //Do the back-off weight plus the lower level probability,
                        //we do a plus as we work with LOG probabilities
                        prob += back_off;
                    }
                }

                /**
                 * This function should implement the computation of the
                 * N-Gram probabilities in the Back-Off Language Model. The
                 * N-Gram hashes can be obtained from the _wordHashes member
                 * variable of the class.
                 * @param level the M-gram level for which the probability is to be computed
                 * @param prob [out] the reference to the probability to be found/computed
                 */
                virtual void get_prob_value(const TModelLevel level, TLogProbBackOff & prob) = 0;

                /**
                 * This function allows to get the back-off weight for the current context.
                 * The N-Gram hashes can be obtained from the pre-computed data member array
                 * _wordHashes.
                 * @param level the M-gram level for which the back-off weight is to be found,
                 * is equal to the context length of the K-Gram in the caller function
                 * @param back_off [out] the back-off weight to be computed
                 * @return the resulting back-off weight probability
                 */
                virtual bool get_back_off_weight(const TModelLevel level, TLogProbBackOff & back_off) = 0;

                /**
                 * This function allows to retrieve the probability stored for the given M-gram level.
                 * If the value is found then it must be added to the prob parameter of the function.
                 * If the value is not found then the prob parameter of the function must not be changed.
                 * @param level the level of the M-gram we need to compute probability for.
                 * @param prob the probability variable that is to be increased with the found probability weight
                 * @return true if the probability for the given M-gram level could be found, otherwise false.
                 */
                virtual bool add_prob_value(const TModelLevel level, TLogProbBackOff & prob) {
                    throw Exception("Implement: bool add_prob_value(const TModelLevel level, TLogProbBackOff & prob)");
                };

                /**
                 * This function allows to retrieve the back-off stored for the given M-gram level.
                 * If the value is found then it must be added to the prob parameter of the function.
                 * If the value is not found then the prob parameter of the function must not be changed.
                 * In that case the back-off weight is just zero.
                 * @param level the level of the M-gram we need to compute probability for.
                 * @param prob the probability variable that is to be increased with the found back-off weight
                 */
                virtual void add_back_off_weight(const TModelLevel level, TLogProbBackOff & prob) {
                    throw Exception("Implement: bool add_back_off_weight(const TModelLevel level, TLogProbBackOff & prob)");
                };

            private:

                //Stores the unknown word masks for the probability computations,
                //up to and including 8-grams:
                // 00000000, 00000001, 00000011, 00000111, 00001111,
                // 00011111, 00111111, 01111111, 11111111
                static const uint8_t PROB_UNK_MASKS[];
                //Stores the unknown word masks for the back-off weight computations,
                //up to and including 8-grams:
                // 00000000, 00000010, 00000110, 00001110,
                // 00011110, 00111110, 01111110, 11111110
                static const uint8_t BACK_OFF_UNK_MASKS[];

                //Unknown word bits
                uint8_t m_unk_word_flags;

                //Stores the bitmap hash caches per M-gram level
                BitmapHashCache m_bitmap_hash_cach[NUM_M_N_GRAM_LEVELS];

                /**
                 * Has to be called after the method that stores the query word ids.
                 * This one looks at the query word ids and creates binary flag map
                 * of the unknown word ids present in the current M-gram
                 */
                inline void store_unk_word_flags() {
                    const TModelLevel max_supp_level = (sizeof (PROB_UNK_MASKS) - 1);

                    if (DO_SANITY_CHECKS && (N > max_supp_level)) {
                        stringstream msg;
                        msg << "store_unk_word_flags: Unsupported m-gram level: "
                                << SSTR(N) << ", must be <= " << SSTR(max_supp_level)
                                << "], insufficient m_unk_word_flags capacity!";
                        throw Exception(msg.str());
                    }

                    //Re-initialize the flags with zero
                    m_unk_word_flags = 0;
                    //Fill in the values from the currently considered M-gram word ids
                    for (size_t idx = (N - m_query_ptr->level); idx < N; ++idx) {
                        if (m_tmp_word_ids[idx] == AWordIndex::UNKNOWN_WORD_ID) {
                            m_unk_word_flags |= (1u << ((N - 1) - idx));
                        }
                    }

                    LOG_DEBUG << "The query unknown word flags are: "
                            << bitset<NUM_BITS_IN_UINT_8>(m_unk_word_flags) << END_LOG;
                }

                /**
                 * Allows to check if the given sub-m-gram contains an unknown word
                 * @param level of the considered M-gram
                 * @return true if the unknown word is present, otherwise false
                 */
                template<bool is_back_off>
                inline bool is_bitmap_hash_cache(const TModelLevel level) {
                    if (m_is_birmap_hash_cache) {
                        const BitmapHashCache & ref = m_bitmap_hash_cach[level - MGRAM_IDX_OFFSET];
                        return ref.is_m_gram<is_back_off>(this->m_query_ptr, level);
                    } else {
                        return true;
                    }
                }

                /**
                 * Allows to check if the given sub-m-gram contains an unknown word
                 * @param level of the considered M-gram
                 * @return true if the unknown word is present, otherwise false
                 */
                template<bool is_back_off>
                inline bool has_no_unk_words(const TModelLevel level) {
                    uint8_t level_flags;

                    if (is_back_off) {
                        level_flags = (m_unk_word_flags & BACK_OFF_UNK_MASKS[level]);

                        LOG_DEBUG << "The back-off level: " << level << " unknown word flags are: "
                                << bitset<NUM_BITS_IN_UINT_8>(level_flags) << END_LOG;

                        return (level_flags == 0);
                    } else {
                        level_flags = (m_unk_word_flags & PROB_UNK_MASKS[level]);

                        LOG_DEBUG << "The probability level: " << level << " unknown word flags are: "
                                << bitset<NUM_BITS_IN_UINT_8>(level_flags) << END_LOG;

                        return (level_flags == 0);
                    }
                }
            };

            template<TModelLevel N>
            const uint8_t ATrie<N>::PROB_UNK_MASKS[] = {
                0x00, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0xFF
            };

            template<TModelLevel N>
            const uint8_t ATrie<N>::BACK_OFF_UNK_MASKS[] = {
                0x00, 0x02, 0x06, 0x0E, 0x1E, 0x3E, 0x7E, 0xFE
            };
        }
    }
}

#endif	/* ATRIE_HPP */

