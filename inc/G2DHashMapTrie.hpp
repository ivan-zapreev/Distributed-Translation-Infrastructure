/* 
 * File:   G2DHashMapTrie.hpp
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
 * Created on September 3, 2015, 3:32 PM
 */

#ifndef G2DHASHMAPTRIE_HPP
#define	G2DHASHMAPTRIE_HPP

#include <string>       // std::string
#include <functional>   // std::function

#include "Globals.hpp"
#include "Exceptions.hpp"

#include "AWordIndex.hpp"
#include "ModelMGram.hpp"
#include "ByteMGramId.hpp"

#include "TextPieceReader.hpp"

#include "ArrayUtils.hpp"
#include "DynamicMemoryArrays.hpp"

#include "GenericTrieBase.hpp"

using namespace std;

using namespace uva::smt::logging;
using namespace uva::smt::file;
using namespace uva::smt::tries::alloc;
using namespace uva::smt::tries::dictionary;
using namespace uva::smt::tries::m_grams;
using namespace uva::smt::tries::__G2DMapTrie;

namespace uva {
    namespace smt {
        namespace tries {

            namespace __G2DMapTrie {

                /**
                 * This template structure is used for storing trie hash map elements
                 * Each element contains and id of the m-gram and its payload -
                 * the probability/back-off data, the latter is the template parameter
                 * @param id stores the M-gram id
                 * @param payload stores the payload which is either probability or probability with back-off
                 */
                template<typename M_GRAM_ID_TYPE, typename PAYLOAD_TYPE>
                struct S_M_GramData {
                    M_GRAM_ID_TYPE id;
                    PAYLOAD_TYPE payload;

                    //Stores the memory increase strategy object
                    const static MemIncreaseStrategy m_mem_strat;

                    typedef M_GRAM_ID_TYPE TMGramIdType;
                    typedef PAYLOAD_TYPE TPayloadType;
                };

                typedef S_M_GramData<T_Gram_Id_Data_Ptr, T_M_Gram_Payload> T_M_Gram_PB_Entry;
                typedef S_M_GramData<T_Gram_Id_Data_Ptr, TLogProbBackOff> T_M_Gram_Prob_Entry;

                template<typename ELEMENT_TYPE>
                void destroy_Comp_M_Gram_Id(ELEMENT_TYPE & elem) {
                    m_gram_id::destroy(elem.id);
                };

                template void destroy_Comp_M_Gram_Id<T_M_Gram_PB_Entry>(T_M_Gram_PB_Entry &elem);
                template void destroy_Comp_M_Gram_Id<T_M_Gram_Prob_Entry>(T_M_Gram_Prob_Entry &elem);

                template<typename M_GRAM_ID_TYPE, typename PAYLOAD_TYPE>
                const MemIncreaseStrategy S_M_GramData<M_GRAM_ID_TYPE, PAYLOAD_TYPE>::m_mem_strat =
                get_mem_incr_strat(__G2DMapTrie::MEM_INC_TYPE,
                        __G2DMapTrie::MIN_MEM_INC_NUM, __G2DMapTrie::MEM_INC_FACTOR);
            }

            /**
             * This is a Gram to Data trie that is implemented as a HashMap.
             * @param MAX_LEVEL - the maximum level of the considered N-gram, i.e. the N value
             */
            template<TModelLevel MAX_LEVEL, typename WordIndexType>
            class G2DMapTrie : public GenericTrieBase<MAX_LEVEL, WordIndexType, __G2DMapTrie::DO_BITMAP_HASH_CACHE> {
            public:
                typedef GenericTrieBase<MAX_LEVEL, WordIndexType, __G2DMapTrie::DO_BITMAP_HASH_CACHE> BASE;
                typedef Byte_M_Gram_Id<typename WordIndexType::TWordIdType> TM_Gram_Id;

                //The typedef for the retrieving function
                typedef function<uint32_t(const G2DMapTrie&, const uint64_t gram_hash) > TGetBucketIdFunct;

                /**
                 * The basic constructor
                 * @param _wordIndex the word index to be used
                 */
                explicit G2DMapTrie(WordIndexType & word_index);

                /**
                 * Allows to log the information about the instantiated trie type
                 */
                inline void log_trie_type_usage_info() const {
                    LOG_USAGE << "Using the <" << __FILE__ << "> model." << END_LOG;
                    LOG_INFO << "Using the #buckets divider: "
                            << SSTR(__G2DMapTrie::WORDS_PER_BUCKET_FACTOR) << END_LOG;
                    LOG_INFO << "Using  and the " << T_M_Gram_PB_Entry::m_mem_strat.get_strategy_info()
                            << " memory allocation strategy." << END_LOG;
                }

                /**
                 * This method can be used to provide the N-gram count information
                 * That should allow for pre-allocation of the memory
                 * @see GenericTrieBase
                 */
                virtual void pre_allocate(const size_t counts[MAX_LEVEL]);

                /**
                 * This method adds a M-Gram (word) to the trie where 1 < M < N
                 * @see GenericTrieBase
                 */
                template<TModelLevel CURR_LEVEL>
                inline void add_m_gram(const T_Model_M_Gram<WordIndexType> & gram) {
                    //Register the m-gram in the hash cache
                    this->template register_m_gram_cache<CURR_LEVEL>(gram);

                    if (CURR_LEVEL == M_GRAM_LEVEL_1) {
                        //Get the word id of this unigram, so there is just one word in it and its the end one
                        const TShortId word_id = gram.get_end_word_id();
                        //Store the probability data in the one gram data storage, under its id
                        m_1_gram_data[word_id] = gram.m_payload;
                    } else {
                        //Get the bucket index
                        LOG_DEBUG << "Getting the bucket id for the m-gram: " << (string) gram << END_LOG;
                        TShortId bucket_idx = get_bucket_id(gram.get_hash(), m_num_buckets[CURR_LEVEL - 1]);

                        if (CURR_LEVEL == MAX_LEVEL) {
                            //Create a new M-Gram data entry
                            T_M_Gram_Prob_Entry & data = m_N_gram_data[bucket_idx].allocate();
                            //Create the N-gram id from the word ids
                            gram.template create_m_gram_id<CURR_LEVEL>(data.id);
                            //Set the probability data
                            data.payload = gram.m_payload.prob;
                        } else {
                            //Compute the M-gram level index
                            constexpr TModelLevel LEVEL_IDX = (CURR_LEVEL - BASE::MGRAM_IDX_OFFSET);
                            //Create a new M-Gram data entry
                            T_M_Gram_PB_Entry & data = m_M_gram_data[LEVEL_IDX][bucket_idx].allocate();
                            //Create the M-gram id from the word ids.
                            gram.template create_m_gram_id<CURR_LEVEL>(data.id);
                            //Set the probability and back-off data
                            data.payload = gram.m_payload;
                        }
                    }
                }

                /**
                 * This method allows to get the payloads and compute the (cumulative) m-gram probabilities.
                 * @see GenericTrieBase
                 */
                template<bool DO_CUMULATIVE_PROBS>
                inline void execute(const T_Query_M_Gram<WordIndexType> & query, const void * payloads[MAX_LEVEL][MAX_LEVEL], TLogProbBackOff probs[MAX_LEVEL]) const {
                    //Declare the stream-compute result status variable
                    MGramStatusEnum status = MGramStatusEnum::UNDEFINED_MGS;

                    //Initialize the begin and end index variables
                    TModelLevel begin_word_idx = query.get_begin_word_idx();
                    //Check if we need cumulative or single conditional m-gram probability
                    TModelLevel end_word_idx = (DO_CUMULATIVE_PROBS) ? begin_word_idx : query.get_end_word_idx();

                    //Do the iterations until the status is successful, the return is done within the loop
                    while (true) {
                        LOG_DEBUG << "-----> Streaming cumulative sub-m-gram [" << SSTR(begin_word_idx)
                                << ", " << SSTR(end_word_idx) << "]" << END_LOG;

                        //Stream the probability computations
                        stream_right(query, payloads[begin_word_idx], probs, begin_word_idx, end_word_idx, status);
                        LOG_DEBUG << "The result for the sub-m-gram: [" << SSTR(begin_word_idx) << ","
                                << SSTR(end_word_idx) << "] is : " << status_to_string(status) << END_LOG;

                        //Check the resulting status and take actions if needed
                        switch (status) {
                            case MGramStatusEnum::BAD_END_WORD_UNKNOWN_MGS:
                                //The end word is not known back-off down and then do diagonal, if there is columns left
                                stream_down_unknown(query, payloads, probs, begin_word_idx, end_word_idx);
                                //If this was the last column, we are done and can just return
                                if (end_word_idx == query.get_end_word_idx()) {
                                    LOG_DEBUG << "The computations are done as streaming down was done for the last column!" << END_LOG;
                                    return;
                                }
                                //If this was not the last column then we need to go diagonal
                                move_diagonal(query, payloads[begin_word_idx], probs, begin_word_idx, end_word_idx);
                                break;
                            case MGramStatusEnum::BAD_NO_PAYLOAD_MGS:
                                //The payload of the m-gram defined by the current values of begin_word_idx, end_word_idx
                                //could not be found in the trie, therefore we need to back-off and then keep streaming.
                                back_off_and_step_down(query, payloads[begin_word_idx], probs, begin_word_idx, end_word_idx);
                                break;
                            case MGramStatusEnum::GOOD_PRESENT_MGS:
                                //The m-gram probabilities have been computed, we can return
                                return;
                            default:
                                THROW_EXCEPTION(string("Unsupported status: ").append(std::to_string(status)));
                        }
                    }
                };

                /**
                 * This method allows to check if post processing should be called after
                 * all the X level grams are read. This method is virtual.
                 * For more details @see WordIndexTrieBase
                 */
                template<TModelLevel CURR_LEVEL>
                bool is_post_grams() const {
                    //Check the base class and we need to do post actions
                    //for all the M-grams with 1 < M <= N. The M-grams level
                    //data has to be ordered per bucket per id, see
                    //post_M_Grams, and post_N_Grams methods below.

                    return (CURR_LEVEL > M_GRAM_LEVEL_1) || BASE::template is_post_grams<CURR_LEVEL>();
                }

                /**
                 * This method should be called after all the X level grams are read.
                 * For more details @see WordIndexTrieBase
                 */
                template<TModelLevel CURR_LEVEL>
                inline void post_grams() {
                    //Call the base class method first
                    if (BASE::template is_post_grams<CURR_LEVEL>()) {
                        BASE::template post_grams<CURR_LEVEL>();
                    }

                    //Do the post actions here
                    if (CURR_LEVEL == MAX_LEVEL) {
                        //Sort the level's data
                        post_M_N_Grams<TProbBucket, MAX_LEVEL>(m_N_gram_data);
                    } else {
                        if (CURR_LEVEL > M_GRAM_LEVEL_1) {
                            //Compute the M-gram level index
                            constexpr TModelLevel CURR_LEVEL_IDX = (CURR_LEVEL - BASE::MGRAM_IDX_OFFSET);

                            //Sort the level's data
                            post_M_N_Grams<TProbBackOffBucket, CURR_LEVEL>(m_M_gram_data[CURR_LEVEL_IDX]);
                        }
                    }
                };

                /**
                 * The basic class destructor
                 */
                virtual ~G2DMapTrie();

            private:
                //Stores the 1-gram data
                T_M_Gram_Payload * m_1_gram_data;

                //These are arrays of buckets for M-Gram levels with 1 < M < N
                typedef ADynamicStackArray<T_M_Gram_PB_Entry, uint8_t, &__G2DMapTrie::destroy_Comp_M_Gram_Id<T_M_Gram_PB_Entry> > TProbBackOffBucket;
                TProbBackOffBucket * m_M_gram_data[BASE::NUM_M_GRAM_LEVELS];

                //This is an array of buckets for the N-Gram level
                typedef ADynamicStackArray<T_M_Gram_Prob_Entry, uint8_t, &__G2DMapTrie::destroy_Comp_M_Gram_Id<T_M_Gram_Prob_Entry> > TProbBucket;
                TProbBucket * m_N_gram_data;

                //Stores the number of gram ids/buckets per level
                TShortId m_num_buckets[MAX_LEVEL];

                //The typedef for the function that gets the payload from the trie
                typedef std::function<void (const TShortId num_buckets[MAX_LEVEL], const T_Query_M_Gram<WordIndexType> & gram, const TProbBucket * buckets,
                        const void ** payload_ptr_ptr, MGramStatusEnum &status) > TGetPayloadProbFunc;

                //Stores the get-payload function pointers for getting probabilities
                static const TGetPayloadProbFunc m_get_prob[M_GRAM_LEVEL_6][M_GRAM_LEVEL_7];

                //The typedef for the function that gets the payload from the trie
                typedef std::function<void (const TShortId num_buckets[MAX_LEVEL], const T_Query_M_Gram<WordIndexType> & gram, const TProbBackOffBucket * buckets,
                        const void ** payload_ptr_ptr, MGramStatusEnum &status) > TGetPayloadProbBackFunc;

                //Stores the get-payload function pointers for getting complete payloads 
                static const TGetPayloadProbBackFunc m_get_prob_back[M_GRAM_LEVEL_6][M_GRAM_LEVEL_7];

                /**
                 * This method allows to stream down the sub-m-gram matrix colum for the case when the end
                 * word is unknown.
                 * @param gram the m-gram query to be executed
                 * @param payloads all the available payloads
                 * @param probs the array of probabilities for each sub-m-gram
                 * @param begin_word_idx[out] the begin word index for the sub-m-gram
                 * @param end_word_idx the end word index for the sub-m-gram
                 */
                inline void stream_down_unknown(const T_Query_M_Gram<WordIndexType> & gram, const void * payloads[MAX_LEVEL][MAX_LEVEL],
                        TLogProbBackOff probs[MAX_LEVEL], TModelLevel & begin_word_idx, const TModelLevel end_word_idx) const {
                    LOG_DEBUG << "Streaming down, from : [" << SSTR(begin_word_idx) << "," << SSTR(end_word_idx) << "]" << END_LOG;

                    //Iterate down the sub-m-gram matrix column
                    while (begin_word_idx < end_word_idx) {
                        LOG_DEBUG << "Going to get back-off payload for sub-m-gram: [" << SSTR(begin_word_idx)
                                << "," << SSTR(end_word_idx) << "]" << END_LOG;
                        //Just do a simple back-off that will also increment begin_word_idx
                        //We could have done here a bit better, as we know that on the first iteration
                        //the payload for the back-off sub-m-gram is already retrieved and on the others
                        //it is definitely not retrieved yet, but this is a minor optimization, I guess
                        back_off_and_step_down(gram, payloads[begin_word_idx], probs, begin_word_idx, end_word_idx);
                    }
                    LOG_DEBUG << "Done with streaming down, need to account for : [" << SSTR(begin_word_idx)
                            << "," << SSTR(end_word_idx) << "]" << END_LOG;
                    //Once we are done with the loop above we need to retrieve and account for the unknown word uni-gram
                    payloads[begin_word_idx][end_word_idx] = reinterpret_cast<void *> (&m_1_gram_data[WordIndexType::UNKNOWN_WORD_ID]);
                    LOG_DEBUG << "probs[" << SSTR(end_word_idx) << "] += " << m_1_gram_data[WordIndexType::UNKNOWN_WORD_ID].prob << END_LOG;
                    probs[end_word_idx] += m_1_gram_data[WordIndexType::UNKNOWN_WORD_ID].prob;
                }

                /**
                 * This method allows to perform the diagonal move in the sub-m-gram matrix in
                 * case the end word of the sub-m-gram is unknown and this is not the last column. 
                 * @param gram the m-gram query to be executed
                 * @param payloads the payload for the given m-gram row
                 * @param probs the array of probabilities for each sub-m-gram
                 * @param begin_word_idx[out] the begin word index for the sub-m-gram
                 * @param end_word_idx[out] the end word index for the sub-m-gram
                 */
                inline void move_diagonal(const T_Query_M_Gram<WordIndexType> & gram, const void * payloads[MAX_LEVEL],
                        TLogProbBackOff probs[MAX_LEVEL], TModelLevel & begin_word_idx, TModelLevel & end_word_idx) const {
                    //At this moment we are down to the unknown word uni-gram payloads[end_word_idx]
                    //This is not the last column, so we can move at least one step further, yet we need to move to
                    //the next row as m-grams in this row contain the unk word and thus are definitely not available 

                    LOG_DEBUG << "Adding the back-off from sub-m-gram : [" << SSTR(begin_word_idx) << ","
                            << SSTR(end_word_idx) << "] to prob[" << SSTR(end_word_idx + 1) << "]!" << END_LOG;

                    //Add the back-off weight of the unknown word stored in the payloads to the next sub-m-gram probability
                    probs[end_word_idx + 1] += reinterpret_cast<const T_M_Gram_Payload *> (payloads[end_word_idx])->back;
                    LOG_DEBUG << "probs[" << SSTR(end_word_idx + 1) << "] += "
                            << reinterpret_cast<const T_M_Gram_Payload *> (payloads[end_word_idx])->back << END_LOG;

                    //Move diagonal, i.e. to the next uni-gram, from there on we can start streaming again
                    begin_word_idx = ++end_word_idx;

                    LOG_DEBUG << "The next uni-gram to consider is : [" << SSTR(begin_word_idx)
                            << "," << SSTR(end_word_idx) << "]" << END_LOG;
                }

                /**
                 * This method adds the back-off weight of the given m-gram, if it is to be found in the trie
                 * @param gram the m-gram query to be executed
                 * @param payloads the payload for the given m-gram row
                 * @param probs the array of probabilities for each sub-m-gram
                 * @param begin_word_idx [out] the begin word index for the sub-m-gram, will be incremented by 1
                 * @param end_word_idx the end word index for the sub-m-gram
                 */
                inline void back_off_and_step_down(const T_Query_M_Gram<WordIndexType> & gram, const void * payloads[MAX_LEVEL],
                        TLogProbBackOff probs[MAX_LEVEL], TModelLevel & begin_word_idx, const TModelLevel end_word_idx) const {
                    //Define the end word index of the back-off sub-m-gram
                    const TModelLevel bo_end_word_idx = end_word_idx - 1;

                    //Check that the payload has not been retrieved yet
                    if (payloads[bo_end_word_idx] == NULL) {
                        LOG_DEBUG << "The payload for sub-m-gram : [" << SSTR(begin_word_idx) << ","
                                << SSTR(bo_end_word_idx) << "] is not available, needs to be retrieved!" << END_LOG;
                        //Check if the back-off sub-m-gram is potentially available
                        MGramStatusEnum status;
                        BASE::is_m_gram_potentially_present(begin_word_idx, bo_end_word_idx, gram, status);
                        LOG_DEBUG << "The payload availability status for sub-m-gram : [" << SSTR(begin_word_idx) << ","
                                << SSTR(bo_end_word_idx) << "] is: " << status_to_string(status) << END_LOG;
                        if (status == MGramStatusEnum::GOOD_PRESENT_MGS) {
                            //Try to retrieve the back-off sub-m-gram
                            if (begin_word_idx == bo_end_word_idx) {
                                //If the back-off sub-m-gram is a uni-gram then
                                get_unigram_payload(gram, payloads, begin_word_idx, bo_end_word_idx, status);
                            } else {
                                //The back-off sub-m-gram has a level M: 1 < M < N
                                get_m_gram_payload(gram, payloads, begin_word_idx, bo_end_word_idx, status);
                            }

                            //Append the back-off if the retrieval was successful
                            if (status == MGramStatusEnum::GOOD_PRESENT_MGS) {
                                LOG_DEBUG << "The m-gram is found, payload: "
                                        << (string) * reinterpret_cast<const T_M_Gram_Payload *> (payloads[bo_end_word_idx]) << END_LOG;
                                probs[end_word_idx] += reinterpret_cast<const T_M_Gram_Payload *> (payloads[bo_end_word_idx])->back;
                                LOG_DEBUG << "probs[" << SSTR(end_word_idx) << "] += "
                                        << reinterpret_cast<const T_M_Gram_Payload *> (payloads[bo_end_word_idx])->back << END_LOG;
                            }
                        }
                    } else {
                        LOG_DEBUG << "The payload for sub-m-gram : [" << SSTR(begin_word_idx)
                                << "," << SSTR(bo_end_word_idx) << "] is available and will be used" << END_LOG;
                        //Add the back-off weight
                        probs[end_word_idx] += reinterpret_cast<const T_M_Gram_Payload *> (payloads[bo_end_word_idx])->back;
                        LOG_DEBUG << "probs[" << SSTR(end_word_idx) << "] += "
                                << reinterpret_cast<const T_M_Gram_Payload *> (payloads[bo_end_word_idx])->back << END_LOG;
                    }
                    //Next we shift to the next row, it is possible as it is not a uni-gram case, the latter 
                    //always get MGramStatusEnum::GOOD_PRESENT_MGS result when their payload is retrieved
                    begin_word_idx++;
                }

                /**
                 * Allows to attempt the sub-m-gram payload retrieval for m==1
                 * @param gram the m-gram to work with
                 * @param payloads [out] the payloads array for the given row defined by begin_word_idx
                 * @param begin_word_idx the begin word index of the sub-m-gram
                 * @param end_word_idx the end word index of the sub-m-gram
                 * @param status [out] the result of the retrieval operation
                 */
                inline void get_unigram_payload(const T_Query_M_Gram<WordIndexType> & gram, const void * payloads[MAX_LEVEL],
                        const TModelLevel begin_word_idx, const TModelLevel end_word_idx, MGramStatusEnum & status) const {
                    //This is at least a uni-gram we have, therefore first process the it in a special way
                    const TShortId word_id = gram[begin_word_idx];
                    //Store the uni-gram payload pointer and add the probability to the total conditional probability 
                    payloads[begin_word_idx] = reinterpret_cast<const void *> (&m_1_gram_data[word_id]);
                    LOG_DEBUG << "Getting the uni-gram payload for word id " << SSTR(word_id) << ": " << (string) m_1_gram_data[word_id] << END_LOG;
                    //Set the status, it is always good
                    status = MGramStatusEnum::GOOD_PRESENT_MGS;
                }

                /**
                 * Allows to attempt the sub-m-gram payload retrieval for m==1
                 * @param gram the m-gram to work with
                 * @param payloads [out] the payloads array for the given row defined by begin_word_idx
                 * @param begin_word_idx the begin word index of the sub-m-gram
                 * @param end_word_idx the end word index of the sub-m-gram
                 * @param status [out] the result of the retrieval operation
                 */
                inline void get_m_gram_payload(const T_Query_M_Gram<WordIndexType> & gram, const void * payloads[MAX_LEVEL],
                        const TModelLevel begin_word_idx, const TModelLevel end_word_idx, MGramStatusEnum & status) const {
                    const TModelLevel curr_level = (end_word_idx - begin_word_idx) + 1;
                    const TModelLevel m_gram_idx = curr_level - BASE::MGRAM_IDX_OFFSET;
                    LOG_DEBUG << "Searching in " << SSTR(curr_level) << "-grams, array index: " << m_gram_idx << END_LOG;

                    //Call the templated part via function pointer
                    m_get_prob_back[begin_word_idx][end_word_idx](m_num_buckets, gram, m_M_gram_data[m_gram_idx], &payloads[end_word_idx], status);
                }

                /**
                 * Allows to attempt the sub-m-gram payload retrieval for m==1
                 * @param gram the m-gram to work with
                 * @param payloads [out] the payloads array for the given row defined by begin_word_idx
                 * @param begin_word_idx the begin word index of the sub-m-gram
                 * @param end_word_idx the end word index of the sub-m-gram
                 * @param status [out] the result of the retrieval operation
                 */
                inline void get_n_gram_payload(const T_Query_M_Gram<WordIndexType> & gram, const void * payloads[MAX_LEVEL],
                        const TModelLevel begin_word_idx, const TModelLevel end_word_idx, MGramStatusEnum & status) const {
                    LOG_DEBUG << "Searching in " << SSTR(MAX_LEVEL) << "-grams" << END_LOG;
                    //Call the templated part via function pointer
                    m_get_prob[begin_word_idx][end_word_idx](m_num_buckets, gram, m_N_gram_data, &payloads[end_word_idx], status);
                }

                /**
                 * This method does stream compute of the m-gram probabilities in one row, until it can not go further.
                 * @param gram the m-gram query to be executed
                 * @param payloads the payload for the given m-gram row
                 * @param probs the array of probabilities for each sub-m-gram
                 * @param begin_word_idx the begin word index for the sub-m-gram
                 * @param end_word_idx [out] the end word index for the sub-m-gram, will be set to the end word index at which we stopped
                 * @param status the resulting status of computations
                 */
                inline void stream_right(const T_Query_M_Gram<WordIndexType> & gram, const void * payloads[MAX_LEVEL],
                        TLogProbBackOff probs[MAX_LEVEL], const TModelLevel begin_word_idx,
                        TModelLevel & end_word_idx, MGramStatusEnum & status) const {
                    //The uni-gram case is special
                    if (begin_word_idx == end_word_idx) {
                        //Retrieve the payload
                        get_unigram_payload(gram, payloads, begin_word_idx, end_word_idx, status);
                        LOG_DEBUG << "The 1-gram is found, payload: "
                                << (string) * reinterpret_cast<const T_M_Gram_Payload *> (payloads[begin_word_idx]) << END_LOG;
                        //No need to check on the status, it is always good for the uni-gram
                        probs[begin_word_idx] += reinterpret_cast<const T_M_Gram_Payload *> (payloads[begin_word_idx])->prob;
                        LOG_DEBUG << "probs[" << SSTR(begin_word_idx) << "] += "
                                << reinterpret_cast<const T_M_Gram_Payload *> (payloads[begin_word_idx])->prob << END_LOG;

                        //Increment the end_word_idx to move on, to the next sub-m-gram
                        end_word_idx++;
                    }

                    //If this is at least a bi-gram, continue iterations, otherwise we are done!
                    for (; end_word_idx <= gram.get_end_word_idx(); ++end_word_idx) {
                        LOG_DEBUG << "Considering the sub-m-gram: [" << SSTR(begin_word_idx) << "," << SSTR(end_word_idx) << "]" << END_LOG;

                        //First check if it makes sense to look into  the trie
                        BASE::is_m_gram_potentially_present(begin_word_idx, end_word_idx, gram, status);
                        LOG_DEBUG << "The payload availability status for sub-m-gram : [" << SSTR(begin_word_idx) << ","
                                << SSTR(end_word_idx) << "] is: " << status_to_string(status) << END_LOG;
                        if (status == MGramStatusEnum::GOOD_PRESENT_MGS) {
                            //If the status says that the m-gram is potentially present then we try to retrieve it from the trie

                            //Obtain the payload, depending on the sub-m-gram level
                            if (end_word_idx == MAX_LEVEL) {
                                //We are at the last trie level, retrieve the payload
                                get_n_gram_payload(gram, payloads, begin_word_idx, end_word_idx, status);

                                //Append the probability if the retrieval was successful
                                if (status == MGramStatusEnum::GOOD_PRESENT_MGS) {
                                    LOG_DEBUG << "The n-gram is found, probability: "
                                            << *reinterpret_cast<const TLogProbBackOff *> (payloads[end_word_idx]) << END_LOG;
                                    probs[end_word_idx] += *reinterpret_cast<const TLogProbBackOff *> (payloads[end_word_idx]);
                                    LOG_DEBUG << "probs[" << SSTR(begin_word_idx) << "] += "
                                            << *reinterpret_cast<const TLogProbBackOff *> (payloads[end_word_idx]) << END_LOG;
                                } else {
                                    //The m-gram is definitely not present in the trie, so we need to stop executing this row and return
                                    return;
                                }
                            } else {
                                //We are at one of the intermediate trie level, retrieve the payload
                                get_m_gram_payload(gram, payloads, begin_word_idx, end_word_idx, status);

                                //Append the probability if the retrieval was successful
                                if (status == MGramStatusEnum::GOOD_PRESENT_MGS) {
                                    LOG_DEBUG << "The m-gram is found, payload: "
                                            << (string) * reinterpret_cast<const T_M_Gram_Payload *> (payloads[end_word_idx]) << END_LOG;
                                    probs[end_word_idx] += reinterpret_cast<const T_M_Gram_Payload *> (payloads[end_word_idx])->prob;
                                    LOG_DEBUG << "probs[" << SSTR(begin_word_idx) << "] += "
                                            << reinterpret_cast<const T_M_Gram_Payload *> (payloads[end_word_idx])->prob << END_LOG;
                                } else {
                                    //The m-gram is definitely not present in the trie, so we need to stop executing this row and return
                                    return;
                                }
                            }
                        } else {
                            //The m-gram is definitely not present in the trie, so we need to stop executing this row and return
                            return;
                        }
                    }

                    //The resulting status at this point is always a success, if we went this far then life is good :-D
                    status = MGramStatusEnum::GOOD_PRESENT_MGS;
                }

                /**
                 * Allows to get the bucket index for the given M-gram hash
                 * @param curr_level the m-gram level we need the bucked id for
                 * @param gram_hash the M-gram hash to compute the bucked index for
                 * @param num_buckets the number of buckers
                 * @param return the resulting bucket index
                 */
                static inline TShortId get_bucket_id(const uint64_t gram_hash, const TShortId num_buckets) {
                    LOG_DEBUG1 << "The m-gram hash is: " << gram_hash << END_LOG;
                    TShortId bucket_idx = gram_hash % num_buckets;
                    LOG_DEBUG1 << "The m-gram bucket_idx: " << SSTR(bucket_idx) << END_LOG;

                    //If the sanity check is on then check on that the id is within the range
                    if (DO_SANITY_CHECKS && (bucket_idx >= num_buckets)) {
                        stringstream msg;
                        msg << "The m-gram has a bad bucket index: " << SSTR(bucket_idx)
                                << ", must be within [0, " << SSTR(num_buckets) << "]";
                        throw Exception(msg.str());
                    }

                    return bucket_idx;
                }

                /**
                 * Allows to perform search in the bucket for the given M-gram id
                 * @param mgram_id_key the m-gram id to look for.
                 * @param ref the reference to the bucket
                 * @param found_idx the found index
                 * @return true if the M-gram id was found and otherwise false
                 */
                template<typename BUCKET_TYPE, TModelLevel CURR_LEVEL>
                static inline bool search_gram(T_Gram_Id_Data_Ptr mgram_id_key, const BUCKET_TYPE & ref, typename BUCKET_TYPE::TIndexType & found_idx) {
                    LOG_DEBUG2 << "# words in the bucket: " << ref.size() << END_LOG;
                    return my_bsearch_id< typename BUCKET_TYPE::TElemType,
                            typename BUCKET_TYPE::TIndexType,
                            typename BUCKET_TYPE::TElemType::TMGramIdType,
                            TM_Gram_Id::template compare<CURR_LEVEL> >
                            (ref.data(), 0, ref.size() - 1, mgram_id_key, found_idx);
                }

                /**
                 * Gets the probability for the given level M-gram, searches on specific level
                 * @param BUCKET_TYPE the level bucket type
                 * @param BEGIN_WORD_IDX the begin word index for the sub-m-gram
                 * @param END_WORD_IDX the end word index for the sub-m-gram
                 * @param query the query M-gram state 
                 * @param buckets the array of buckets to work with
                 * @param payload_ptr_ptr [out] the pointer to the payload pointer
                 * @param status [out] the resulting status of the operation
                 */
                template<typename BUCKET_TYPE, TModelLevel BEGIN_WORD_IDX, TModelLevel END_WORD_IDX>
                static inline void get_payload(const TShortId num_buckets[MAX_LEVEL], const T_Query_M_Gram<WordIndexType> & gram, const BUCKET_TYPE * buckets,
                        const void ** payload_ptr_ptr, MGramStatusEnum & status) {
                    //Compute the current level of the sub-m-gram
                    constexpr TModelLevel CURR_LEVEL = (END_WORD_IDX - BEGIN_WORD_IDX) + 1;

                    LOG_DEBUG << "Getting the bucket id for the sub-" << SSTR(CURR_LEVEL) << "-gram ["
                            << BEGIN_WORD_IDX << "," << END_WORD_IDX << "] of: " << (string) gram << END_LOG;

                    const uint32_t bucket_idx = get_bucket_id(gram.template get_hash<BEGIN_WORD_IDX, END_WORD_IDX>(), num_buckets[CURR_LEVEL - 1]);
                    LOG_DEBUG << "The " << SSTR(CURR_LEVEL) << "-gram hash bucket idx is: " << bucket_idx << END_LOG;

                    LOG_DEBUG << "Retrieving payload for a sub-" << SSTR(CURR_LEVEL) << "-gram ["
                            << SSTR(BEGIN_WORD_IDX) << ", " << SSTR(END_WORD_IDX) << "]" << END_LOG;
                    //Get the bucket to look into
                    const BUCKET_TYPE & ref = buckets[bucket_idx];

                    //1. Check that the bucket with the given index is not empty
                    if (ref.has_data()) {
                        LOG_DEBUG << "The bucket contains " << ref.size() << " elements!" << END_LOG;
                        //2. Compute the query id
                        DECLARE_STACK_GRAM_ID(TM_Gram_Id, mgram_id, CURR_LEVEL);
                        T_Gram_Id_Data_Ptr mgram_id_ptr = &mgram_id[0];

                        //Create the M-gram id from the word ids.
                        gram.template create_m_gram_id<BEGIN_WORD_IDX, END_WORD_IDX>(mgram_id_ptr);

                        //3. Search for the query id in the bucket
                        //The data is available search for the word index in the array
                        typename BUCKET_TYPE::TIndexType found_idx;
                        if (search_gram<BUCKET_TYPE, CURR_LEVEL>(mgram_id_ptr, ref, found_idx)) {
                            *payload_ptr_ptr = reinterpret_cast<const void*> (&ref[found_idx].payload);
                            status = MGramStatusEnum::GOOD_PRESENT_MGS;
                            //We are now done, the payload is found, can return!
                            return;
                        }
                    }

                    //Could not retrieve the payload for the given sub-m-gram
                    LOG_DEBUG << "Unable to find the sub-m-gram [" << SSTR(BEGIN_WORD_IDX)
                            << ", " << SSTR(END_WORD_IDX) << "] payload!" << END_LOG;
                    status = MGramStatusEnum::BAD_NO_PAYLOAD_MGS;
                }

                /**
                 * Allows to retrieve the probability and back-off weight of the unknown word
                 * @see GenericTrieBase
                 */
                inline void get_unk_word_payload(T_M_Gram_Payload & payload) const {
                    payload = m_1_gram_data[WordIndexType::UNKNOWN_WORD_ID];
                };

                /**
                 * Performs the post-processing actions on the buckets in the given M-gram level
                 * @param BUCKET_TYPE the sort of buckets we should work with
                 * @param CURR_LEVEL the M-gram level value M
                 * @param buckets the pointer to the array of buckets to process
                 */
                template<typename BUCKET_TYPE, TModelLevel CURR_LEVEL >
                void post_M_N_Grams(BUCKET_TYPE * buckets) {
                    //Iterate through all buckets and shrink/sort sub arrays
                    for (TShortId bucket_idx = 0; bucket_idx < m_num_buckets[CURR_LEVEL - 1]; ++bucket_idx) {
                        //First get the sub-array reference. 
                        BUCKET_TYPE & ref = buckets[bucket_idx];

                        LOG_DEBUG1 << "Shrinking the " << SSTR(CURR_LEVEL) << "-gram level bucket idx: " << SSTR(bucket_idx) << " ..." << END_LOG;
                        //Reduce capacity if there is unused memory
                        ref.shrink();
                        LOG_DEBUG1 << "Shrinking the " << SSTR(CURR_LEVEL) << "-gram level bucket idx: " << SSTR(bucket_idx) << " is done" << END_LOG;

                        LOG_DEBUG1 << "Sorting the " << SSTR(CURR_LEVEL) << "-gram level bucket idx: " << SSTR(bucket_idx) << " ..." << END_LOG;
                        //Order the N-gram array as it is unordered and we will binary search it later!
                        ref.sort([&] (const typename BUCKET_TYPE::TElemType & first, const typename BUCKET_TYPE::TElemType & second) -> bool {
                            LOG_DEBUG1 << "Comparing " << SSTR((void*) first.id) << " with " << SSTR((void*) second.id) << END_LOG;
                            //Update the progress bar status
                            Logger::update_progress_bar();
                                    //Return the result
                            return TM_Gram_Id::template is_less_m_grams_id<CURR_LEVEL>(first.id, second.id);
                        });
                        LOG_DEBUG1 << "Sorting the " << SSTR(CURR_LEVEL) << "-gram level bucket idx: " << SSTR(bucket_idx) << " is done" << END_LOG;
                    }
                }
            };

            template<TModelLevel MAX_LEVEL, typename WordIndexType>
            const typename G2DMapTrie<MAX_LEVEL, WordIndexType>::TGetPayloadProbFunc G2DMapTrie<MAX_LEVEL, WordIndexType>::m_get_prob[M_GRAM_LEVEL_6][M_GRAM_LEVEL_7] = {
                {NULL, &get_payload<TProbBucket, 0, 1>, &get_payload<TProbBucket, 0, 2>, &get_payload<TProbBucket, 0, 3>, &get_payload<TProbBucket, 0, 4>, &get_payload<TProbBucket, 0, 5>, &get_payload<TProbBucket, 0, 6>},
                {NULL, NULL, &get_payload<TProbBucket, 1, 2>, &get_payload<TProbBucket, 1, 3>, &get_payload<TProbBucket, 1, 4>, &get_payload<TProbBucket, 1, 5>, &get_payload<TProbBucket, 1, 6>},
                {NULL, NULL, NULL, &get_payload<TProbBucket, 2, 3>, &get_payload<TProbBucket, 2, 4>, &get_payload<TProbBucket, 2, 5>, &get_payload<TProbBucket, 2, 6>},
                {NULL, NULL, NULL, NULL, &get_payload<TProbBucket, 3, 4>, &get_payload<TProbBucket, 3, 5>, &get_payload<TProbBucket, 3, 6>},
                {NULL, NULL, NULL, NULL, NULL, &get_payload<TProbBucket, 4, 5>, &get_payload<TProbBucket, 4, 6>},
                {NULL, NULL, NULL, NULL, NULL, NULL, &get_payload<TProbBucket, 5, 6>},
            };

            template<TModelLevel MAX_LEVEL, typename WordIndexType>
            const typename G2DMapTrie<MAX_LEVEL, WordIndexType>::TGetPayloadProbBackFunc G2DMapTrie<MAX_LEVEL, WordIndexType>::m_get_prob_back[M_GRAM_LEVEL_6][M_GRAM_LEVEL_7] = {
                {NULL, &get_payload<TProbBackOffBucket, 0, 1>, &get_payload<TProbBackOffBucket, 0, 2>, &get_payload<TProbBackOffBucket, 0, 3>, &get_payload<TProbBackOffBucket, 0, 4>, &get_payload<TProbBackOffBucket, 0, 5>, &get_payload<TProbBackOffBucket, 0, 6>},
                {NULL, NULL, &get_payload<TProbBackOffBucket, 1, 2>, &get_payload<TProbBackOffBucket, 1, 3>, &get_payload<TProbBackOffBucket, 1, 4>, &get_payload<TProbBackOffBucket, 1, 5>, &get_payload<TProbBackOffBucket, 1, 6>},
                {NULL, NULL, NULL, &get_payload<TProbBackOffBucket, 2, 3>, &get_payload<TProbBackOffBucket, 2, 4>, &get_payload<TProbBackOffBucket, 2, 5>, &get_payload<TProbBackOffBucket, 2, 6>},
                {NULL, NULL, NULL, NULL, &get_payload<TProbBackOffBucket, 3, 4>, &get_payload<TProbBackOffBucket, 3, 5>, &get_payload<TProbBackOffBucket, 3, 6>},
                {NULL, NULL, NULL, NULL, NULL, &get_payload<TProbBackOffBucket, 4, 5>, &get_payload<TProbBackOffBucket, 4, 6>},
                {NULL, NULL, NULL, NULL, NULL, NULL, &get_payload<TProbBackOffBucket, 5, 6>},
            };


            typedef G2DMapTrie<M_GRAM_LEVEL_MAX, BasicWordIndex > TG2DMapTrieBasic;
            typedef G2DMapTrie<M_GRAM_LEVEL_MAX, CountingWordIndex > TG2DMapTrieCount;
            typedef G2DMapTrie<M_GRAM_LEVEL_MAX, TOptBasicWordIndex > TG2DMapTrieOptBasic;
            typedef G2DMapTrie<M_GRAM_LEVEL_MAX, TOptCountWordIndex > TG2DMapTrieOptCount;
        }
    }
}


#endif	/* G2DHASHMAPTRIE_HPP */

