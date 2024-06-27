add_executable(bm_foo ${CMAKE_SOURCE_DIR}/bench/bm_foo.cc)
target_link_libraries(bm_foo
    PRIVATE imlab
    PRIVATE benchmark::benchmark
)

add_executable(bm_trees ${CMAKE_SOURCE_DIR}/bench/bm_trees.cc)
target_link_libraries(bm_trees
    PRIVATE imlab
)

add_executable(tree_perf ${CMAKE_SOURCE_DIR}/bench/tree_perf.cc)
target_link_libraries(tree_perf
    PRIVATE imlab
)
