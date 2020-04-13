cmake_minimum_required (VERSION 2.8)

project(demo_For_Epoll_Client)

#add_subdirectory(src)

aux_source_directory (src SRC_LIST)

include_directories (include)

add_executable (epoll_client ${SRC_LIST})

set (EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
