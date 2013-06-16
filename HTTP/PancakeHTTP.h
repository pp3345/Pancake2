
#ifndef _PANCAKE_HTTP_H
#define _PANCAKE_HTTP_H

#include "Pancake.h"
#include "PancakeConfiguration.h"

#ifdef PANCAKE_HTTP

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

extern PancakeModule PancakeHTTP;
extern PancakeHTTPVirtualHostIndex *PancakeHTTPVirtualHosts;
extern PancakeHTTPVirtualHost *PancakeHTTPDefaultVirtualHost;
extern PancakeHTTPConfigurationStructure PancakeHTTPConfiguration;

UByte PancakeHTTPInitialize();

#endif
#endif
