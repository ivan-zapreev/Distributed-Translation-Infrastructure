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

                    //Define the feature weights delimiter string for the config file
                    static const string RM_FEATURE_WEIGHTS_DELIMITER_STR = "|";
                    
                    //Stores the various numbers of RM features
                    static const size_t SIX_RM_FEATURES = 6;
                    static const size_t EIGHT_RM_FEATURES = 8;
                    //Stores the maximum number of the reordering model features
                    static const size_t MAX_NUM_RM_FEATURES = EIGHT_RM_FEATURES;

                    namespace models {

                        namespace __unk_phrase {
                            //Stores the unknown source phrase string, should be configurable
                            static const string RM_UNKNOWN_SOURCE_STR = string("UNK");
                            //Stores the unknown target phrase string, should be configurable
                            static const string RM_UNKNOWN_TARGET_STR = string("UNK");
                        }

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

