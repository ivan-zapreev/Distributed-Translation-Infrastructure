/* 
 * File:   ModelMGram.hpp
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
 * Created on October 28, 2015, 10:28 AM
 */

#ifndef MODELMGRAM_HPP
#define	MODELMGRAM_HPP

#include <string>       // std::string

#include "Globals.hpp"
#include "Exceptions.hpp"

#include "BaseMGram.hpp"

#include "TextPieceReader.hpp"
#include "HashingUtils.hpp"
#include "MathUtils.hpp"

#include "ByteMGramId.hpp"
#include "BasicWordIndex.hpp"
#include "CountingWordIndex.hpp"
#include "OptimizingWordIndex.hpp"

namespace uva {
    namespace smt {
        namespace tries {
            namespace m_grams {

                /**
                 * This class is used to represent the N-Gram that will be stored into the language model.
                 */
                template<typename WordIndexType>
                class T_Model_M_Gram : public T_Base_M_Gram<WordIndexType> {
                public:

                    //Stores the m-gram probability, the log_10 probability of the N-Gram Must be a negative value
                    TLogProbBackOff m_prob;

                    //Stores the m-gram log_10 back-off weight (probability) of the N-gram can be 0 is the probability is not available
                    TLogProbBackOff m_back_off;

                    //Stores, if needed, the m-gram's context i.e. for "w1 w2 w3" -> "w1 w2"
                    TextPieceReader m_context;

                    /**
                     * If the context is set to be all the m-gram tokens, then this method
                     * allows to exclude the last token from it. It also takes care of the
                     * space in between. Note that, this method must only be called if the
                     * context is initialized and it he m-gram level m is greater than one. 
                     */
                    inline void exclude_last_token_from_context() {
                        //ToDo: Implement
                    }
                };

            }
        }
    }
}

#endif	/* MODELMGRAM_HPP */

