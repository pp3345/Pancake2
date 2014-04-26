#ifndef _PANCAKE_NETWORK_H
#define _PANCAKE_NETWORK_H

/* Forward declaration is necessary since Pancake.h requires this */
typedef struct _PancakeServerArchitecture PancakeServerArchitecture;

#include "Pancake.h"
#include "PancakeConfiguration.h"

/* Forward declarations */
typedef struct _PancakeSocket PancakeSocket;
typedef struct _PancakeNetworkConnectionCache PancakeNetworkConnectionCache;
typedef struct _PancakeNetworkLayer PancakeNetworkLayer;

typedef void (*PancakeNetworkEventHandler)(PancakeSocket *socket);
typedef void (*PancakeSocketHandler)(PancakeSocket *socket);

typedef struct _PancakeNetworkBuffer {
	UByte *value;
	UInt32 length;
	UInt32 size;
} PancakeNetworkBuffer;

typedef struct _PancakeSocket {
	Int32 fd;
	UInt32 flags;
	PancakeNetworkEventHandler onRead;
	PancakeNetworkEventHandler onWrite;
	PancakeNetworkEventHandler onRemoteHangup;
	PancakeNetworkBuffer readBuffer;
	PancakeNetworkBuffer writeBuffer;
	PancakeNetworkLayer *layer;

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

	PancakeSocketHandler onSocketClose;

	PancakeModuleInitializeFunction initialize;

	UT_hash_handle hh;
} PancakeServerArchitecture;

typedef struct _PancakeNetworkClientInterface {
	struct sockaddr *address;
} PancakeNetworkClientInterface;

typedef struct _PancakeNetworkConnectionCache {
	PancakeSocket *socket;

	PancakeNetworkConnectionCache *next;
} PancakeNetworkConnectionCache;

#define PANCAKE_NETWORK_LAYER_MODE_SERVER 1
#define PANCAKE_NETWORK_LAYER_MODE_CLIENT 2

typedef UByte (*PancakeNetworkLayerAcceptConnectionFunction)(PancakeSocket **socket, PancakeSocket *parent);
typedef Int32 (*PancakeNetworkLayerReadFunction)(PancakeSocket *socket, UInt32 maxLength, UByte *buf);
typedef Int32 (*PancakeNetworkLayerWriteFunction)(PancakeSocket *socket);
typedef void (*PancakeNetworkLayerCloseFunction)(PancakeSocket *socket);
typedef void (*PancakeNetworkLayerConfigurationFunction)(PancakeConfigurationGroup *parent, UByte mode);

typedef struct _PancakeNetworkLayer {
	String name;

	PancakeNetworkLayerConfigurationFunction configure;
	PancakeNetworkLayerAcceptConnectionFunction acceptConnection;
	PancakeNetworkLayerReadFunction read;
	PancakeNetworkLayerWriteFunction write;
	PancakeNetworkLayerCloseFunction close;

	struct _PancakeNetworkLayer *next;
} PancakeNetworkLayer;

/* Forward declaration */
typedef struct _PancakeConfigurationGroup PancakeConfigurationGroup;
typedef struct _PancakeConfigurationSetting PancakeConfigurationSetting;
typedef struct _PancakeConfigurationScope PancakeConfigurationScope;
typedef struct config_setting_t config_setting_t;
typedef UByte (*PancakeConfigurationHook)(UByte step, config_setting_t *setting, PancakeConfigurationScope **scope);

PANCAKE_API void PancakeRegisterServerArchitecture(PancakeServerArchitecture *arch);
PANCAKE_API PancakeConfigurationGroup *PancakeNetworkRegisterClientInterfaceGroup(PancakeConfigurationGroup *parent, PancakeConfigurationHook hook);
PANCAKE_API PancakeConfigurationSetting *PancakeNetworkRegisterListenInterfaceGroup(PancakeConfigurationGroup *parent, PancakeConfigurationHook hook);
PANCAKE_API UByte PancakeNetworkInterfaceConfiguration(UByte step, config_setting_t *setting, PancakeConfigurationScope **scope);
PANCAKE_API void PancakeNetworkClientInterfaceConfiguration(PancakeNetworkClientInterface *client);
PANCAKE_API Byte *PancakeNetworkGetInterfaceName(struct sockaddr *addr);
PANCAKE_API void PancakeNetworkReplaceListenSocket(PancakeSocket *previous, PancakeSocket *new);

PANCAKE_API inline PancakeSocket *PancakeNetworkAcceptConnection(PancakeSocket *sock);
PANCAKE_API inline PancakeSocket *PancakeNetworkConnect(struct sockaddr *addr, PancakeNetworkConnectionCache **cache, UByte cachePolicy);
PANCAKE_API inline Int32 PancakeNetworkRead(PancakeSocket *sock, UInt32 maxLength);
PANCAKE_API inline Int32 PancakeNetworkWrite(PancakeSocket *sock);
PANCAKE_API inline void PancakeNetworkClose(PancakeSocket *sock);

PANCAKE_API void PancakeNetworkActivateListenSockets();
PANCAKE_API inline void PancakeNetworkCacheConnection(PancakeNetworkConnectionCache **cache, PancakeSocket *socket);
PANCAKE_API inline void PancakeNetworkUncacheConnection(PancakeNetworkConnectionCache **cache, PancakeSocket *sock);

PANCAKE_API void PancakeNetworkRegisterNetworkLayer(PancakeNetworkLayer *layer);

/* NetworkTLS */
#ifdef PANCAKE_NETWORK_TLS

typedef String* (*PancakeNetworkTLSApplicationLayerProtocolNegotiationFunction)(PancakeSocket *sock, String *input);
typedef String* (*PancakeNetworkTLSNextProtocolNegotiationFunction)(PancakeSocket *sock);
typedef UByte (*PancakeNetworkTLSInitializationFunction)(PancakeSocket *sock);
typedef UByte (*PancakeNetworkTLSServerNameIndication)(PancakeSocket *sock, String *input);

typedef struct _PancakeNetworkTLSApplicationProtocol {
	String name;

	PancakeNetworkTLSApplicationLayerProtocolNegotiationFunction ALPN;
	PancakeNetworkTLSNextProtocolNegotiationFunction NPN;
	PancakeNetworkTLSServerNameIndication SNI;
	PancakeNetworkTLSInitializationFunction initialize;

	struct _PancakeNetworkTLSApplicationProtocol *next;
} PancakeNetworkTLSApplicationProtocol;

PANCAKE_API void PancakeNetworkTLSRegisterApplicationProtocol(PancakeNetworkTLSApplicationProtocol *module);
PANCAKE_API PancakeNetworkTLSApplicationProtocol *PancakeNetworkTLSGetApplicationProtocol(String *name);
#endif

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
void PancakeNetworkUnload();

#define PANCAKE_NETWORK_CONNECTION_CACHE_KEEP 1
#define PANCAKE_NETWORK_CONNECTION_CACHE_REMOVE 2

#endif
