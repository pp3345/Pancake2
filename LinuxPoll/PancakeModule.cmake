option(PANCAKE_LINUX_POLL "Enable Pancake Linux epoll server architecture module")

if(PANCAKE_LINUX_POLL)
    pancake_enable_module("LinuxPoll" "PancakeLinuxPoll" "LinuxPoll/PancakeLinuxPoll.h")
    set(HAVE_SERVER_ARCHITECTURE 1)
    set(PANCAKE_SOURCE_FILES ${PANCAKE_SOURCE_FILES} "LinuxPoll/PancakeLinuxPoll.c")
    require_include_file("sys/epoll.h")
endif()
