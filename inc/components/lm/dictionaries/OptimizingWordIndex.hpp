/* 
 * File:   OptimizingWordIndex.hpp
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
 * Created on September 17, 2015, 12:16 PM
 */

#ifndef OPTIMIZINGWORDINDEX_HPP
#define	OPTIMIZINGWORDINDEX_HPP

#include <string>   // std::string
#include <cstring>  // std::strncpy std::strncmp

#include "components/lm/TrieConstants.hpp"
#include "components/logging/Logger.hpp"
#include "utils/Exceptions.hpp"

#include "utils/MathUtils.hpp"

#include "components/lm/dictionaries/AWordIndex.hpp"
#include "components/lm/dictionaries/BasicWordIndex.hpp"

#include "utils/containers/ArrayUtils.hpp"
#include "utils/file/TextPieceReader.hpp"

using namespace std;
using namespace uva::utils::math;
using namespace uva::utils::file;
using namespace uva::utils::exceptions;
using namespace uva::smt::tries;
using namespace uva::utils::containers::utils;
using namespace uva::utils::hashing;

namespace uva {
    namespace smt {
        namespace tries {
            namespace dictionary {

                namespace __OptimizingWordIndex {

                    /**
                     * This structure is to store the word index data, the word itself and its index
                     */
                    template<typename TWordIdType>
                    struct WordIndexBucketEntry {

                        WordIndexBucketEntry() : m_word(NULL), m_len(0), m_word_id(0) {
                        }
                        char * m_word;
                        uint8_t m_len;
                        TWordIdType m_word_id;
                    } __attribute__((packed));
                }

                /**
                 * This class is to be used as an optimizer wrapped around the original index.
                 * The main idea is that a word index is provided to this class and is used
                 * for initial data gathering. After that is done, during the post actions
                 * the data from the original word index is taken and converted into optimized
                 * format. This data is then stored within this class. The original word index
                 * is then destroyed to save space.
                 * @param SubWordIndexType the sub WordIndex type to be used
                 */
                template<typename SubWordIndexType>
                class OptimizingWordIndex : public AWordIndex<TShortId> {
                public:

                    /**
                     * This is the main constructor to be used. It accepts ther disposable word index.
                     * Which will be destroyed by this class at any needed moment, so no one else must
                     * have a reference or a pointer to the argument object
                     * @param memory_factor the memory factor for the SubWordIndexType constructor
                     */
                    OptimizingWordIndex(const float memory_factor)
                    : m_num_words(0), m_num_buckets(0), m_word_buckets(NULL) {
                        m_disp_word_index_ptr = new SubWordIndexType(memory_factor);
                        ASSERT_SANITY_THROW(!m_disp_word_index_ptr->is_word_registering_needed(),
                                "This word index requires a sub-word index with word registration!");
                    }

                    /**
                     * This method should be used to pre-allocate the word index
                     * @param num_words the number of words
                     */
                    inline void reserve(const size_t num_words) {
                        LOG_DEBUG2 << "Reserving " << num_words << " words!" << END_LOG;

                        if (m_disp_word_index_ptr != NULL) {
                            //If the disposable is still present then return its data
                            m_disp_word_index_ptr->reserve(num_words);
                        } else {
                            //The reserve is called at stage of data preallocation
                            //At that moment the disposable word index must still be present.
                            throw Exception("OptimizingWordIndex::reserve: Un-timed request!");
                        }

                        //Store the number of words as indicated by the disposable word index
                        //There we also take into account unknown and undefined words
                        m_num_words = m_disp_word_index_ptr->get_number_of_words(num_words);

                        //We will reserve internal memory after the the disposable
                        //word index if filled in with all the existing words.
                    };

                    /**
                     * Allows to get the total words count including the unknown and undefined words
                     * @see AWordIndex
                     */
                    inline size_t get_number_of_words(const size_t num_words) const {
                        if (m_disp_word_index_ptr != NULL) {
                            //If the disposable is still present then return its data
                            return m_disp_word_index_ptr->get_number_of_words(num_words);
                        } else {
                            //The number of words is now typically needed only at stage of data preallocation
                            //At that moment the disposable word index must still be present.
                            throw Exception("OptimizingWordIndex::get_number_of_words: Un-timed request!");
                        }
                    };

#define IS_EQUAL(token, entry) (((token).length() == (entry).m_len) && (strncmp((token).get_begin_c_str(), (entry).m_word, (entry).m_len) == 0))

                    /**
                     * This function gets an id for the given word word based no the stored 1-Grams.
                     * @see AWordIndex
                     */
                    inline TWordIdType get_word_id(const TextPieceReader & token) const {
                        //Compute the bucket id
                        uint_fast64_t bucket_idx = get_bucket_idx(token);

                        LOG_DEBUG3 << "Searching for: " << token.str() << ", the " <<
                                "initial bucket index is: " << bucket_idx << END_LOG;

                        //Search until an empty bucket or the word is found
                        while (m_word_buckets[bucket_idx].m_word != NULL) {
                            //Search within the bucket
                            if (IS_EQUAL(token, m_word_buckets[bucket_idx])) {
                                return m_word_buckets[bucket_idx].m_word_id;
                            }
                            get_next_bucket_idx(bucket_idx);
                        }
                        LOG_DEBUG3 << "Encountered an empty bucket, the word is unknown!" << END_LOG;

                        return UNKNOWN_WORD_ID;
                    };

                    /**
                     * This method allows to indicate whether registering a word is
                     * needed by the given implementation of the word index.
                     * @see AWordIndex
                     */
                    inline bool is_word_registering_needed() const {
                        return true;
                    };

                    /**
                     * This function creates/gets an id for the given word.
                     * @see AWordIndex
                     */
                    inline TWordIdType register_word(const TextPieceReader & token) {
                        return m_disp_word_index_ptr->register_word(token);
                    };

                    /**
                     * This method allows to indicate whether word counting is
                     * needed by the given implementation of the word index.
                     * @see AWordIndex
                     */
                    inline bool is_word_counts_needed() const {
                        return m_disp_word_index_ptr->is_word_counts_needed();
                    };

                    /**
                     * This method is to be used when the word counting is needed.
                     * @see AWordIndex
                     */
                    inline void count_word(const TextPieceReader & word, TLogProbBackOff prob) {
                        m_disp_word_index_ptr->count_word(word, prob);
                    };

                    /**
                     * Should be called if the word count is needed
                     * after all the words have been counted.
                     * @see AWordIndex
                     */
                    inline void do_post_word_count() {
                        m_disp_word_index_ptr->do_post_word_count();
                    };

                    /**
                     * Indicates if the post-actions are needed. The post actions
                     * should be called after all the words have been filled into
                     * the index.
                     * @see AWordIndex
                     */
                    inline bool is_post_actions_needed() const {
                        return true || m_disp_word_index_ptr->is_post_actions_needed();
                    };

                    /**
                     * Is to be called if the post actions are needed right after
                     * that all the individual words have beed added into the index.
                     * @see AWordIndex
                     */
                    inline void do_post_actions() {
                        //Perform the post actions if needed, before starting further actions.
                        if (m_disp_word_index_ptr->is_post_actions_needed()) {
                            m_disp_word_index_ptr->do_post_actions();
                        }

                        //Do the conversions from the disposable word index into internal data

                        //Allocate the data storages
                        allocate_data_storage();

                        //Fill in the buckets with data
                        fill_buckets_with_data();

                        //Dispose the unneeded disposable word index
                        dispose_disp_word_index();
                    };

                    /**
                     * Allows to indicate if the word index is continuous, i.e.
                     * it issues the word ids in a continuous range starting from 0.
                     * @see AWordIndex
                     * @return true - this word index is continuous.
                     */
                    static constexpr inline bool is_word_index_continuous() {
                        return true;
                    }

                    /**
                     * The basic destructor
                     */
                    virtual ~OptimizingWordIndex() {
                        //Delete bucket entries
                        if (m_word_buckets != NULL) {
                            //Delete words
                            for (size_t idx = 0; idx < m_num_buckets; ++idx) {
                                if ((m_word_buckets[idx].m_len != 0) && (m_word_buckets[idx].m_word != NULL)) {
                                    delete[] m_word_buckets[idx].m_word;
                                }
                            }
                            //Delete entries
                            delete[] m_word_buckets;
                        }

                        //Dispose the disposable word index if this is not done yet
                        dispose_disp_word_index();
                    };

                private:
                    //Stores the disposable word index pointer up until it is not needed any more
                    SubWordIndexType * m_disp_word_index_ptr;

                    //Stores the number of words
                    size_t m_num_words;

                    //Stores the number of mappings
                    size_t m_num_buckets;

                    //Stores the number of buckets divider
                    uint_fast64_t m_capacity;

                    //Typedef the bucket entry
                    typedef __OptimizingWordIndex::WordIndexBucketEntry<TWordIdType> TBucketEntry;

                    //Stores the buckets data
                    TBucketEntry * m_word_buckets;

                    /**
                     * Sets the number of buckets as a power of two, based on the number of elements
                     * @param buckets_factor the buckets factor that the number of elements will be
                     * multiplied with before converting it into the number of buckets.
                     * @param num_elems the number of elements to compute the buckets for
                     */
                    inline void set_number_of_elements(const double buckets_factor, const size_t num_elems) {
                        //Do a compulsory assert on the buckets factor
                        ASSERT_CONDITION_THROW((buckets_factor < 1.0), string("buckets_factor: ") +
                                std::to_string(buckets_factor) + string(", must be >= 1.0"));

                        //Compute the number of buckets
                        m_num_buckets = const_expr::power(2, const_expr::ceil(const_expr::log2(buckets_factor * (num_elems + 1))));
                        //Compute the buckets divider
                        m_capacity = m_num_buckets - 1;

                        ASSERT_CONDITION_THROW((num_elems > m_capacity), string("Insufficient buckets capacity: ") +
                                std::to_string(m_capacity) + string(" need at least ") + std::to_string(num_elems));

                        LOG_DEBUG << "OWI: num_elems: " << num_elems << ", m_num_buckets: " << m_num_buckets
                                << ", m_capacity: " << m_capacity << END_LOG;
                    }

                    /**
                     * Allocate the data storages
                     */
                    inline void allocate_data_storage() {
                        //Set the number of buckets and the capacity
                        set_number_of_elements(__OptimizingWordIndex::BUCKETS_FACTOR, m_num_words);

                        LOG_DEBUG << "m_num_words: " << m_num_words << ", m_num_buckets: " << m_num_buckets << END_LOG;

                        //Allocate the buckets themselves
                        m_word_buckets = new TBucketEntry[m_num_buckets];

                        LOG_DEBUG << "Allocated " << m_num_buckets << " buckets for "
                                << m_num_words << " words" << END_LOG;
                    };

                    /**
                     * Allows to compute the bucket index for the given token
                     * @param token the token to compute the bucket id for
                     * @return the bucket id
                     */
                    inline uint_fast64_t get_bucket_idx(const TextPieceReader & token) const {
                        return compute_hash(token.get_begin_c_str(), token.length()) & m_capacity;
                    }

                    /**
                     * Allows to compute the bucket index for the given token
                     * @param token the token to compute the bucket id for
                     * @return the bucket id
                     */
                    inline uint_fast64_t get_bucket_idx(const string & token) const {
                        return compute_hash(token) & m_capacity;
                    }

                    /**
                     * Provides the next bucket index
                     * @param bucket_idx [in/out] the bucket index
                     */
                    inline void get_next_bucket_idx(uint_fast64_t & bucket_idx) const {
                        bucket_idx = (bucket_idx + 1) & m_capacity;
                        LOG_DEBUG3 << "Moving on to the next bucket: " << bucket_idx << END_LOG;
                    }

                    /**
                     * Fill in the buckets with data
                     */
                    inline void fill_buckets_with_data() {
                        LOG_DEBUG2 << "Start filling the buckets with data" << END_LOG;

                        BasicWordIndex::TWordIndexMapConstIter curr = m_disp_word_index_ptr->begin();
                        const BasicWordIndex::TWordIndexMapConstIter end = m_disp_word_index_ptr->end();
                        uint_fast64_t bucket_idx = 0;

                        //Go through all the words and fill in the buckets
                        while (curr != end) {
                            const BasicWordIndex::TWordIndexEntry & word_data = *curr;

                            LOG_DEBUG2 << "Converting word: '" << word_data.first
                                    << "' with word id: " << word_data.second << END_LOG;

                            bucket_idx = get_bucket_idx(word_data.first);

                            LOG_DEBUG2 << "Got bucket_idx: " << bucket_idx << END_LOG;

                            //Search for the first empty bucket
                            while (m_word_buckets[bucket_idx].m_word != NULL) {
                                LOG_DEBUG2 << "The bucket: " << bucket_idx <<
                                        " is full, skipping to the next." << END_LOG;
                                get_next_bucket_idx(bucket_idx);
                            }

                            LOG_DEBUG2 << "Adding word: '" << word_data.first
                                    << "' to bucket: " << bucket_idx << END_LOG;

                            //Copy the data into the bucket
                            TBucketEntry & entry = m_word_buckets[bucket_idx];

                            //Copy the string but without the terminating null character
                            entry.m_len = word_data.first.length();
                            entry.m_word = new char[entry.m_len];
                            strncpy(entry.m_word, word_data.first.c_str(), entry.m_len);

                            //Copy the word id
                            entry.m_word_id = word_data.second;

                            ++curr;
                        }
                    };

                    /**
                     * Allows to dispose the disposable word index, if present
                     */
                    inline void dispose_disp_word_index() {
                        if (m_disp_word_index_ptr != NULL) {
                            delete m_disp_word_index_ptr;
                            m_disp_word_index_ptr = NULL;
                        }
                    };
                };

                typedef OptimizingWordIndex<BasicWordIndex> TOptBasicWordIndex;
                typedef OptimizingWordIndex<CountingWordIndex> TOptCountWordIndex;
            }
        }
    }
}


#endif	/* OPTIMIZINGWORDINDEX_HPP */

