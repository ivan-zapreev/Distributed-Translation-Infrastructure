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

#include "server/server_configs.hpp"

using namespace std;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace rm {
                    namespace models {
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

