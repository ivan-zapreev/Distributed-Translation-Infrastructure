/* 
 * File:   rm_configs.hpp
 * Author: zapreevis
 *
 * Created on February 10, 2016, 4:23 PM
 */

#ifndef RM_CONFIGS_HPP
#define RM_CONFIGS_HPP

#include <inttypes.h>
#include <string>

using namespace std;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace rm {
                    namespace models {

                        namespace __unk_phrase {
                            //Stores the unknown phrase string, should be configurable
                            static const string UNKNOWN_PHRASE_STR = string("UNK");
                        }

                        //Store the number of of weights in the reordering model entry
                        //The number of weights is a multiple of 2 
                        static constexpr uint8_t NUMBER_WEIGHT_ENTRIES = 6;

                        //Stores the configuration parameters for the basic reordering model implementation
                        namespace __rm_basic_model {
                            //Influences the number of buckets that will be created for the basic model implementations
                            static constexpr double SOURCES_BUCKETS_FACTOR = 3.0;
                        }
                    }
                }
            }
        }
    }
}

#endif /* RM_CONFIGS_HPP */

