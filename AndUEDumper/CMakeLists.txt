cmake_minimum_required(VERSION 3.5)

project(UEDump3r_${CMAKE_ANDROID_ARCH})
project(shared_UEDump3r_${CMAKE_ANDROID_ARCH})

set(KITTYMEMORY_PATH ../deps/KittyMemoryEx/KittyMemoryEx)
file(GLOB KITTYMEMORY_SRC ${KITTYMEMORY_PATH}/*.cpp)

set(DEPS_PATH ../deps)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -Wno-unused-command-line-argument -O0 -g -std=c++20 -DDEBUG -DkNO_KEYSTONE")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -Wno-unused-command-line-argument -O2 -s -std=c++20 -DNDEBUG -DkNO_KEYSTONE")

file(GLOB PROJ_SRC src/KittyCmdln.cpp src/Core/*.cpp)

include_directories(${DEPS_PATH} ${KITTYMEMORY_PATH})
link_libraries(-llog)

add_executable(UEDump3r_${CMAKE_ANDROID_ARCH} ${PROJ_SRC} src/executable.cpp ${KITTYMEMORY_SRC} ${DEPS_PATH}/fmt/format.cc)
add_library(shared_UEDump3r_${CMAKE_ANDROID_ARCH} SHARED ${PROJ_SRC} src/library.cpp ${KITTYMEMORY_SRC} ${DEPS_PATH}/fmt/format.cc)