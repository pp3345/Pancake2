#include "PancakeHTTP.h"
#include "PancakeConfiguration.h"
#include "PancakeLogger.h"
#include "PancakeNetwork.h"

#ifdef PANCAKE_HTTP

PancakeModule PancakeHTTP = {
		"HTTP",
		PancakeHTTPInitialize,
		NULL,
		NULL,
		0
};

PancakeHTTPVirtualHostIndex *PancakeHTTPVirtualHosts = NULL;
PancakeHTTPVirtualHost *PancakeHTTPDefaultVirtualHost = NULL;
PancakeHTTPConfigurationStructure PancakeHTTPConfiguration;

/* Forward declarations */
static void PancakeHTTPInitializeConnection(PancakeSocket *sock);

static UByte PancakeHTTPVirtualHostConfiguration(UByte step, config_setting_t *setting, PancakeConfigurationScope **scope) {
	PancakeHTTPVirtualHost *vhost;

	switch(step) {
		case PANCAKE_CONFIGURATION_INIT: {
			vhost = PancakeAllocate(sizeof(PancakeHTTPVirtualHost));
			*scope = PancakeConfigurationAddScope();
			(*scope)->data = (void*) vhost;
			vhost->configurationScope = *scope;

			setting->hook = (void*) vhost;
		} break;
		case PANCAKE_CONFIGURATION_DTOR: {
			vhost = (PancakeHTTPVirtualHost*) setting->hook;

			PancakeConfigurationDestroyScope(vhost->configurationScope);
			PancakeFree(vhost);
		} break;
	}

	return 1;
}

static UByte PancakeHTTPHostsConfiguration(UByte step, config_setting_t *setting, PancakeConfigurationScope **scope) {
	switch(step) {
		case PANCAKE_CONFIGURATION_INIT: {
			PancakeHTTPVirtualHost *vHost = (PancakeHTTPVirtualHost*) (*scope)->data;
			config_setting_t *element;
			UInt16 i = 0;

			while(element = config_setting_get_elem(setting, i++)) {
				PancakeHTTPVirtualHostIndex *index = PancakeAllocate(sizeof(PancakeHTTPVirtualHostIndex));

				index->vHost = vHost;

				HASH_ADD_KEYPTR(hh, PancakeHTTPVirtualHosts, &element->value.sval, strlen(element->value.sval), index);
			}
		} break;
		case PANCAKE_CONFIGURATION_DTOR: {
			config_setting_t *element;
			UInt16 i = 0;

			while(element = config_setting_get_elem(setting, i++)) {
				PancakeHTTPVirtualHostIndex *index;

				HASH_FIND(hh, PancakeHTTPVirtualHosts, &element->value.sval, strlen(element->value.sval), index);
				PancakeAssert(index != NULL);

				HASH_DEL(PancakeHTTPVirtualHosts, index);
				PancakeFree(index);
			}
		} break;
	}

	return 1;
}

static UByte PancakeHTTPDefaultConfiguration(UByte step, config_setting_t *setting, PancakeConfigurationScope **scope) {
	if(step == PANCAKE_CONFIGURATION_INIT && setting->value.ival == 1) {
		PancakeHTTPDefaultVirtualHost = (PancakeHTTPVirtualHost*) (*scope)->data;
	}

	return 1;
}

static UByte PancakeHTTPDocumentRootConfiguration(UByte step, config_setting_t *setting, PancakeConfigurationScope **scope) {
	String *documentRoot;

	switch(step) {
		case PANCAKE_CONFIGURATION_INIT: {
			struct stat buf;

			if(stat(setting->value.sval, &buf) == -1) {
				PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Can't stat document root %s: %s", setting->value.sval, strerror(errno));
				return 0;
			}

			if(!S_ISDIR(buf.st_mode)) {
				PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Document root %s is not a directory", setting->value.sval);
				return 0;
			}

			// Make String out of document root
			documentRoot = PancakeAllocate(sizeof(String));
			documentRoot->length = strlen(setting->value.sval);
			documentRoot->value = PancakeAllocate(documentRoot->length + 1);
			memcpy(documentRoot->value, setting->value.sval, documentRoot->length + 1);

			free(setting->value.sval);
			setting->type = CONFIG_TYPE_SPECIAL;
			setting->value.sval = (char*) documentRoot;
		} break;
		case PANCAKE_CONFIGURATION_DTOR: {
			// Free memory
			documentRoot = (String*) setting->value.sval;
			PancakeFree(documentRoot->value);
			PancakeFree(documentRoot);

			// Make library happy
			setting->type = CONFIG_TYPE_NONE;
		} break;
	}

	return 1;
}

static UByte PancakeHTTPNetworkInterfaceConfiguration(UByte step, config_setting_t *setting, PancakeConfigurationScope **scope) {
	if(PancakeNetworkInterfaceConfiguration(step, setting, scope) && step == PANCAKE_CONFIGURATION_INIT) {
		PancakeSocket *sock = (PancakeSocket*) setting->hook;

		sock->onRead = PancakeHTTPInitializeConnection;

		return 1;
	}

	return 0;
}

UByte PancakeHTTPInitialize() {
	PancakeConfigurationGroup *group;
	PancakeConfigurationSetting *setting;

	group = PancakeConfigurationAddGroup(NULL, (String) {"HTTP", sizeof("HTTP") - 1}, NULL);
	PancakeNetworkRegisterListenInterfaceGroup(group, PancakeHTTPNetworkInterfaceConfiguration);

	setting = PancakeConfigurationAddSetting(group, (String) {"VirtualHosts", sizeof("VirtualHosts") - 1}, CONFIG_TYPE_LIST, NULL, 0, (config_value_t) 0, NULL);
	group = PancakeConfigurationListGroup(setting, PancakeHTTPVirtualHostConfiguration);
	PancakeConfigurationAddSetting(group, (String) {"Hosts", sizeof("Hosts") - 1}, CONFIG_TYPE_LIST, NULL, 0, (config_value_t) 0, PancakeHTTPHostsConfiguration);
	PancakeConfigurationAddSetting(group, (String) {"Default", sizeof("Default") - 1}, CONFIG_TYPE_INT, NULL, 0, (config_value_t) 0, PancakeHTTPDefaultConfiguration);
	PancakeConfigurationAddSetting(group, (String) {"DocumentRoot", sizeof("DocumentRoot") - 1}, CONFIG_TYPE_STRING, &PancakeHTTPConfiguration.documentRoot, sizeof(String*), (config_value_t) "", PancakeHTTPDocumentRootConfiguration);
	PancakeConfigurationAddSetting(group, (String) {"ContentServeBackends", sizeof("ContentServeBackends") - 1}, CONFIG_TYPE_LIST, NULL, 0, (config_value_t) 0, NULL);
	PancakeConfigurationAddSetting(group, (String) {"OutputFilters", sizeof("OutputFilters") - 1}, CONFIG_TYPE_LIST, NULL, 0, (config_value_t) 0, NULL);

	PancakeConfigurationAddGroupByName(group, (String) {"Logging", sizeof("Logging") - 1});

	return 1;
}

static void PancakeHTTPInitializeConnection(PancakeSocket *sock) {
	PancakeSocket *client = PancakeNetworkAcceptConnection(sock);

	if(client == NULL) {
		return;
	}
}

#endif
