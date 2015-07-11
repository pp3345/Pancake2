option(PANCAKE_HTTP_BASIC_AUTHENTICATION "Enable Pancake HTTP Basic Authentication module" OFF)

if(PANCAKE_HTTP_BASIC_AUTHENTICATION)
    pancake_enable_module("HTTPBasicAuthentication" "PancakeHTTPBasicAuthenticationModule" "HTTPBasicAuthentication/PancakeHTTPBasicAuthentication.h")
    pancake_require_module("HTTP")
    pancake_require_module("Authentication")

    set(PANCAKE_SOURCE_FILES ${PANCAKE_SOURCE_FILES} HTTPBasicAuthentication/PancakeHTTPBasicAuthentication.c)
endif()
