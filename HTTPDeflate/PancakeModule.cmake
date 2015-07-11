option(PANCAKE_HTTP_DEFLATE "Enable Pancake HTTP deflate compression module" OFF)

if(PANCAKE_HTTP_DEFLATE)
    pancake_enable_module("HTTPDeflate" "PancakeHTTPDeflate" "HTTPDeflate/PancakeHTTPDeflate.h")
    pancake_require_module("HTTP")

    message(STATUS "Looking for zlib")
    include(FindZLIB)

    if(NOT ZLIB_FOUND)
        message(FATAL_ERROR "zlib not found")
    endif()

    message(STATUS "Found zlib ${ZLIB_VERSION_STRING}")

    require_include_file("zlib.h")

    foreach(LIBRARY ${ZLIB_LIBRARIES})
        pancake_link_library(${LIBRARY})
    endforeach()

    set(PANCAKE_SOURCE_FILES ${PANCAKE_SOURCE_FILES} HTTPDeflate/PancakeHTTPDeflate.c)
endif()
