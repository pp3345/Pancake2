#ifndef _PANCAKE_NETWORK_H
#define _PANCAKE_NETWORK_H

/* Forward declaration is necessary since Pancake.h requires this */
typedef struct _PancakeServerArchitecture PancakeServerArchitecture;

#include "Pancake.h"
#include "PancakeConfiguration.h"

typedef struct _PancakeSocket PancakeSocket;

typedef void (*PancakeNetworkEventHandler)(PancakeSocket *socket);
typedef void (*PancakeSocketHandler)(PancakeSocket *socket);

typedef struct _PancakeNetworkBuffer {
	UByte *value;
	UInt32 length;
	UInt32 size;
} PancakeNetworkBuffer;

typedef struct _PancakeSocket {
	Int32 fd;
	UInt16 flags;
	UInt16 backlog;
	PancakeNetworkEventHandler onRead;
	PancakeNetworkEventHandler onWrite;
	PancakeNetworkEventHandler onRemoteHangup;
	PancakeNetworkBuffer readBuffer;
	PancakeNetworkBuffer writeBuffer;

	struct sockaddr *localAddress;
	struct sockaddr remoteAddress;

	void *data;
} PancakeSocket;

typedef struct _PancakeServerArchitecture {
	String name;

	PancakeWorkerEntryFunction runServer;

	PancakeSocketHandler addReadSocket;
	PancakeSocketHandler addWriteSocket;
	PancakeSocketHandler addReadWriteSocket;

	PancakeSocketHandler removeReadSocket;
	PancakeSocketHandler removeWriteSocket;
	PancakeSocketHandler removeSocket;

	PancakeSocketHandler setReadSocket;
	PancakeSocketHandler setWriteSocket;
	PancakeSocketHandler setSocket;

	PancakeModuleInitializeFunction initialize;

	UT_hash_handle hh;
} PancakeServerArchitecture;

/* Forward declaration */
typedef struct _PancakeConfigurationGroup PancakeConfigurationGroup;
typedef struct _PancakeConfigurationSetting PancakeConfigurationSetting;
typedef struct _PancakeConfigurationScope PancakeConfigurationScope;
typedef struct config_setting_t config_setting_t;
typedef UByte (*PancakeConfigurationHook)(UByte step, config_setting_t *setting, PancakeConfigurationScope **scope);

PANCAKE_API void PancakeRegisterServerArchitecture(PancakeServerArchitecture *arch);
PANCAKE_API PancakeConfigurationSetting *PancakeNetworkRegisterListenInterfaceGroup(PancakeConfigurationGroup *parent, PancakeConfigurationHook hook);
PANCAKE_API UByte PancakeNetworkInterfaceConfiguration(UByte step, config_setting_t *setting, PancakeConfigurationScope **scope);
PANCAKE_API Byte *PancakeNetworkGetInterfaceName(struct sockaddr *addr);

PANCAKE_API inline PancakeSocket *PancakeNetworkAcceptConnection(PancakeSocket *sock);
PANCAKE_API inline UInt32 PancakeNetworkRead(PancakeSocket *sock, UInt32 maxLength);
PANCAKE_API inline UInt32 PancakeNetworkWrite(PancakeSocket *sock);
PANCAKE_API inline void PancakeNetworkClose(PancakeSocket *sock);

PANCAKE_API void PancakeNetworkActivateListenSockets();

#define PancakeNetworkAddReadSocket(socket) (PancakeMainConfiguration.serverArchitecture->addReadSocket(socket))
#define PancakeNetworkAddWriteSocket(socket) (PancakeMainConfiguration.serverArchitecture->addWriteSocket(socket))
#define PancakeNetworkAddReadWriteSocket(socket) (PancakeMainConfiguration.serverArchitecture->addReadWriteSocket(socket))

#define PancakeNetworkRemoveSocket(socket) (PancakeMainConfiguration.serverArchitecture->removeSocket(socket))
#define PancakeNetworkRemoveReadSocket(socket) (PancakeMainConfiguration.serverArchitecture->removeReadSocket(socket))
#define PancakeNetworkRemoveWriteSocket(socket) (PancakeMainConfiguration.serverArchitecture->removeWriteSocket(socket))

#define PancakeNetworkSetReadSocket(socket) (PancakeMainConfiguration.serverArchitecture->setReadSocket(socket))
#define PancakeNetworkSetWriteSocket(socket) (PancakeMainConfiguration.serverArchitecture->setWriteSocket(socket))
#define PancakeNetworkSetSocket(socket) (PancakeMainConfiguration.serverArchitecture->setSocket(socket))

UByte PancakeNetworkActivate();

#endif
