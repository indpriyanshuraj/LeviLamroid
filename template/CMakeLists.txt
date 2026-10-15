cmake_minimum_required(VERSION 3.22)
project(ExampleMod VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -fPIC -fvisibility=hidden -Wall -Wextra -pthread")

# Path to LeviLamroid root (defaults to parent directory when inside template/)
if(NOT DEFINED LEVILAMROID_ROOT)
    set(LEVILAMROID_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")
endif()

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${LEVILAMROID_ROOT}/include
)

link_directories(
    /system/lib64
    /data/data/com.termux/files/usr/lib
)

set(SOURCES
    src/ExampleMod.cpp
)

add_library(example_mod SHARED ${SOURCES})

target_link_libraries(example_mod
    PRIVATE
    dl
    log
)

set_target_properties(example_mod PROPERTIES
    OUTPUT_NAME "example_mod"
    PREFIX "lib"
)
