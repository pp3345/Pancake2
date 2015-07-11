option(PANCAKE_AUTHENTICATION "Enable Pancake Authentication module" OFF)

if(PANCAKE_AUTHENTICATION)
    pancake_enable_module("Authentication" "PancakeAuthenticationModule" "Authentication/PancakeAuthentication.h")
    set(PANCAKE_SOURCE_FILES ${PANCAKE_SOURCE_FILES} Authentication/PancakeAuthentication.c)
endif()
