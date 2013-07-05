
#ifndef _PANCAKE_HTTP_H
#define _PANCAKE_HTTP_H

#include "Pancake.h"
#include "PancakeConfiguration.h"
#include "PancakeNetwork.h"
#include "MIME/PancakeMIME.h"

#ifdef PANCAKE_HTTP

/* Forward declarations */
typedef struct _PancakeHTTPHeader PancakeHTTPHeader;
typedef struct _PancakeHTTPContentServeBackend PancakeHTTPContentServeBackend;

typedef UByte (*PancakeHTTPContentServeHandler)(PancakeSocket *sock);

typedef struct _PancakeHTTPContentServeBackend {
	UByte *name;
	PancakeHTTPContentServeHandler handler;

	PancakeHTTPContentServeBackend *next;
} PancakeHTTPContentServeBackend;

typedef struct _PancakeHTTPVirtualHost {
	PancakeConfigurationScope *configurationScope;
	PancakeHTTPContentServeHandler *contentBackends;

	UInt16 numContentBackends;
} PancakeHTTPVirtualHost;

typedef struct _PancakeHTTPVirtualHostIndex {
	PancakeHTTPVirtualHost *vHost;

	UT_hash_handle hh;
} PancakeHTTPVirtualHostIndex;

typedef struct _PancakeHTTPConfigurationStructure {
	String *documentRoot;
} PancakeHTTPConfigurationStructure;

typedef struct _PancakeHTTPRequest {
	String requestAddress;
	String host;
	String path;

	PancakeHTTPVirtualHost *vHost;
	PancakeHTTPHeader *headers;

	PancakeConfigurationScopeGroup scopeGroup;

	struct stat fileStat;

	UInt32 contentLength;
	UInt16 answerCode;
	PancakeMIMEType *answerType;
	void *contentServeData;

	UByte method;
	UByte HTTPVersion;
	UByte statDone;
	UByte chunkedTransfer;
} PancakeHTTPRequest;

typedef struct _PancakeHTTPHeader {
	String name;
	String value;

	PancakeHTTPHeader *next;
} PancakeHTTPHeader;

#define PANCAKE_HTTP_GET 1
#define PANCAKE_HTTP_POST 2

#define PANCAKE_HTTP_10 1
#define PANCAKE_HTTP_11 2

extern PancakeModule PancakeHTTP;
extern PancakeHTTPVirtualHostIndex *PancakeHTTPVirtualHosts;
extern PancakeHTTPVirtualHost *PancakeHTTPDefaultVirtualHost;
extern PancakeHTTPConfigurationStructure PancakeHTTPConfiguration;

UByte PancakeHTTPInitialize();

PANCAKE_API void PancakeHTTPRegisterContentServeBackend(PancakeHTTPContentServeBackend *backend);
PANCAKE_API UByte PancakeHTTPRunAccessChecks(PancakeSocket *sock);
PANCAKE_API void PancakeHTTPOnRemoteHangup(PancakeSocket *sock);

#endif
#endif
