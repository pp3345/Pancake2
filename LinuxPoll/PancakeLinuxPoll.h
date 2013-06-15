
#ifndef _PANCAKE_LINUX_POLL_H
#define _PANCAKE_LINUX_POLL_H

#include "Pancake.h"

#ifdef PANCAKE_LINUX_POLL

extern PancakeModule PancakeLinuxPoll;

UByte PancakeLinuxPollInitialize();
void PancakeLinuxPollWait();

#endif
#endif
