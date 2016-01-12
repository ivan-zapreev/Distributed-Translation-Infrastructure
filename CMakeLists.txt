cmake_minimum_required (VERSION 2.8.12.2)
project (Basic-Phrase-Based-Decoding)

#Bring the headers into the project
include_directories(inc ext)

#Manually add the source files if needed
#set(BPBD_SERVER_SOURCES src/XXX.cpp src/YYY.cpp)

#Bring the source files into the project
file(GLOB BPBD_SERVER_SOURCES "src/*.cpp")

#Add the required flags
set(CMAKE_CXX_FLAGS "-std=c++0x -Wall -m64")

#Define the server executable
add_executable(bpbd-server ${BPBD_SERVER_SOURCES})

#In case we are on linux add linking with the rt library
if(UNIX AND NOT APPLE)
    target_link_libraries(bpbd-server rt)
endif()
