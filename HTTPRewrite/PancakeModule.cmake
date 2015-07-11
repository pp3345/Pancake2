option(PANCAKE_HTTP_REWRITE "Enable Pancake HTTP Rewrite module" OFF)

if(PANCAKE_HTTP_REWRITE)
    pancake_enable_module("HTTPRewrite" "PancakeHTTPRewriteModule" "HTTPRewrite/PancakeHTTPRewrite.h")
    pancake_require_module("HTTP")

    set(PANCAKE_SOURCE_FILES ${PANCAKE_SOURCE_FILES} HTTPRewrite/PancakeHTTPRewrite.c HTTPRewrite/PancakeHTTPRewriteDefaultVariables.c HTTPRewrite/PancakeHTTPRewriteVM.c)
endif()
