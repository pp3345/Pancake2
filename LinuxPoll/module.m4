AC_ARG_ENABLE([linux-poll], 
				[AS_HELP_STRING([--enable-linux-poll], [Enable Pancake Linux epoll server architecture module])],
				[])
				
if test "$enable_linux_poll" == "yes"; then
	PANCAKE_MODULE_HEADERS+="LinuxPoll/PancakeLinuxPoll.h "
	PANCAKE_MODULES+="PancakeLinuxPoll "
	PANCAKE_HAVE_SERVER_ARCHITECTURE=1
	AC_DEFINE([PANCAKE_LINUX_POLL], [1], [Pancake LinuxPoll module])
	AC_CHECK_HEADER([sys/epoll.h], [], [AC_ERROR([epoll is not available on your system])])
fi