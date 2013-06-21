
#ifndef _PANCAKE_WORKERS_H
#define _PANCAKE_WORKERS_H

#include "Pancake.h"
#include "PancakeNetwork.h"

#define PANCAKE_WORKER_GRACEFUL_SHUTDOWN "\1"
#define PANCAKE_WORKER_GRACEFUL_SHUTDOWN_INT '\1'

typedef struct _PancakeWorker {
	String name;
	PancakeWorkerEntryFunction run;
	Int32 pid;

	Int32 masterSocket;
	PancakeSocket workerSocket;

	UByte isMaster;
} PancakeWorker;

PANCAKE_API UByte PancakeRunWorker(PancakeWorker *worker);

extern PancakeWorker *PancakeCurrentWorker;
extern PancakeWorker **PancakeWorkerRegistry;

#endif
