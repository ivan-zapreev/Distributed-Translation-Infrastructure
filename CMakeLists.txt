cmake_minimum_required (VERSION 2.8.12.2)
project (Basic-Phrase-Based-Decoding)

#Bring the headers into the project
include_directories(inc ext)

#Add the required flags
set(CMAKE_CXX_FLAGS "-std=c++0x -Wall -m64 -Wno-unused-local-typedef")

###############################DEFINE THE LM QUERY###################################

#Bring the source files into the project
set(LM_QUERY_SOURCES 
    src/server/lm/lm_query.cpp
    src/server/lm/tries/W2CHybridTrie.cpp
    src/server/lm/tries/W2CArrayTrie.cpp
    src/server/lm/tries/H2DMapTrie.cpp
    src/server/lm/tries/G2DMapTrie.cpp
    src/server/lm/tries/C2WArrayTrie.cpp
    src/server/lm/tries/C2DMapTrie.cpp
    src/server/lm/tries/C2DHybridTrie.cpp
    src/server/lm/ngrams/ByteMGramId.cpp
    src/server/lm/dictionaries/AWordIndex.cpp
    src/server/lm/builders/ARPATrieBuilder.cpp
    src/server/lm/builders/ARPAGramBuilder.cpp
    src/common/utils/monitore/statistics_monitor.cpp 
    src/common/utils/logging/logger.cpp
    src/common/utils/xxhash.c
)
#Define the server executable
add_executable(lm-query ${LM_QUERY_SOURCES})

###############################DEFINE THE SERVER EXECUTABLE##########################

#Manually add the source files if needed
set(BPBD_SERVER_SOURCES src/server/bpbd_server.cpp
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
