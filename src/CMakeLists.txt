set(SOURCES
    betree.hpp
    btree.hpp
    buffer_manager.hpp
    rbtree.hpp
    segment_file.cc
)

add_library(imlab STATIC ${SOURCES})
target_include_directories(imlab PUBLIC
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/include
)

add_subdirectory(test)
