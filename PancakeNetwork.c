#include "PancakeNetwork.h"
#include "PancakeLogger.h"

static PancakeServerArchitecture *architectures = NULL;

PANCAKE_API void PancakeRegisterServerArchitecture(PancakeServerArchitecture *arch) {
	HASH_ADD_KEYPTR(hh, architectures, arch->name.value, arch->name.length, arch);
}

void PancakeNetworkUnload() {
	HASH_CLEAR(hh, architectures);
}

UByte PancakeConfigurationServerArchitecture(UByte step, config_setting_t *setting, PancakeConfigurationScope **scope) {
	PancakeServerArchitecture *arch;

	switch(step) {
		case PANCAKE_CONFIGURATION_INIT:
			PancakeAssert(setting->type == CONFIG_TYPE_STRING);

			HASH_FIND(hh, architectures, setting->value.sval, strlen(setting->value.sval), arch);

			// Fail if server architecture does not exist
			if(arch == NULL) {
				PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Unknown server architecture %s", setting->value.sval);
				return 0;
			}

			free(setting->value.sval);

			// Set special type
			setting->type = CONFIG_TYPE_SERVER_ARCHITECTURE;
			setting->value.sval = (char*) arch;

			break;
		case PANCAKE_CONFIGURATION_DTOR:
			// Reset type to make library happy
			if(setting->type == CONFIG_TYPE_SERVER_ARCHITECTURE) {
				setting->type = CONFIG_TYPE_NONE;
			}

			break;
	}

	return 1;
}
