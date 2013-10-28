
#ifndef _PANCAKE_HTTP_FASTCGI_H
#define _PANCAKE_HTTP_FASTCGI_H

#include "Pancake.h"

#ifdef PANCAKE_HTTP_FASTCGI

#include "HTTP/PancakeHTTP.h"

#define PANCAKE_FASTCGI_MAX_REQUEST_ID 1024

typedef struct _PancakeFastCGIClient {
	// MUST be the first element (struct will be casted to PancakeNetworkClientInterface)
	struct sockaddr *address;

	String name;
	PancakeNetworkConnectionCache *connectionCache;

	PancakeHTTPRequest *requests[PANCAKE_FASTCGI_MAX_REQUEST_ID];
	PancakeSocket *sockets[PANCAKE_FASTCGI_MAX_REQUEST_ID];

	Byte multiplex;
	UInt16 highestRequestID;

	UT_hash_handle hh;
} PancakeFastCGIClient;

typedef struct _PancakeFastCGIConfigurationStructure {
	PancakeFastCGIClient *client;
} PancakeFastCGIConfigurationStructure;

extern PancakeModule PancakeHTTPFastCGIModule;

#define PANCAKE_FASTCGI_UNCACHED_CONNECTION 1

/* FastCGI definitions */
#define FCGI_VERSION_1           1

#define FCGI_BEGIN_REQUEST       1
#define FCGI_ABORT_REQUEST       2
#define FCGI_END_REQUEST         3
#define FCGI_PARAMS              4
#define FCGI_STDIN               5
#define FCGI_STDOUT              6
#define FCGI_STDERR              7
#define FCGI_DATA                8
#define FCGI_GET_VALUES          9
#define FCGI_GET_VALUES_RESULT  10
#define FCGI_UNKNOWN_TYPE       11
#define FCGI_MAXTYPE (FCGI_UNKNOWN_TYPE)

#define FCGI_REQUEST_COMPLETE 0
#define FCGI_CANT_MPX_CONN    1
#define FCGI_OVERLOADED       2
#define FCGI_UNKNOWN_ROLE     3

#define FCGI_RESPONDER  1
#define FCGI_AUTHORIZER 2
#define FCGI_FILTER     3

#define FCGI_KEEP_CONN  1

#endif

#endif
