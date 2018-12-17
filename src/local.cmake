# ---------------------------------------------------------------------------
# IMLAB
# ---------------------------------------------------------------------------

# ---------------------------------------------------------------------------
# Includes
# ---------------------------------------------------------------------------

set(SRC_CC_LINTER_IGNORE "")
# include("${CMAKE_SOURCE_DIR}/src/this-could-be-your-folder/local.cmake")

# ---------------------------------------------------------------------------
# Files
# ---------------------------------------------------------------------------

file(GLOB_RECURSE SRC_CC src/*.cc src/*.hpp)
set(SRC_CC ${SRC_CC})

file(GLOB_RECURSE INCLUDE_TCC src/*.hpp)
set(INCLUDE_TCC ${INCLUDE_TCC})

# Gather lintable files
set(SRC_CC_LINTING "")
foreach(SRC_FILE ${SRC_CC})
    list(FIND SRC_CC_LINTER_IGNORE "${SRC_FILE}" SRC_FILE_IDX)
    if (${SRC_FILE_IDX} EQUAL -1)
        list(APPEND SRC_CC_LINTING "${SRC_FILE}")
    endif()
endforeach()

# ---------------------------------------------------------------------------
# Library
# ---------------------------------------------------------------------------

add_library(imlab STATIC ${SRC_CC} ${INCLUDE_H})
target_include_directories(imlab INTERFACE src)
target_link_libraries(imlab gflags Threads::Threads)

# ---------------------------------------------------------------------------
# Linting
# ---------------------------------------------------------------------------

add_cpplint_target(lint_src "${SRC_CC_LINTING}")
list(APPEND lint_targets lint_src)
