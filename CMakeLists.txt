cmake_minimum_required (VERSION 2.8.12.2)
project (Basic-Phrase-Based-Decoding)

#Bring the headers into the project
include_directories(inc SYSTEM ext)

#Add the required flags
set(CMAKE_CXX_FLAGS "-pipe -std=c++0x -Wall -m64")

#Add the performance profiling flag to Debug mode
set(CMAKE_CXX_FLAGS_DEBUG "-pg")

###############################DEFINE THE LM QUERY###################################

#Bring the source files into the project
set(LM_QUERY_SOURCES 
    src/server/lm/lm_query.cpp
    src/server/lm/lm_configurator.cpp
    src/server/lm/models/m_gram_query.cpp
    src/server/lm/models/w2c_hybrid_trie.cpp
    src/server/lm/models/w2c_array_trie.cpp
    src/server/lm/models/h2d_map_trie.cpp
    src/server/lm/models/g2d_map_trie.cpp
    src/server/lm/models/c2w_array_trie.cpp
    src/server/lm/models/c2d_map_trie.cpp
    src/server/lm/models/c2d_hybrid_trie.cpp
    src/server/lm/mgrams/query_m_gram.cpp
    src/server/lm/mgrams/model_m_gram.cpp
    src/server/lm/mgrams/byte_m_gram_id.cpp
    src/server/lm/builders/lm_basic_builder.cpp
    src/server/lm/builders/lm_gram_builder.cpp
    src/common/utils/monitor/statistics_monitor.cpp 
    src/common/utils/logging/logger.cpp
)
#Define the server executable
add_executable(lm-query ${LM_QUERY_SOURCES})

###############################DEFINE THE SERVER EXECUTABLE##########################

#Manually add the source files if needed
set(BPBD_SERVER_SOURCES src/server/bpbd_server.cpp
    src/common/utils/logging/logger.cpp
    src/server/decoder/de_configurator.cpp
    src/server/lm/lm_configurator.cpp
    src/server/tm/tm_configurator.cpp
    src/server/tm/models/tm_target_entry.cpp
    src/server/rm/rm_configurator.cpp
    src/server/lm/models/m_gram_query.cpp
    src/server/lm/models/w2c_hybrid_trie.cpp
    src/server/lm/models/w2c_array_trie.cpp
    src/server/lm/models/h2d_map_trie.cpp
    src/server/lm/models/g2d_map_trie.cpp
    src/server/lm/models/c2w_array_trie.cpp
    src/server/lm/models/c2d_map_trie.cpp
    src/server/lm/models/c2d_hybrid_trie.cpp
    src/server/lm/mgrams/query_m_gram.cpp
    src/server/lm/mgrams/model_m_gram.cpp
    src/server/lm/mgrams/byte_m_gram_id.cpp
    src/server/lm/builders/lm_basic_builder.cpp
    src/server/lm/builders/lm_gram_builder.cpp
    src/common/utils/monitor/statistics_monitor.cpp 
    src/common/utils/logging/logger.cpp
    src/common/messaging/trans_job_code.cpp
    src/server/trans_task_pool_worker.cpp
    src/server/trans_task_pool.cpp
)
#Define the server executable
add_executable(bpbd-server ${BPBD_SERVER_SOURCES})

##############################DEFINE THE CLIENT EXECUTABLE###########################

#Manually add the source files if needed
set(BPBD_CLIENT_SOURCES
    src/client/bpbd_client.cpp
    src/common/utils/logging/logger.cpp
    src/common/messaging/trans_job_code.cpp
    src/client/trans_job_status.cpp
)
#Define the server executable
add_executable(bpbd-client ${BPBD_CLIENT_SOURCES})

##############################ADD THE NEEDED LIBRARIES###############################

#In case we are on linux add linking with the rt library
if(UNIX AND NOT APPLE)
    target_link_libraries(lm-query rt)
    target_link_libraries(bpbd-server rt pthread)
    target_link_libraries(bpbd-client rt pthread)
endif()

#####################################################################################3
#Bring the source files into the project
#file(GLOB LM_QUERY_SOURCES "src/*.cpp")
