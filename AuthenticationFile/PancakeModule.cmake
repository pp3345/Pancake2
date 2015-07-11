option(PANCAKE_AUTHENTICATION_FILE "Enable Pancake File Authentication Backend module" OFF)

if(PANCAKE_AUTHENTICATION_FILE)
    pancake_enable_module("AuthenticationFile" "PancakeAuthenticationFileModule" "AuthenticationFile/PancakeAuthenticationFile.h")
    pancake_require_module("Authentication")

    message(STATUS "Looking for OpenSSL")
    include(FindOpenSSL)

    if(NOT OPENSSL_FOUND)
        message(FATAL_ERROR "OpenSSL library not found")
    endif()

    message(STATUS "Found OpenSSL ${OPENSSL_VERSION}")

    require_include_file("openssl/md5.h")
    require_include_file("openssl/sha.h")

    foreach(LIBRARY ${OPENSSL_LIBRARIES})
        pancake_link_library(${LIBRARY})
    endforeach()

    set(PANCAKE_SOURCE_FILES ${PANCAKE_SOURCE_FILES} AuthenticationFile/PancakeAuthenticationFile.c)
endif()
