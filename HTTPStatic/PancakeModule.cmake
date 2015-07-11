option(PANCAKE_HTTP_STATIC "Enable Pancake HTTP static file serving module" ON)

if(PANCAKE_HTTP_STATIC)
    pancake_enable_module("HTTPStatic" "PancakeHTTPStatic" "HTTPStatic/PancakeHTTPStatic.h")
    pancake_require_module("HTTP")

    set(PANCAKE_SOURCE_FILES ${PANCAKE_SOURCE_FILES} HTTPStatic/PancakeHTTPStatic.c)
endif()
