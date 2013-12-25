
#include "PancakeAuthentication.h"

#ifdef PANCAKE_AUTHENTICATION

#include "PancakeConfiguration.h"
#include "PancakeLogger.h"

PancakeModule PancakeAuthenticationModule = {
		"Authentication",
		PancakeAuthenticationInitialize,
		NULL,
		PancakeAuthenticationShutdown,
		0
};

static PancakeAuthenticationBackend *PancakeAuthenticationBackends = NULL;
static PancakeAuthenticationConfiguration *PancakeAuthenticationConfigurations = NULL;

/* Global variable */
PancakeAuthenticationConfiguration *PancakeAuthenticationActiveConfiguration = NULL;

static UByte PancakeAuthenticationGroupConfiguration(UByte step, config_setting_t *setting, PancakeConfigurationScope **scope) {
	PancakeAuthenticationConfiguration *config;

	if(step == PANCAKE_CONFIGURATION_INIT) {
		config = PancakeAllocate(sizeof(PancakeAuthenticationConfiguration));

		setting->hook = (void*) config;
		config->backend = NULL;
		config->backendData = NULL;
	} else {
		config = (PancakeAuthenticationConfiguration*) setting->hook;
		PancakeFree(config);
	}

	return 1;
}

static UByte PancakeAuthenticationNameConfiguration(UByte step, config_setting_t *setting, PancakeConfigurationScope **scope) {
	PancakeAuthenticationConfiguration *config = (PancakeAuthenticationConfiguration*) setting->parent->hook;

	if(step == PANCAKE_CONFIGURATION_INIT) {
		config->name.value = setting->value.sval;
		config->name.length = strlen(setting->value.sval);

		HASH_ADD_KEYPTR(hh, PancakeAuthenticationConfigurations, config->name.value, config->name.length, config);
	} else {
		HASH_DEL(PancakeAuthenticationConfigurations, config);
	}

	return 1;
}

static UByte PancakeAuthenticationBackendConfiguration(UByte step, config_setting_t *setting, PancakeConfigurationScope **scope) {
	PancakeAuthenticationConfiguration *config = (PancakeAuthenticationConfiguration*) setting->parent->hook;

	if(step == PANCAKE_CONFIGURATION_INIT) {
		UInt32 length = strlen(setting->value.sval);

		HASH_FIND(hh, PancakeAuthenticationBackends, setting->value.sval, length, config->backend);

		if(config->backend == NULL) {
			PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Unknown authentication backend %s", setting->value.sval);
			return 0;
		}

		// Call onConfiguration handler
		if(config->backend->onConfiguration) {
			config->backend->onConfiguration(config);
		}

		// Save some memory
		free(setting->value.sval);
		setting->type = CONFIG_TYPE_NONE;
	}

	return 1;
}

static UByte PancakeAuthenticationConfigurationConfiguration(UByte step, config_setting_t *setting, PancakeConfigurationScope **scope) {
	PancakeAuthenticationConfiguration *config = NULL;

	if(step == PANCAKE_CONFIGURATION_INIT) {
		UInt32 length = strlen(setting->value.sval);

		HASH_FIND(hh, PancakeAuthenticationConfigurations, setting->value.sval, length, config);

		if(config == NULL) {
			PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Unknown authentication configuration %s", setting->value.sval);
			return 0;
		}

		free(setting->value.sval);
		setting->type = CONFIG_TYPE_SPECIAL;
		setting->value.sval = (void*) config;
	} else {
		// Make library happy
		setting->type = CONFIG_TYPE_NONE;
	}

	return 1;
}

UByte PancakeAuthenticationInitialize() {
	PancakeConfigurationGroup *group;
	PancakeConfigurationSetting *setting;

	setting = PancakeConfigurationAddSetting(NULL, (String) {"Authentication", sizeof("Authentication") - 1}, CONFIG_TYPE_LIST, NULL, 0, (config_value_t) 0, NULL);
	group = PancakeConfigurationListGroup(setting, PancakeAuthenticationGroupConfiguration);

	PancakeConfigurationAddSetting(group, (String) {"Backend", sizeof("Backend") - 1}, CONFIG_TYPE_STRING, NULL, 0, (config_value_t) 0, PancakeAuthenticationBackendConfiguration);
	PancakeConfigurationAddSetting(group, (String) {"Name", sizeof("Name") - 1}, CONFIG_TYPE_STRING, NULL, 0, (config_value_t) 0, PancakeAuthenticationNameConfiguration);

	PancakeConfigurationAddSetting(NULL, (String) {"AuthenticationConfiguration", sizeof("AuthenticationConfiguration") - 1}, CONFIG_TYPE_STRING, &PancakeAuthenticationActiveConfiguration, sizeof(void*), (config_value_t) 0, PancakeAuthenticationConfigurationConfiguration);

	return 1;
}

UByte PancakeAuthenticationShutdown() {
	PancakeAuthenticationBackend *backend, *tmp;

	HASH_ITER(hh, PancakeAuthenticationBackends, backend, tmp) {
		HASH_DEL(PancakeAuthenticationBackends, backend);
	}

	return 1;
}

PANCAKE_API void PancakeAuthenticationRegisterBackend(PancakeAuthenticationBackend *backend) {
	HASH_ADD_KEYPTR(hh, PancakeAuthenticationBackends, backend->name.value, backend->name.length, backend);
}

PANCAKE_API inline UByte PancakeAuthenticate(PancakeAuthenticationConfiguration *config, void *data) {
	return config->backend->authenticate(config, data);
}

#endif

