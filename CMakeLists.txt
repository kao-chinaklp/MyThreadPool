cmake_minimum_required(VERSION 3.23)

# set the project name
project(MyThreadPool)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(EXPORT_COMPILE_COMMANDS ON)

# add the executable
file(GLOB SOURCES "src/*.cpp")
add_library(MyThreadPool SHARED ${SOURCES})

set(CMAKE_AR gcc-ar)

target_include_directories(MyThreadPool PRIVATE ${CMAKE_SOURCE_DIR}/header)

# Main configurations
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(MyThreadPool PRIVATE DEBUG)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g3")
else()
    target_compile_definitions(MyThreadPool PRIVATE NDEBUG)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2 -s -flto")
endif()