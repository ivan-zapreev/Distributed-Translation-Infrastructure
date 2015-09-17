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

#include "Globals.hpp"
#include "Logger.hpp"
#include "Exceptions.hpp"

#include "MathUtils.hpp"

#include "AWordIndex.hpp"
#include "BasicWordIndex.hpp"

#include "ArrayUtils.hpp"
#include "TextPieceReader.hpp"

using namespace std;
using namespace uva::utils::math;
using namespace uva::smt::file;
using namespace uva::smt::exceptions;
using namespace uva::smt::tries;
using namespace uva::smt::utils::array;

namespace uva {
    namespace smt {
        namespace tries {
            namespace dictionary {

                namespace __OptimizingWordIndex {

                    /**
                     * This structure is to store the word index data, the word itself and its index
                     */
                    struct WordIndexBucketEntry {

                        WordIndexBucketEntry() : m_word(NULL), m_len(0), m_word_id(0) {
                        }
                        char * m_word;
                        uint8_t m_len;
                        TShortId m_word_id;
                    } __attribute__((packed));
                }

                /**
                 * This class is to be used as an optimizer wrapped around the original index.
                 * The main idea is that a word index is provided to this class and is used
                 * for initial data gathering. After that is done, during the post actions
                 * the data from the original word index is taken and converted into optimized
                 * format. This data is then stored within this class. The original word index
                 * is then destroyed to save space.
                 */
                class OptimizingWordIndex : public AWordIndex {
                public:

                    /**
                     * This is the main constructor to be used. It accepts ther disposable word index.
                     * Which will be destroyed by this class at any needed moment, so no one else must
                     * have a reference or a pointer to the argument object
                     * @param disp_word_index_ptr the disposable word index
                     */
                    OptimizingWordIndex(BasicWordIndex * disp_word_index_ptr)
                    : m_disp_word_index_ptr(disp_word_index_ptr), m_num_words(0),
                    m_num_buckets(0), m_num_bucket_maps(0),
                    m_word_hash_buckets(NULL), m_word_entries(NULL) {
                        if (disp_word_index_ptr == NULL) {
                            throw Exception("OptimizingWordIndex::OptimizingWordIndex: a NULL pointer for the disposable word index!");
                        }
                    }

                    /**
                     * This method should be used to pre-allocate the word index
                     * @param num_words the number of words
                     */
                    virtual void reserve(const size_t num_words) {
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
                    virtual size_t get_number_of_words(const size_t num_words) {
                        if (m_disp_word_index_ptr != NULL) {
                            //If the disposable is still present then return its data
                            return m_disp_word_index_ptr->get_number_of_words(num_words);
                        } else {
                            //The number of words is now typically needed only at stage of data preallocation
                            //At that moment the disposable word index must still be present.
                            throw Exception("OptimizingWordIndex::get_number_of_words: Un-timed request!");
                        }
                    };

#define IS_EQUAL(token, entry) (((token).getLen() == (entry).m_len) && (strncmp((token).getBeginCStr(), (entry).m_word, (entry).m_len) == 0))

                    /**
                     * This function gets an id for the given word word based no the stored 1-Grams.
                     * @see AWordIndex
                     */
                    virtual bool get_word_id(const TextPieceReader & token, TShortId &wordId) const {
                        LOG_DEBUG2 << "Searching for: '" << token.str() << "'" << END_LOG;

                        const uint32_t bucket_idx = get_bucket_idx(token);
                        const uint32_t begin_idx = m_word_hash_buckets[bucket_idx];
                        const uint32_t end_idx = m_word_hash_buckets[bucket_idx + 1];

                        if (begin_idx != end_idx) {
                            if (IS_EQUAL(token, m_word_entries[begin_idx])) {
                                //Found in the first entry!
                                wordId = m_word_entries[begin_idx].m_word_id;
                                return true;
                            } else {
                                //Could not find in the first entry so do some linear search
                                for (uint_fast32_t idx = begin_idx + 1; idx != end_idx; ++idx) {
                                    if (IS_EQUAL(token, m_word_entries[idx])) {
                                        wordId = m_word_entries[idx].m_word_id;
                                        return true;
                                    }
                                }
                            }
                        }

                        wordId = UNKNOWN_WORD_ID;

                        return false;
                    };

                    /**
                     * This function creates/gets an id for the given word.
                     * @see AWordIndex
                     */
                    virtual TShortId register_word(const TextPieceReader & token) {
                        return m_disp_word_index_ptr->register_word(token);
                    };

                    /**
                     * This method allows to indicate whether word counting is
                     * needed by the given implementation of the word index.
                     * @see AWordIndex
                     */
                    virtual bool is_word_counts_needed() const {
                        return m_disp_word_index_ptr->is_word_counts_needed();
                    };

                    /**
                     * This method is to be used when the word counting is needed.
                     * @see AWordIndex
                     */
                    virtual void count_word(const TextPieceReader & token) {
                        m_disp_word_index_ptr->count_word(token);
                    };

                    /**
                     * Should be called if the word count is needed
                     * after all the words have been counted.
                     * @see AWordIndex
                     */
                    virtual void do_post_word_count() {
                        m_disp_word_index_ptr->do_post_word_count();
                    };

                    /**
                     * Indicates if the post-actions are needed. The post actions
                     * should be called after all the words have been filled into
                     * the index.
                     * @see AWordIndex
                     */
                    virtual bool is_post_actions_needed() const {
                        return true || m_disp_word_index_ptr->is_post_actions_needed();
                    };

                    /**
                     * Is to be called if the post actions are needed right after
                     * that all the individual words have beed added into the index.
                     * @see AWordIndex
                     */
                    virtual void do_post_actions() {
                        //Perform the post actions if needed, before starting further actions.
                        if (m_disp_word_index_ptr->is_post_actions_needed()) {
                            m_disp_word_index_ptr->do_post_actions();
                        }

                        //Do the conversions from the disposable word index into internal data

                        //Allocate the data storages
                        allocate_data_storage();

                        //Count the number of elements per bucket
                        count_elements_per_bucket();

                        //Convert the number of elements per bucket into word index
                        build_word_hash_buckets();

                        //Fill in the buckets with data
                        fill_buckets_with_data();

                        //Dispose the unneeded disposable word index
                        dispose_disp_word_index();
                    };

                    /**
                     * The basic destructor
                     */
                    virtual ~OptimizingWordIndex() {
                        //Delete the buckets
                        if (m_word_hash_buckets != NULL) {
                            delete[] m_word_hash_buckets;
                        }

                        //Delete words
                        for (size_t idx = 0; idx < m_num_words; ++idx) {
                            if (m_word_entries[idx].m_word != NULL) {
                                delete[] m_word_entries[idx].m_word;
                            }
                        }

                        //Delete bucket entries
                        if (m_word_entries != NULL) {
                            delete[] m_word_entries;
                        }

                        //Dispose the disposable word index if this is not done yet
                        dispose_disp_word_index();
                    };

                private:
                    //Stores the disposable word index pointer up until it is not needed any more
                    BasicWordIndex * m_disp_word_index_ptr;

                    //Stores the number of words
                    size_t m_num_words;

                    //Stores the number of mappings
                    size_t m_num_buckets;

                    //Stores the number of bucket mappings: the number of buckets + 1
                    size_t m_num_bucket_maps;

                    //Stores the word hash mapping from the hash to the begin position in the data array
                    uint32_t * m_word_hash_buckets;

                    //Typedef the bucket entry
                    typedef __OptimizingWordIndex::WordIndexBucketEntry TBucketEntry;

                    //Stores the buckets data
                    TBucketEntry * m_word_entries;

                    /**
                     * Allocate the data storages
                     */
                    inline void allocate_data_storage() {
                        //First determine the number of buckets to be used
                        m_num_buckets = (__OptimizingWordIndex::BUCKETS_FACTOR * m_num_words);

                        //Make it even to facilitate the divisions (?)
                        m_num_buckets += (is_odd_A(m_num_buckets) ? 1 : 0);

                        //Now allocate the number of elements in the hash mappings
                        //Make it +1 in order to store the end position of the last bucket
                        m_num_bucket_maps = m_num_buckets + 1;
                        m_word_hash_buckets = new uint32_t[m_num_bucket_maps];
                        fill_n(m_word_hash_buckets, m_num_bucket_maps, 0);

                        //Allocate the buckets themselves
                        m_word_entries = new TBucketEntry[m_num_words];

                        LOG_DEBUG2 << "Allocated " << m_num_buckets << " buckets for "
                                << m_num_words << " words" << END_LOG;
                    };

                    /**
                     * Allows to compute the bucket index for the given token
                     * @param token the token to compute the bucket id for
                     * @return the bucket id
                     */
                    inline uint32_t get_bucket_idx(const TextPieceReader & token) const {
                        return computeBoundedHash(token.getBeginCStr(), token.getLen(), m_num_buckets);
                    }

                    /**
                     * Allows to compute the bucket index for the given token
                     * @param token the token to compute the bucket id for
                     * @return the bucket id
                     */
                    inline uint32_t get_bucket_idx(const string & token) const {
                        return computeBoundedHash(token, m_num_buckets);
                    }

                    /**
                     * Count the number of elements per bucket
                     */
                    inline void count_elements_per_bucket() {
                        BasicWordIndex::TWordIndexMapConstIter curr = m_disp_word_index_ptr->begin();
                        const BasicWordIndex::TWordIndexMapConstIter end = m_disp_word_index_ptr->end();
                        uint32_t idx = 0;

                        //Go through all the words and count the number of elements per bucket
                        while (curr != end) {
                            const BasicWordIndex::TWordIndexEntry & entry = *curr;
                            idx = get_bucket_idx(entry.first);
                            //Here we miss use the mappings array to store counts
                            m_word_hash_buckets[idx] += 1;
                            curr++;
                        }
                    };

                    /**
                     * Convert the number of elements per bucket into word index
                     */
                    inline void build_word_hash_buckets() {
                        uint32_t curr_idx = 0;
                        uint32_t next_idx = 0;

                        //Go through the buckets array and initialize the begin indexes
                        for (size_t idx = 0; idx < m_num_bucket_maps; ++idx) {
                            if (DO_SANITY_CHECKS && m_word_hash_buckets[idx] > 2) {
                                LOG_WARNING << "A bucket with " << m_word_hash_buckets[idx] << " words is detected!" << END_LOG;
                            }

                            next_idx = curr_idx + m_word_hash_buckets[idx];
                            m_word_hash_buckets[idx] = curr_idx;
                            curr_idx = next_idx;
                        }
                    };

                    /**
                     * Fill in the buckets with data
                     */
                    inline void fill_buckets_with_data() {
                        LOG_DEBUG2 << "Start filling the buckets with data" << END_LOG;

                        BasicWordIndex::TWordIndexMapConstIter curr = m_disp_word_index_ptr->begin();
                        const BasicWordIndex::TWordIndexMapConstIter end = m_disp_word_index_ptr->end();
                        uint32_t bucket_idx = 0;
                        uint32_t entry_idx = 0;

                        //Go through all the words and fill in the buckets
                        while (curr != end) {
                            const BasicWordIndex::TWordIndexEntry & word_data = *curr;

                            LOG_DEBUG2 << "Converting word: '" << word_data.first
                                    << "' with word id: " << word_data.second << END_LOG;

                            bucket_idx = get_bucket_idx(word_data.first);

                            LOG_DEBUG2 << "Got bucket_idx: " << bucket_idx << END_LOG;

                            //Get the bucket begin index
                            entry_idx = m_word_hash_buckets[bucket_idx];

                            LOG_DEBUG2 << "Got bucket begin entry_idx: " << entry_idx << END_LOG;

                            //Search for the first empty entry
                            while (m_word_entries[entry_idx].m_word != NULL) {
                                entry_idx++;

                                if (DO_SANITY_CHECKS && (entry_idx >= m_num_words)) {
                                    stringstream msg;
                                    msg << "Exceeded the allowed entry index: " << (m_num_words - 1)
                                            << " in bucket: " << bucket_idx;
                                    throw Exception(msg.str());
                                }

                                LOG_DEBUG2 << "Skipping on to entry_idx: " << entry_idx << END_LOG;
                            }

                            LOG_DEBUG2 << "Adding word: '" << word_data.first
                                    << "' to entry position: " << entry_idx << END_LOG;

                            //Copy the data into the bucket
                            TBucketEntry & entry = m_word_entries[entry_idx];

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
            }
        }
    }
}


#endif	/* OPTIMIZINGWORDINDEX_HPP */

