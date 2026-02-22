if (POLICY CMP0063)
    cmake_policy(SET CMP0063 NEW)
endif ()

# CRoaring 4.5.x uses C11 alignof/_Static_assert; MSVC C does not support them.
# Compile as C++ on MSVC so the file builds (roaring.c is C-compatible).
if (MSVC)
    set_source_files_properties(roaring.c PROPERTIES LANGUAGE CXX)
endif ()

add_library(roaring_bitmap STATIC roaring.c)

target_include_directories(
        roaring_bitmap
        PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
