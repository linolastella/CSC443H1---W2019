cmake_minimum_required(VERSION 3.0.0)
project(A2 VERSION 0.1.0)

include(CTest)
enable_testing()

set(SOURCE src)

add_executable(external_sorting ${SOURCE}/external_sorting.cpp)
add_executable(hash_indexing ${SOURCE}/hash_indexing.cpp)
add_executable(query_by_index ${SOURCE}/query_by_index.cpp)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
