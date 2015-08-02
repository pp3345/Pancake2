option(PANCAKE_SELECT "Enable Pancake select() server architecture module")

if(PANCAKE_SELECT)
    pancake_enable_module("Select" "PancakeSelect" "Select/PancakeSelect.h")
    set(HAVE_SERVER_ARCHITECTURE 1)
    set(PANCAKE_SOURCE_FILES ${PANCAKE_SOURCE_FILES} "Select/PancakeSelect.c")
    require_include_file("sys/select.h")
endif()
