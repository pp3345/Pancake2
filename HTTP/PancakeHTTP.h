
#ifndef _PANCAKE_HTTP_H
#define _PANCAKE_HTTP_H

#include "../Pancake.h"
#include "../PancakeConfiguration.h"
#include "../PancakeNetwork.h"
#include "../MIME/PancakeMIME.h"
#include "../PancakeScheduler.h"

/* Forward declarations */
typedef struct _PancakeHTTPHeader PancakeHTTPHeader;
typedef struct _PancakeHTTPContentServeBackend PancakeHTTPContentServeBackend;
typedef struct _PancakeHTTPOutputFilter PancakeHTTPOutputFilter;
typedef struct _PancakeHTTPParserHook PancakeHTTPParserHook;
typedef struct _PancakeHTTPRequest PancakeHTTPRequest;

typedef UByte (*PancakeHTTPContentServeHandler)(PancakeSocket *sock);
typedef UByte (*PancakeHTTPOutputFilterFunction)(PancakeSocket *sock, String *output);
typedef UByte (*PancakeHTTPParserHookFunction)(PancakeSocket *sock);
typedef void (*PancakeHTTPEventHandler)(PancakeHTTPRequest *request);

#define PANCAKE_HTTP_SERVER_HEADER "Server: Pancake/" PANCAKE_VERSION "\r\n"
#define PANCAKE_HTTP_SERVER_TOKEN "Pancake " PANCAKE_VERSION

typedef struct _PancakeHTTPContentServeBackend {
	UByte *name;
	PancakeHTTPContentServeHandler handler;

	PancakeHTTPContentServeBackend *next;
} PancakeHTTPContentServeBackend;

typedef struct _PancakeHTTPOutputFilter {
	UByte *name;
	PancakeHTTPOutputFilterFunction handler;

	PancakeHTTPOutputFilter *next;
} PancakeHTTPOutputFilter;

typedef struct _PancakeHTTPParserHook {
	UByte *name;
	PancakeHTTPParserHookFunction handler;

	PancakeHTTPParserHook *next;
} PancakeHTTPParserHook;

typedef struct _PancakeHTTPVirtualHost {
	PancakeConfigurationScope *configurationScope;
	PancakeHTTPContentServeHandler *contentBackends;
	PancakeHTTPOutputFilterFunction *outputFilters;
	PancakeHTTPParserHookFunction *parserHooks;

#ifdef PANCAKE_HTTPREWRITE
	void *rewriteConfiguration;
#endif

	UInt8 numContentBackends;
	UInt8 numOutputFilters;
	UInt8 numParserHooks;
} PancakeHTTPVirtualHost;

typedef struct _PancakeHTTPVirtualHostIndex {
	PancakeHTTPVirtualHost *vHost;

	UT_hash_handle hh;
} PancakeHTTPVirtualHostIndex;

typedef struct _PancakeHTTPConfigurationStructure {
	String *documentRoot;
	UByte serverHeader;
	UInt32 requestTimeout;
	UInt32 keepAliveTimeout;
} PancakeHTTPConfigurationStructure;

typedef struct _PancakeHTTPRequest {
	String requestAddress;
	String path;
	String ifModifiedSince;
	StringOffset host;
	StringOffset userAgent;
	StringOffset acceptEncoding;
	StringOffset authorization;

	PancakeHTTPVirtualHost *vHost;
	PancakeHTTPHeader *headers;
	PancakeHTTPHeader *answerHeaders;

	PancakeConfigurationScopeGroup scopeGroup;

	struct stat fileStat;

	UInt32 clientContentLength;
	UInt32 contentLength;
	UInt16 answerCode;
	UInt16 headerEnd;
	PancakeMIMEType *answerType;
	void *contentServeData;
	void *outputFilterData;
	String *contentEncoding;
	PancakeHTTPOutputFilterFunction outputFilter;
	Native lastModified;

	PancakeHTTPEventHandler onRequestEnd;
	PancakeHTTPEventHandler onOutputEnd;
	PancakeSocket *socket;
	PancakeSchedulerEvent *schedulerEvent;

	UByte method;
	UByte HTTPVersion;
	UByte statDone;
	UByte chunkedTransfer;
	UByte keepAlive;
	UByte headerSent;
} PancakeHTTPRequest;

typedef struct _PancakeHTTPHeader {
	String name;
	String value;

	PancakeHTTPHeader *next;
} PancakeHTTPHeader;

#define PANCAKE_HTTP_GET 1
#define PANCAKE_HTTP_POST 2
#define PANCAKE_HTTP_HEAD 3

#define PANCAKE_HTTP_10 1
#define PANCAKE_HTTP_11 2

#define PANCAKE_HTTP_EXCEPTION 1 << 0
#define PANCAKE_HTTP_HEADER_DATA_COMPLETE 1 << 1
#define PANCAKE_HTTP_CLIENT_HANGUP 1 << 2
#define PANCAKE_HTTPS 1 << 3

#define PANCAKE_HTTP_EXCEPTION_PAGE_HEADER "<!doctype html><html><head><title>"
#define PANCAKE_HTTP_EXCEPTION_PAGE_BODY_ERROR "</title><style>body{font-family:\"Arial\"}hr{border:1px solid #000}</style></head><body><h1>"
#define PANCAKE_HTTP_EXCEPTION_PAGE_BODY_TOKEN "</h1><hr>"
#define PANCAKE_HTTP_EXCEPTION_PAGE_FOOTER "</body></html>"


extern PancakeModule PancakeHTTP;
extern PancakeHTTPVirtualHostIndex *PancakeHTTPVirtualHosts;
extern PancakeHTTPVirtualHost *PancakeHTTPDefaultVirtualHost;
extern PancakeHTTPConfigurationStructure PancakeHTTPConfiguration;
extern String PancakeHTTPAnswerCodes[];
extern const String PancakeHTTPMethods[];
extern UInt16 PancakeHTTPNumVirtualHosts;

UByte PancakeHTTPInitialize();
UByte PancakeHTTPCheckConfiguration();
void PancakeHTTPSRegisterProtocol();

PANCAKE_API void PancakeHTTPRegisterContentServeBackend(PancakeHTTPContentServeBackend *backend);
PANCAKE_API void PancakeHTTPRegisterOutputFilter(PancakeHTTPOutputFilter *filter);
PANCAKE_API void PancakeHTTPRegisterParserHook(PancakeHTTPParserHook *hook);
PANCAKE_API UByte PancakeHTTPRunAccessChecks(PancakeSocket *sock);
PANCAKE_API inline UByte PancakeHTTPServeContent(PancakeSocket *sock, UByte ignoreException);
PANCAKE_API void PancakeHTTPException(PancakeSocket *sock, UInt16 code);
PANCAKE_API inline void PancakeHTTPOnRemoteHangup(PancakeSocket *sock);
PANCAKE_API inline void PancakeHTTPFullWriteBuffer(PancakeSocket *sock);
PANCAKE_API void PancakeHTTPOutput(PancakeSocket *sock, String *output);
PANCAKE_API void PancakeHTTPOutputChunk(PancakeSocket *sock, String *output);
PANCAKE_API void PancakeHTTPSendChunk(PancakeSocket *sock, String *chunk);
PANCAKE_API inline void PancakeHTTPSendLastChunk(PancakeSocket *sock);
PANCAKE_API void PancakeHTTPBuildAnswerHeaders(PancakeSocket *sock);
PANCAKE_API inline void PancakeHTTPOnRequestEnd(PancakeSocket *sock);
PANCAKE_API inline void PancakeHTTPOnWrite(PancakeSocket *sock);
PANCAKE_API inline void PancakeHTTPRemoveQueryString(PancakeHTTPRequest *request);
PANCAKE_API inline void PancakeHTTPExtractQueryString(PancakeHTTPRequest *request, String *queryString);
PANCAKE_API inline void PancakeHTTPFreeContentEncoding(PancakeHTTPRequest *request);

#endif
