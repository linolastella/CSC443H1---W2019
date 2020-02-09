cmake_minimum_required(VERSION 3.0.0)
project(A1 VERSION 0.1.0)

include(CTest)
enable_testing()

add_subdirectory(sqlite)

add_executable(writer write.cc)
add_executable(reader read.cc)

target_link_libraries(writer PUBLIC sqlite3)
target_link_libraries(reader PUBLIC sqlite3)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
