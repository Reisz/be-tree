set(SOURCES
    betree_test.cc
    btree_test.cc
    buffer_manager_test.cc
    rbtree_test.cc
)

add_executable(tester ${SOURCES})
target_link_libraries(tester
    PRIVATE imlab_test
    PRIVATE GTest::gtest
    PRIVATE GTest::gtest_main
)

gtest_discover_tests(tester)
