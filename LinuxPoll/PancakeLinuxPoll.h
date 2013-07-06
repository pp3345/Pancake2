
#ifndef _PANCAKE_LINUX_POLL_H
#define _PANCAKE_LINUX_POLL_H

#include "Pancake.h"

#ifdef PANCAKE_LINUX_POLL

#include <sys/epoll.h>

extern PancakeModule PancakeLinuxPoll;
extern Int32 PancakeLinuxPollFD;

#define PANCAKE_LINUX_POLL_SOCKET 	1 << 10
#define PANCAKE_LINUX_POLL_IN 		1 << 11
#define PANCAKE_LINUX_POLL_OUT		1 << 12

#endif
#endif
