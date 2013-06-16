
#include "Pancake.h"
#include "PancakeLogger.h"
#include "PancakeConfiguration.h"

PancakeWorker PancakeCurrentWorker;
PancakeMainConfigurationStructure PancakeMainConfiguration;

/* Forward declaration */
UByte PancakeConfigurationServerArchitecture(UByte step, config_setting_t *setting, PancakeConfigurationScope **scope);

/* Here we go. */
Int32 main(Int32 argc, Byte **argv) {
	PancakeConfigurationGroup *group;
	UInt32 i = 0;
	UByte haveDeferred = 0;
	PancakeModule *module;

	// Initialize global variables
	PancakeCurrentWorker.name.value = "Master";
	PancakeCurrentWorker.name.length = sizeof("Master") - 1;

	// Tell the user we are loading
	PancakeLogger(PANCAKE_LOGGER_SYSTEM,
				0,
				&((String) {"Loading Pancake " PANCAKE_VERSION "... " PANCAKE_COPYRIGHT
				, sizeof("Loading Pancake " PANCAKE_VERSION "... " PANCAKE_COPYRIGHT) - 1}));

	// Intialize configuration heap
	PancakeConfigurationInitialize();

	// Set Pancake core settings
	group = PancakeConfigurationAddGroup(NULL, (String) {"Logging", sizeof("Logging") - 1}, NULL);
	PancakeConfigurationAddSetting(group, (String) {"System", sizeof("System") - 1}, CONFIG_TYPE_STRING, &PancakeMainConfiguration.systemLog, sizeof(FILE*), (config_value_t) 0, PancakeConfigurationFile);
	PancakeConfigurationAddSetting(group, (String) {"Request", sizeof("Request") - 1}, CONFIG_TYPE_STRING, &PancakeMainConfiguration.requestLog, sizeof(FILE*), (config_value_t) 0, PancakeConfigurationFile);
	PancakeConfigurationAddSetting(group, (String) {"Error", sizeof("Error") - 1}, CONFIG_TYPE_STRING, &PancakeMainConfiguration.errorLog, sizeof(FILE*), (config_value_t) 0, PancakeConfigurationFile);

	group = PancakeConfigurationAddGroup(NULL, (String) {"Workers", sizeof("Workers") - 1}, NULL);
	PancakeConfigurationAddSetting(group, (String) {"Amount", sizeof("Amount") - 1}, CONFIG_TYPE_INT, &PancakeMainConfiguration.workers, sizeof(Int32), (config_value_t) 2, NULL);
	PancakeConfigurationAddSetting(group, (String) {"User", sizeof("User") - 1}, CONFIG_TYPE_STRING, &PancakeMainConfiguration.user, sizeof(Byte*), (config_value_t) "www-data", NULL);
	PancakeConfigurationAddSetting(group, (String) {"Group", sizeof("Group") - 1}, CONFIG_TYPE_STRING, &PancakeMainConfiguration.group, sizeof(Byte*), (config_value_t) "www-data", NULL);
	PancakeConfigurationAddSetting(group, (String) {"ConcurrencyLimit", sizeof("ConcurrencyLimit") - 1}, CONFIG_TYPE_INT, &PancakeMainConfiguration.concurrencyLimit, sizeof(Int32), (config_value_t) 0, NULL);

	PancakeConfigurationAddSetting(NULL, (String) {"ServerArchitecture", sizeof("ServerArchitecture") - 1}, CONFIG_TYPE_STRING, &PancakeMainConfiguration.serverArchitecture, sizeof(PancakeServerArchitecture*), (config_value_t) 0, PancakeConfigurationServerArchitecture);

	// Fetch modules into accessible array
	PancakeFetchModules(); // Defined by configure.ac

	// Initialize modules
	do {
		while((module = PancakeModules[i])) {
			if(module->intialized) {
				continue;
			}

			PancakeDebug {
				PancakeLoggerFormat(PANCAKE_LOGGER_SYSTEM, 0, "Loading module %s...", module->name);
			}

			if(module->init) {
				UByte retval = module->init();

				switch(retval) {
					case 0:
						// Init failed
						PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Failed to initialize module %s", module->name);
						exit(2);
						break;
					case 1:
						// Init successful
						module->intialized = 1;
						break;
					case 2:
						// Defer init
						haveDeferred = 1;
						break;
				}
			} else {
				module->intialized = 1;
			}

			i++;
		}

		if(haveDeferred) {
			haveDeferred = 0;
			continue;
		}
	} while(0);

	// Parse and check configuration
	if(!PancakeConfigurationLoad()) {
		exit(3);
	}

	// Run configuration module hooks
	i = 0;

	while((module = PancakeModules[i])) {
		if(module->configurationLoaded && !module->configurationLoaded()) {
			PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Configuration check of module %s failed", module->name);
			exit(2);
		}

		i++;
	}

	// Unload configuration and free memory
	PancakeConfigurationUnload();
	PancakeConfigurationDestroy();

	// Unload server architectures
	PancakeNetworkUnload();

	// Show memory leaks
	PancakeDumpHeap();

	return 0;
}
