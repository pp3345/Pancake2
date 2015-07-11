option(PANCAKE_OPENSSL "Enable Pancake OpenSSL cryptographic network layer module" OFF)

if(PANCAKE_OPENSSL)
    pancake_enable_module("OpenSSL" "PancakeOpenSSL" "OpenSSL/PancakeOpenSSL.h")

    message(STATUS "Looking for OpenSSL")
    include(FindOpenSSL)

    if(NOT OPENSSL_FOUND)
        message(FATAL_ERROR "OpenSSL library not found")
    endif()

    message(STATUS "Found OpenSSL ${OPENSSL_VERSION}")

    require_include_file("openssl/ssl.h")
    require_include_file("openssl/err.h")
    require_include_file("openssl/evp.h")
    require_include_file("openssl/conf.h")
    require_include_file("openssl/engine.h")
    require_include_file("openssl/pem.h")

    foreach(LIBRARY ${OPENSSL_LIBRARIES})
        pancake_link_library(${LIBRARY})
    endforeach()

    set(PANCAKE_NETWORK_TLS 1)
    set(PANCAKE_SOURCE_FILES ${PANCAKE_SOURCE_FILES} OpenSSL/PancakeOpenSSL.c)
endif()
