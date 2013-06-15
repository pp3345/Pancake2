#include "PancakeHTTP.h"
#include "PancakeConfiguration.h"

#ifdef PANCAKE_HTTP

PancakeModule PancakeHTTP = {
		"HTTP",
		PancakeHTTPInitialize,
		NULL,
		0
};

UByte PancakeHTTPInitialize() {
	PancakeConfigurationGroup *group;

	group = PancakeConfigurationAddGroup(NULL, (String) {"HTTP", sizeof("HTTP") - 1}, NULL);
	//PancakeConfigurationAddSetting(group, (String) {"VirtualHosts", sizeof("VirtualHosts") - 1}, CONFIG_TYPE_ARRAY, NULL, 0, (config_value_t) 0, NULL);

	return 1;
}

#endif
