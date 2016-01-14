cmake_minimum_required (VERSION 2.8.12.2)
project (Basic-Phrase-Based-Decoding)

#Bring the headers into the project
include_directories(inc ext)

#Add the required flags
set(CMAKE_CXX_FLAGS "-std=c++0x -Wall -m64")

###############################DEFINE THE LM QUERY###################################

#Bring the source files into the project
set(LM_QUERY_SOURCES 
    src/components/lm/lm_query.cpp
    src/components/lm/W2CHybridTrie.cpp
    src/components/lm/W2CArrayTrie.cpp
    src/components/lm/H2DMapTrie.cpp
    src/components/lm/G2DMapTrie.cpp
    src/components/lm/C2WArrayTrie.cpp
    src/components/lm/C2DMapTrie.cpp
    src/components/lm/C2DHybridTrie.cpp
    src/components/lm/ByteMGramId.cpp
    src/components/lm/AWordIndex.cpp
    src/components/lm/ARPATrieBuilder.cpp
    src/components/lm/ARPAGramBuilder.cpp
    src/components/logging/Logger.cpp
    src/components/statistics/StatisticsMonitor.cpp 
    src/components/utils/xxhash.c)
#Define the server executable
add_executable(lm-query ${LM_QUERY_SOURCES})

###############################DEFINE THE SERVER EXECUTABLE##########################

#Manually add the source files if needed
set(BPBD_SERVER_SOURCES src/server/bpbd_server.cpp)
#Define the server executable
add_executable(bpbd-server ${BPBD_SERVER_SOURCES})

##############################DEFINE THE CLIENT EXECUTABLE###########################

#Manually add the source files if needed
set(BPBD_CLIENT_SOURCES src/client/bpbd_client.cpp)
#Define the server executable
add_executable(bpbd-client ${BPBD_CLIENT_SOURCES})

##############################ADD THE NEEDED LIBRARIES###############################

#In case we are on linux add linking with the rt library
if(UNIX AND NOT APPLE)
    target_link_libraries(lm-query rt)
    target_link_libraries(bpbd-server rt)
    target_link_libraries(bpbd-client rt pthread)
endif()

#####################################################################################3
#Bring the source files into the project
#file(GLOB LM_QUERY_SOURCES "src/*.cpp")
