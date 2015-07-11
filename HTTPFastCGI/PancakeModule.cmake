option(PANCAKE_HTTP_FASTCGI "Enable Pancake HTTP Basic Authentication module" OFF)

if(PANCAKE_HTTP_FASTCGI)
    pancake_enable_module("HTTPFastCGI" "PancakeHTTPFastCGIModule" "HTTPFastCGI/PancakeHTTPFastCGI.h")
    pancake_require_module("HTTP")

    set(PANCAKE_SOURCE_FILES ${PANCAKE_SOURCE_FILES} HTTPFastCGI/PancakeHTTPFastCGI.c)
endif()
