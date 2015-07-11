option(PANCAKE_MIME "Enable Pancake MIME module" ON)

if(PANCAKE_MIME)
    pancake_enable_module("MIME" "PancakeMIME" "MIME/PancakeMIME.h")
    set(PANCAKE_SOURCE_FILES ${PANCAKE_SOURCE_FILES} MIME/PancakeMIME.c)
endif()
