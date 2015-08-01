cmake_minimum_required(VERSION 2.8.4)
project(Pancake2 C)

message(STATUS "${CMAKE_SYSTEM} on ${CMAKE_SYSTEM_PROCESSOR} processor")

include(CheckIncludeFile)
include(CheckIncludeFiles)
include(CheckFunctionExists)
include(CheckLibraryExists)
include(CheckTypeSize)
include(FindFLEX)
include(FindBISON)

macro(pancake_require_module NAME)
    set(REQUIRED_MODULES ${REQUIRED_MODULES} ${NAME})
endmacro()

macro(pancake_enable_module NAME MODULE INCLUDE_FILE)
    message(STATUS "Enabling module ${NAME}")

    string(TOUPPER ${NAME} NAME_UPPER)
    add_definitions(-DPANCAKE_${NAME_UPPER})
    set(PANCAKE_MODULE_${NAME} 1)

    set(MODULE_VARIABLES ${MODULE_VARIABLES} ${MODULE})
    set(MODULE_INCLUDES ${MODULE_INCLUDES} ${INCLUDE_FILE})
endmacro()

macro(pancake_link_library LIBRARY)
    message(STATUS "Linking with library ${LIBRARY}")

    set(LINK_LIBRARIES ${LINK_LIBRARIES} ${LIBRARY})
endmacro()

macro(require_include_file NAME)
    check_include_file("${NAME}" HAVE_INCLUDE_FILE)

    if(NOT HAVE_INCLUDE_FILE)
        unset(HAVE_INCLUDE_FILE CACHE)
        message(FATAL_ERROR "${NAME} not found")
    endif()

    unset(HAVE_INCLUDE_FILE CACHE)
endmacro()

macro(require_library LIBRARY FUNCTION LOCATION)
    check_library_exists(${LIBRARY} ${FUNCTION} "${LOCATION}" HAVE_LIBRARY)

    if(NOT HAVE_LIBRARY)
        unset(HAVE_LIBRARY CACHE)
        message(FATAL_ERROR "${FUNCTION} not found in library ${LIBRARY} (or library not found)")
    endif()

    unset(HAVE_LIBRARY CACHE)
endmacro()

set(PANCAKE_SOURCE_FILES ConfigurationParser/PancakeConfigurationParser.c
    ConfigurationParser/scanctx.c
    ConfigurationParser/scanner.l
    ConfigurationParser/strbuf.c
    SharedDependencies/Base64Decode.c
    Pancake.c
    PancakeConfiguration.c
    PancakeDateTime.c
    PancakeDebug.c
    PancakeLogger.c
    PancakeNetwork.c
    PancakeScheduler.c
    PancakeWorkers.c
    PancakeModules.c)

require_include_file("errno.h")
require_include_file("unistd.h")
require_include_file("stdlib.h")
require_include_file("stddef.h")
require_include_file("stdio.h")
require_include_file("string.h")
require_include_file("stdarg.h")
require_include_file("signal.h")
require_include_file("fcntl.h")
require_include_file("ctype.h")
require_include_file("limits.h")
require_include_file("time.h")
require_include_file("sys/stat.h")
require_include_file("sys/types.h")
require_include_file("sys/socket.h")
require_include_file("sys/un.h")
require_include_file("netinet/in.h")

if(NOT CYGWIN)
    require_include_file("netinet/tcp.h") # Cygwin does not correctly include type definitions
endif()

require_include_file("arpa/inet.h")

check_include_file("valgrind/valgrind.h" HAVE_VALGRIND_H)
check_include_file("ucontext.h" HAVE_UCONTEXT_H)
check_include_file("execinfo.h" HAVE_EXECINFO_H)
check_include_file("xlocale.h" HAVE_XLOCALE_H)
check_type_size("long" SIZEOF_LONG)

check_function_exists("newlocale" HAVE_NEWLOCALE)
check_function_exists("uselocale" HAVE_USELOCALE)
check_function_exists("freelocale" HAVE_FREELOCALE)

find_package(BISON)
find_package(FLEX)

bison_target(ConfigurationParserGrammar ConfigurationParser/grammar.y ${CMAKE_CURRENT_SOURCE_DIR}/ConfigurationParser/grammar.c)
flex_target(ConfigurationParserScanner ConfigurationParser/scanner.l ${CMAKE_CURRENT_SOURCE_DIR}/ConfigurationParser/scanner.c)
add_flex_bison_dependency(ConfigurationParserScanner ConfigurationParserGrammar)

set(PANCAKE_SOURCE_FILES ${PANCAKE_SOURCE_FILES} ${BISON_ConfigurationParserGrammar_OUTPUTS} ${FLEX_ConfigurationParserScanner_OUTPUTS})

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Enabling Pancake debug mode")
    set(PANCAKE_DEBUG 1)
endif()

file(GLOB FILES ${CMAKE_CURRENT_SOURCE_DIR}/*)
foreach(FILE ${FILES})
    if(IS_DIRECTORY ${FILE})
        if(EXISTS "${FILE}/PancakeModule.cmake")
            message(STATUS "Found Pancake module ${FILE}")
            include("${FILE}/PancakeModule.cmake")
        endif()
    endif()
endforeach()

set(PANCAKE_CONFIG_PATH "config/pancake.cfg" CACHE STRING "Pancake configuration path")

foreach(MODULE ${REQUIRED_MODULES})
    message(STATUS "Checking for required module ${MODULE}")

    if(NOT DEFINED PANCAKE_MODULE_${MODULE})
        message(FATAL_ERROR "Required module ${MODULE} was not enabled")
    endif()
endforeach()

configure_file(${Pancake2_SOURCE_DIR}/config.h.in ${Pancake2_BINARY_DIR}/config.h)

add_executable(GeneratePancakeModules PancakeModulesGenerator.c)
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/PancakeModules.c
    COMMAND GeneratePancakeModules ${CMAKE_CURRENT_BINARY_DIR}/PancakeModules.c ${MODULE_INCLUDES} ${MODULE_VARIABLES}
    DEPENDS GeneratePancakeModules
)

add_definitions(-DHAVE_CONFIG_H)
add_executable(Pancake ${PANCAKE_SOURCE_FILES})

if(LINK_LIBRARIES)
    target_link_libraries(Pancake ${LINK_LIBRARIES})
endif()

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
