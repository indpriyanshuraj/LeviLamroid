cmake_minimum_required(VERSION 3.22)
project(LeviLamroid VERSION 1.0.3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -fPIC -fvisibility=hidden -Wall -Wextra -pthread")

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

link_directories(
    /system/lib64
    /data/data/com.termux/files/usr/lib
)

set(SOURCES
    src/LeviLamroid.cpp
    src/memory/Hook.cpp
    src/memory/PatternScan.cpp
    src/mcpe/Offsets.cpp
    src/mcpe/Symbols.cpp
    src/hooks/CoreHooks.cpp
    src/hooks/UIHooks.cpp
    src/hooks/RenderHooks.cpp
    src/hooks/ResourceHooks.cpp
    src/hooks/ItemHooks.cpp
    src/hooks/BlockHooks.cpp
    src/hooks/RecipeHooks.cpp
    src/hooks/CommandHooks.cpp
    src/plugin/PluginManager.cpp
    src/resource/ResourceManager.cpp
    src/resource/AddonIngestion.cpp
)

add_library(levilamroid SHARED ${SOURCES})

target_link_options(levilamroid PRIVATE
    "-L/data/data/com.termux/files/usr/lib"
    "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/levilamroid.version"
)

target_link_libraries(levilamroid
    PRIVATE
    dl
    log
    GLESv3
)

set_target_properties(levilamroid PROPERTIES
    OUTPUT_NAME "levilamroid"
    PREFIX "lib"
)
