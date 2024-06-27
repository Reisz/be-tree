set(SOURCES
    check_betree.hpp
    check_rbtree.hpp
)

add_library(imlab_test STATIC ${SOURCES})
target_link_libraries(imlab_test PUBLIC imlab)
