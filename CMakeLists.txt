cmake_minimum_required(VERSION 3.7)
project(be-tree)
enable_testing()

set(CXX_STANDARD 17)
set(CXX_STANDARD_REQUIRED ON)

find_package(GTest REQUIRED)
find_package(benchmark REQUIRED)

add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(bench)
