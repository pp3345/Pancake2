
#include "Pancake.h"
#include "PancakeLogger.h"
#include "PancakeConfiguration.h"
#include "PancakeWorkers.h"
#include "PancakeNetwork.h"
#include "PancakeScheduler.h"

PancakeWorker *PancakeCurrentWorker;
PancakeWorker **PancakeWorkerRegistry;
PancakeMainConfigurationStructure PancakeMainConfiguration;
UByte PancakeDoShutdown = 0;

/* Forward declarations */
STATIC void PancakeSignalHandler(Int32 type, siginfo_t *info, void *context);
void PancakeFetchModules();

/* Here we go. */
Int32 main(Int32 argc, Byte **argv) {
	PancakeConfigurationGroup *group;
	UInt32 i = 0;
	UByte haveDeferred = 0;
	PancakeModule *module;
	PancakeWorker worker;
	struct sigaction signalAction;
	sigset_t signalSet;

	// Initialize segfault handling
#if defined(HAVE_SIGACTION) && defined(HAVE_PANCAKE_SIGSEGV)
	PancakeDebug {
#ifdef HAVE_VALGRIND_H
		if(!RUNNING_ON_VALGRIND) {
#endif
			struct sigaction action;

			action.sa_sigaction = PancakeDebugHandleSegfault;
			action.sa_flags = SA_RESTART | SA_SIGINFO;

			sigaction(SIGSEGV, &action, NULL);
#ifdef HAVE_VALGRIND_H
		}
#endif
	}
#endif

	// Initialize global variables
	worker.name.value = "Master";
	worker.name.length = sizeof("Master") - 1;
	worker.pid = getpid();
	worker.isMaster = 1;
	PancakeCurrentWorker = &worker;

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

	group = PancakeConfigurationAddGroup(NULL, (String) {"NetworkBuffering", sizeof("NetworkBuffering") - 1}, NULL);
	PancakeConfigurationAddSetting(group, (String) {"Max", sizeof("Max") - 1}, CONFIG_TYPE_INT, &PancakeMainConfiguration.networkBufferingMax, sizeof(Int32), (config_value_t) 131072, NULL);
	PancakeConfigurationAddSetting(group, (String) {"Min", sizeof("Min") - 1}, CONFIG_TYPE_INT, &PancakeMainConfiguration.networkBufferingMin, sizeof(Int32), (config_value_t) 10240, NULL);

	// Fetch modules into accessible array
	PancakeFetchModules(); // Defined by configure.ac

	// Initialize modules
	do {
		for(i = 0; module = PancakeModules[i]; i++) {
			if(module->initialized) {
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
						module->initialized = 1;
						break;
					case 2:
						// Defer init
						haveDeferred = 1;
						break;
				}
			} else {
				module->initialized = 1;
			}
		}

		if(haveDeferred) {
			haveDeferred = 0;
			continue;
		}

		break;
	} while(1);

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

	// Initialize server architecture
	if(PancakeMainConfiguration.serverArchitecture->initialize) {
		PancakeMainConfiguration.serverArchitecture->initialize();
	}

	// Activate sockets
	if(!PancakeNetworkActivate()) {
		exit(3);
	}

	// Initialize signal handling
	sigemptyset(&signalSet);
	sigaddset(&signalSet, SIGCHLD);
	sigaddset(&signalSet, SIGINT);
	sigaddset(&signalSet, SIGTERM);
	sigaddset(&signalSet, SIGPIPE);
	signalAction.sa_sigaction = PancakeSignalHandler;
	signalAction.sa_mask = signalSet;
	signalAction.sa_flags = SA_SIGINFO;

	sigaction(SIGCHLD, &signalAction, NULL);
	sigaction(SIGINT, &signalAction, NULL);
	sigaction(SIGTERM, &signalAction, NULL);
	sigaction(SIGPIPE, &signalAction, NULL);

	// Run workers
	if(PancakeMainConfiguration.workers > 0) {
		// Multithreaded mode
		UInt16 i;

		// Allocate worker registry
		PancakeWorkerRegistry = PancakeAllocate(PancakeMainConfiguration.workers * sizeof(PancakeWorker*));
		memset(PancakeWorkerRegistry, '\0', PancakeMainConfiguration.workers * sizeof(PancakeWorker*));

		PancakeDebug {
			PancakeLoggerFormat(PANCAKE_LOGGER_SYSTEM, 0, "Multithreaded mode enabled with %i workers", PancakeMainConfiguration.workers);
		}

		// Run workers
		for(i = 1; i <= PancakeMainConfiguration.workers; i++) {
			PancakeWorker *worker = PancakeAllocate(sizeof(PancakeWorker));

			worker->name.value = PancakeAllocate(sizeof("Worker #65535"));
			worker->name.length = sprintf(worker->name.value, "Worker #%i", i);
			worker->run = PancakeMainConfiguration.serverArchitecture->runServer;
			worker->isMaster = 0;

			switch(PancakeRunWorker(worker)) {
				case 0:
					// Fork failed
					PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Unable to run worker");
					exit(1);
				case 1: {
					// Child started successfully

					// Add worker to registry
					PancakeWorkerRegistry[i - 1] = worker;

					// Sleep forever after all workers are started
					if(i == PancakeMainConfiguration.workers) {
						do {
							sleep(3600);
						} while(!PancakeDoShutdown);
					}
				} break;
				case 2: {
					goto shutdown;
				}
			}
		}
	} else {
		// Singlethreaded mode

		PancakeDebug {
			PancakeLoggerFormat(PANCAKE_LOGGER_SYSTEM, 0, "Singlethreaded mode enabled");
		}

		// Run server
		PancakeMainConfiguration.serverArchitecture->runServer();
	}

	shutdown:
	// Shutdown

	// Scheduler first (will run all scheduled events, should not free anything before)
	PancakeSchedulerShutdown();

	if(PancakeCurrentWorker->isMaster) {
		UInt16 i;

		PancakeLoggerFormat(PANCAKE_LOGGER_SYSTEM, 0, "Stopping...");

		for(i = 0; i < PancakeMainConfiguration.workers; i++) {
			PancakeWorker *worker = PancakeWorkerRegistry[i];

			write(worker->masterSocket, PANCAKE_WORKER_GRACEFUL_SHUTDOWN, sizeof(PANCAKE_WORKER_GRACEFUL_SHUTDOWN));
		}
	}

	// Destroy worker registry
	if(PancakeMainConfiguration.workers > 0) {
		for(i = 0; i < PancakeMainConfiguration.workers; i++) {
			PancakeWorker *worker = PancakeWorkerRegistry[i];

			if(worker == NULL) {
				continue;
			}

			PancakeFree(worker->name.value);
			PancakeFree(worker);
		}

		PancakeFree(PancakeWorkerRegistry);
	}

	// Unload configuration and free memory
	PancakeConfigurationUnload();
	PancakeConfigurationDestroy();

	// Unload server architectures
	PancakeNetworkUnload();

	// Call module shutdown hooks
	i = 0;
	while(module = PancakeModules[i]) {
		if(module->shutdown) {
			module->shutdown();
		}

		i++;
	}

	// Free worker
	if(!PancakeCurrentWorker->isMaster) {
		PancakeFree(PancakeCurrentWorker->name.value);
		PancakeFree(PancakeCurrentWorker);
	}

	// Show memory leaks
	PancakeDumpHeap();
	PancakeDumpMemoryUsage();
	PancakeFreeAllocatorMeta();

	return 0;
}

STATIC void PancakeSignalHandler(Int32 type, siginfo_t *info, void *context) {
	switch(type) {
		case SIGINT:
		case SIGTERM:
			PancakeDoShutdown = 1;
			break;
		case SIGPIPE:
			return;
		case SIGCHLD:
			if(PancakeDoShutdown) {
				return;
			}

			PancakeDoShutdown = 1;

			PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Worker crashed");
			break;
	}
}
