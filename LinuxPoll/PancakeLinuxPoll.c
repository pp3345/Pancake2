#include "PancakeLinuxPoll.h"
#include "PancakeNetwork.h"
#include "PancakeConfiguration.h"

#ifdef PANCAKE_LINUX_POLL

PancakeModule PancakeLinuxPoll = {
		"LinuxPoll",
		PancakeLinuxPollInitialize,
		NULL,
		0
};

PancakeServerArchitecture PancakeLinuxPollServer = {
		(String) {"LinuxPoll", sizeof("LinuxPoll") - 1},

		PancakeLinuxPollWait,

		NULL
};

UByte PancakeLinuxPollInitialize() {
	PancakeRegisterServerArchitecture(&PancakeLinuxPollServer);

	return 1;
}

void PancakeLinuxPollWait() {
}

#endif
