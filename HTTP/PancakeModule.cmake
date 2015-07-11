option(PANCAKE_HTTP "Enable Pancake HTTP module" ON)

if(PANCAKE_HTTP)
    pancake_enable_module("HTTP" "PancakeHTTP" "HTTP/PancakeHTTP.h")
    pancake_require_module("MIME")

    set(PANCAKE_SOURCE_FILES ${PANCAKE_SOURCE_FILES} HTTP/PancakeHTTP.c HTTP/PancakeHTTPS.c)
endif()
