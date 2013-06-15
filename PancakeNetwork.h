#ifndef _PANCAKE_NETWORK_H
#define _PANCAKE_NETWORK_H

/* Forward declaration is necessary since Pancake.h requires this */
typedef struct _PancakeServerArchitecture PancakeServerArchitecture;

#include "Pancake.h"
#include "PancakeConfiguration.h"

typedef struct _PancakeSocket PancakeSocket;

typedef void (*PancakeNetworkEventHandler)(PancakeSocket *socket);

typedef struct _PancakeSocket {
	Int32 fd;
	PancakeNetworkEventHandler onRead;
	PancakeNetworkEventHandler onWrite;
	PancakeNetworkEventHandler onRemoteHangup;
	String *readBuffer;
	String *writeBuffer;
	void *data;
} PancakeSocket;

typedef struct _PancakeServerArchitecture {
	String name;

	PancakeWorkerEntryFunction runServer;

	UT_hash_handle hh;
} PancakeServerArchitecture;

PANCAKE_API void PancakeRegisterServerArchitecture(PancakeServerArchitecture *arch);

#endif
