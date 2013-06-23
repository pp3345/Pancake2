
#ifndef _PANCAKE_HTTP_H
#define _PANCAKE_HTTP_H

#include "Pancake.h"
#include "PancakeConfiguration.h"

#ifdef PANCAKE_HTTP

/* Forward declarations */
typedef struct _PancakeHTTPHeader PancakeHTTPHeader;

typedef struct _PancakeHTTPVirtualHost {
	PancakeConfigurationScope *configurationScope;
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

	UByte method;
	UByte HTTPVersion;
} PancakeHTTPRequest;

typedef struct _PancakeHTTPHeader {
	String name;
	String value;

	PancakeHTTPHeader *prev;
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

#endif
#endif
