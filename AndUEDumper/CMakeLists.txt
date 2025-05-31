cmake_minimum_required(VERSION 3.10)

project(UEDump3r_${CMAKE_ANDROID_ARCH})
project(shared_UEDump3r_${CMAKE_ANDROID_ARCH})

set(KITTYMEMORY_PATH ../deps/KittyMemoryEx/KittyMemoryEx)
file(GLOB KITTYMEMORY_SRC ${KITTYMEMORY_PATH}/*.cpp)

set(DEPS_PATH ../deps)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -O0 -g -std=c++20 -fexceptions -DDEBUG -DkNO_KEYSTONE")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O2 -s -std=c++20 -fexceptions -DNDEBUG -DkNO_KEYSTONE")

file(GLOB UE_SRC src/UE/*.cpp)

include_directories(${DEPS_PATH} ${KITTYMEMORY_PATH})
link_libraries(-llog)

add_executable(UEDump3r_${CMAKE_ANDROID_ARCH} ${UE_SRC} src/executable.cpp src/Dumper.cpp src/UPackageGenerator.cpp ${KITTYMEMORY_SRC} ${DEPS_PATH}/fmt/format.cc)
add_library(shared_UEDump3r_${CMAKE_ANDROID_ARCH} SHARED ${UE_SRC} src/library.cpp src/Dumper.cpp src/UPackageGenerator.cpp ${KITTYMEMORY_SRC} ${DEPS_PATH}/fmt/format.cc)

target_compile_definitions(UEDump3r_${CMAKE_ANDROID_ARCH} PRIVATE kEXECUTABLE)