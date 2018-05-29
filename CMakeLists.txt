cmake_minimum_required (VERSION 2.8.12.2)
project (Basic-Phrase-Based-Decoding)

set(MIN_GCC_VERSION "4.9.1")
if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS ${MIN_GCC_VERSION})
    message(FATAL_ERROR "Insufficient gcc version " ${CMAKE_CXX_COMPILER_VERSION} " minimum required is " ${MIN_GCC_VERSION})
endif()

#If SSL/TLS support is requested
if(DEFINED WITH_TLS)
    if(${WITH_TLS})
        message(STATUS "TLS support is ENABLED")
        find_package(OpenSSL REQUIRED)

        message(STATUS "OpenSSL include dir: '${OPENSSL_INCLUDE_DIR}'")
        message(STATUS "OpenSSL libraries: '${OPENSSL_LIBRARIES}'")
        if(OPENSSL_FOUND AND (EXISTS ${OPENSSL_INCLUDE_DIR}))
            include_directories(${OPENSSL_INCLUDE_DIR})
            add_compile_options(-DWITH_TLS=true)
        else()
            message(FATAL_ERROR "Could NOT find OpenSSL, try to set the path to OpenSSL root folder in the system variable OPENSSL_ROOT_DIR (missing: OPENSSL_LIBRARIES or OPENSSL_INCLUDE_DIR)")
        endif()
    else()
        message(STATUS "TLS support is DISABLED")
    endif()
else()
    message(STATUS "TLS support is DISABLED")
endif()

#Bring the headers into the project
include_directories(inc ${SYSTEM} ext)

#Add the required flags
add_compile_options(-pipe)
add_compile_options(-Wall)
add_compile_options(-m64)
add_compile_options(-std=c++11)
add_compile_options(-Wno-unknown-pragmas)

#Add the performance profiling flag to Debug mode
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-pg)
endif()

###############################DEFINE THE LM QUERY###################################

#Bring the source files into the project
set(LM_QUERY_SOURCES 
    src/server/lm/lm_query.cpp
    src/server/lm/lm_parameters.cpp
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
)
#Define the server executable
add_executable(lm-query ${LM_QUERY_SOURCES})

###############################DEFINE THE SERVER EXECUTABLE##########################

#Manually add the source files if needed
set(BPBD_SERVER_SOURCES
    src/server/server_parameters.cpp
    src/server/decoder/de_parameters.cpp
    src/server/decoder/de_configurator.cpp
    src/server/lm/lm_parameters.cpp
    src/server/tm/tm_parameters.cpp
    src/server/rm/rm_parameters.cpp
    src/server/rm/models/rm_entry.cpp
    src/server/lm/lm_configurator.cpp
    src/server/tm/tm_configurator.cpp
    src/server/rm/rm_configurator.cpp
    src/server/tm/models/tm_target_entry.cpp
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
    src/common/messaging/websocket/websocket_server_params.cpp
    src/common/messaging/trans_job_code.cpp
    src/common/messaging/messaging.cpp
    src/server/messaging/messaging.cpp
    src/server/trans_task.cpp
    src/server/bpbd_server.cpp
)
#Define the server executable
add_executable(bpbd-server ${BPBD_SERVER_SOURCES})

########################DEFINE THE LOAD BALANCER EXECUTABLE##########################

#Bring the source files into the project
set(LOAD_BALANCER_SOURCES 
    src/common/messaging/websocket/websocket_server_params.cpp
    src/common/messaging/websocket/websocket_client_params.cpp
    src/common/messaging/language_registry.cpp
    src/common/messaging/messaging.cpp
    src/client/messaging/messaging.cpp
    src/balancer/balancer_parameters.cpp
    src/balancer/translator_adapter.cpp
    src/balancer/balancer_job.cpp
    src/balancer/bpbd_balancer.cpp
)
#Define the server executable
add_executable(bpbd-balancer ${LOAD_BALANCER_SOURCES})

########################DEFINE THE TEXT PROCESSOR EXECUTABLE##########################

#Bring the source files into the project
set(TEXT_PROCESSOR_SOURCES
    src/common/messaging/websocket/websocket_server_params.cpp
    src/common/messaging/language_registry.cpp
    src/common/messaging/messaging.cpp
    src/processor/processor_parameters.cpp
    src/processor/processor_job.cpp
    src/processor/bpbd_processor.cpp
)
#Define the server executable
add_executable(bpbd-processor ${TEXT_PROCESSOR_SOURCES})

##############################DEFINE THE CLIENT EXECUTABLE###########################

#Manually add the source files if needed
set(BPBD_CLIENT_SOURCES
    src/client/client_parameters.cpp
    src/common/messaging/websocket/websocket_client_params.cpp
    src/client/bpbd_client.cpp
    src/common/messaging/trans_job_code.cpp
    src/common/messaging/messaging.cpp
    src/client/messaging/messaging.cpp
    src/client/trans_job_status.cpp
)
#Define the server executable
add_executable(bpbd-client ${BPBD_CLIENT_SOURCES})

##############################ADD THE NEEDED LIBRARIES###############################

#If SSL/TLS is requested then add it
if(DEFINED WITH_TLS)
    if(${WITH_TLS})
        target_link_libraries(bpbd-client ${OPENSSL_LIBRARIES})
        target_link_libraries(bpbd-server ${OPENSSL_LIBRARIES})
        target_link_libraries(bpbd-balancer ${OPENSSL_LIBRARIES})
        target_link_libraries(bpbd-processor ${OPENSSL_LIBRARIES})
    endif()
endif()

#In case we are on linux add linking with the rt library
if(UNIX AND NOT APPLE)
    target_link_libraries(lm-query rt)
    target_link_libraries(bpbd-client rt pthread dl)
    target_link_libraries(bpbd-server rt pthread dl)
    target_link_libraries(bpbd-balancer rt pthread dl)
    target_link_libraries(bpbd-processor rt pthread dl)
endif()

#####################################################################################3
#Bring the source files into the project
#file(GLOB LM_QUERY_SOURCES "src/*.cpp")
