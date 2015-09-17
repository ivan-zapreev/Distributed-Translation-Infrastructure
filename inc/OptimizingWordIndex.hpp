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

#include "Globals.hpp"
#include "Logger.hpp"
#include "Exceptions.hpp"

#include "AWordIndex.hpp"
#include "BasicWordIndex.hpp"

#include "ArrayUtils.hpp"
#include "TextPieceReader.hpp"

using namespace std;
using namespace uva::smt::file;
using namespace uva::smt::exceptions;
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
                        char m_word[];
                        TShortId word_id;
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
                    m_num_buckets(0), m_word_hash_mappings(NULL), m_buckets(NULL) {
                        if (disp_word_index_ptr == NULL) {
                            throw Exception("OptimizingWordIndex::OptimizingWordIndex: a NULL pointer for the disposable word index!");
                        }
                    }

                    /**
                     * This method should be used to pre-allocate the word index
                     * @param num_words the number of words
                     */
                    virtual void reserve(const size_t num_words) {
                        if (m_disp_word_index_ptr != NULL) {
                            //If the disposable is still present then return its data
                            return m_disp_word_index_ptr->reserve(num_words);
                        } else {
                            //The reserve is called at stage of data preallocation
                            //At that moment the disposable word index must still be present.
                            throw Exception("OptimizingWordIndex::reserve: Un-timed request!");
                        }
                        //Store the number of words
                        m_num_words = num_words;

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
                            return m_disp_word_index_ptr->get_number_of_words();
                        } else {
                            //The number of words is now typically needed only at stage of data preallocation
                            //At that moment the disposable word index must still be present.
                            throw Exception("OptimizingWordIndex::get_number_of_words: Un-timed request!");
                        }
                    };

                    /**
                     * This function gets an id for the given word word based no the stored 1-Grams.
                     * @see AWordIndex
                     */
                    virtual bool get_word_id(const TextPieceReader & token, TShortId &wordId) {
                        //ToDo: Implement
                    };

                    /**
                     * This function creates/gets an id for the given word.
                     * @see AWordIndex
                     */
                    virtual TShortId register_word(const TextPieceReader & token) {
                        m_disp_word_index_ptr->register_word(token);
                    };

                    /**
                     * This method allows to indicate whether word counting is
                     * needed by the given implementation of the word index.
                     * @see AWordIndex
                     */
                    virtual bool is_word_counts_needed() {
                        return m_disp_word_index_ptr->need_word_counts();
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
                    virtual void post_word_count() {
                        m_disp_word_index_ptr->do_post_word_count();
                    };

                    /**
                     * Indicates if the post-actions are needed. The post actions
                     * should be called after all the words have been filled into
                     * the index.
                     * @see AWordIndex
                     */
                    virtual bool is_post_actions_needed() {
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
                        count_elements_pre_bucket();

                        //Convert the number of elements per bucket into word index
                        build_word_hash_mappings();

                        //Fill in the buckets with data
                        fill_buckets_with_data();

                        //Dispose the unneeded disposable word index
                        dispose_disp_word_index();
                    };

                    /**
                     * The basic destructor
                     */
                    virtual ~OptimizingWordIndex() {
                        if (m_word_hash_mappings != NULL) {
                            delete[] m_word_hash_mappings;
                        }

                        if (m_buckets != NULL) {
                            delete[] m_buckets;
                        }

                        dispose_disp_word_index();
                    };

                private:
                    //Stores the disposable word index pointer up until it is not needed any more
                    BasicWordIndex * m_disp_word_index_ptr;

                    //Stores the number of words
                    size_t m_num_words;

                    //Stores the number of word buckets
                    size_t m_num_buckets;

                    //Stores the word hash mapping from the hash to the begin position in the data array
                    uint32_t m_word_hash_mappings[];

                    //Typedef the bucket entry
                    typedef __OptimizingWordIndex::WordIndexBucketEntry TBucketEntry;

                    //Stores the buckets data
                    TBucketEntry m_buckets[];

                    /**
                     * Allocate the data storages
                     */
                    void allocate_data_storage() {
                        //First determine the number of buckets to be used
                        m_num_buckets = (__OptimizingWordIndex::BUCKETS_FACTOR * m_num_words);

                        //Now allocate the number of elements in the hash mappings
                        //Make it +1 in order to store the end position of the last bucket
                        m_word_hash_mappings = new uint32_t[m_num_buckets + 1];

                        //Allocate the buckets themselves
                        m_buckets = new TBucketEntry[m_num_buckets];
                    };
                    
                    uint32_t compute_hash(const string & token) {
                        return computeHash()
                    }

                    /**
                     * Count the number of elements per bucket
                     */
                    void count_elements_pre_bucket() {
                        
                        BasicWordIndex::TWordIndexMapConstIter curr = m_disp_word_index_ptr->begin();
                        const BasicWordIndex::TWordIndexMapConstIter end = m_disp_word_index_ptr->end();
                        while(curr != end){
                            const TWordIndexEntry & entry = *curr;
                            entry.first;
                            curr++;
                        }
                        
                    };

                    /**
                     * Convert the number of elements per bucket into word index
                     */
                    void build_word_hash_mappings() {
                    };

                    /**
                     * Fill in the buckets with data
                     */
                    void fill_buckets_with_data() {
                    };

                    /**
                     * Allows to dispose the disposable word index, if present
                     */
                    void dispose_disp_word_index() {
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

