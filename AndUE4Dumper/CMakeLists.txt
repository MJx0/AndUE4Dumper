cmake_minimum_required(VERSION 3.5)

project(UE4Dump3r_${CMAKE_ANDROID_ARCH})
project(shared_UE4Dump3r_${CMAKE_ANDROID_ARCH})

set(KITTYMEMORY_PATH ../deps/KittyMemoryEx/KittyMemoryEx)
file(GLOB KITTYMEMORY_SRC ${KITTYMEMORY_PATH}/*.cpp)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -Wno-unused-command-line-argument -O0 -g -std=c++20 -DDEBUG -DkNO_KEYSTONE")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -Wno-unused-command-line-argument -O2 -s -std=c++20 -DNDEBUG -DkNO_KEYSTONE")

file(GLOB PROJ_SRC source/src/KittyCmdln.cpp source/src/Core/*.cpp source/src/Core/GameProfiles/*.cpp)

include_directories(source/includes ${KITTYMEMORY_PATH})
link_libraries(-llog)

add_executable(UE4Dump3r_${CMAKE_ANDROID_ARCH} ${PROJ_SRC} source/src/executable.cpp ${KITTYMEMORY_SRC} source/includes/fmt/format.cc)
add_library(shared_UE4Dump3r_${CMAKE_ANDROID_ARCH} SHARED ${PROJ_SRC} source/src/library.cpp ${KITTYMEMORY_SRC} source/includes/fmt/format.cc)