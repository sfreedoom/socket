cmake_minimum_required (VERSION 2.8)

project (demoForEpollServer)

aux_source_directory (src SRC_LIST)

include_directories (include)

add_executable (epoll_server ${SRC_LIST})

set (EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
