cmake_minimum_required (VERSION 2.8)

project (socket)

include(CheckCXXCompilerFlag)
include_directories (include)
aux_source_directory (src SRC_LIST)

add_executable (socket.bin ${SRC_LIST})

add_definitions(-std=c++11 -Wl,--no-as-needed -pthread)

set(LIBRARY_OUTPUT_PATH /lib)
set(EXECUTABLE_OUTPUT_PATH /bin)
set(CMAKE_CXX_FLAGS " -std=c++11 -march=native -O3 -pthread" )

CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
