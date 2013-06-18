
#ifndef _PANCAKE_LINUX_POLL_H
#define _PANCAKE_LINUX_POLL_H

#include "Pancake.h"

#ifdef PANCAKE_LINUX_POLL

#include <sys/epoll.h>

extern PancakeModule PancakeLinuxPoll;
extern Int32 PancakeLinuxPollFD;

#endif
#endif
