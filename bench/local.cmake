# ---------------------------------------------------------------------------
# IMLAB
# ---------------------------------------------------------------------------

add_executable(bm_foo ${CMAKE_SOURCE_DIR}/bench/bm_foo.cc)

target_link_libraries(
    bm_foo
    imlab
    benchmark
    gflags
    Threads::Threads)


add_executable(bm_trees ${CMAKE_SOURCE_DIR}/bench/bm_trees.cc)

target_link_libraries(
    bm_trees
    imlab)
