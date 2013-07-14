
#include "PancakeHTTPDeflate.h"

#ifdef PANCAKE_HTTP_DEFLATE

#include "PancakeConfiguration.h"
#include "PancakeLogger.h"

static UByte PancakeHTTPDeflateChunk(PancakeSocket *sock, String *chunk);
static UByte PancakeHTTPDeflateInitialize();
static void PancakeHTTPDeflateOnOutputEnd(PancakeHTTPRequest *request);

PancakeModule PancakeHTTPDeflate = {
		"HTTPDeflate",

		PancakeHTTPDeflateInitialize,
		NULL,
		NULL,

		0
};

PancakeHTTPOutputFilter PancakeHTTPDeflateFilter = {
		"Deflate",
		PancakeHTTPDeflateChunk,

		NULL
};

PancakeHTTPDeflateConfigurationStructure PancakeHTTPDeflateConfiguration;

static String PancakeHTTPDeflateContentEncoding = {
		"deflate",
		sizeof("deflate") - 1
};

static UByte PancakeHTTPDeflateInitialize() {
	PancakeConfigurationGroup *HTTP, *vHostGroup, *group;
	PancakeConfigurationSetting *vHost;

	// Defer if HTTP module is not yet initialized
	if(!PancakeHTTP.intialized) {
		return 2;
	}

	// Register deflate filter
	PancakeHTTPRegisterOutputFilter(&PancakeHTTPDeflateFilter);

	HTTP = PancakeConfigurationLookupGroup(NULL, (String) {"HTTP", sizeof("HTTP") - 1});
	vHost = PancakeConfigurationLookupSetting(HTTP, (String) {"VirtualHosts", sizeof("VirtualHosts") - 1});
	vHostGroup = vHost->listGroup;
	group = PancakeConfigurationAddGroup(HTTP, (String) {"Deflate", sizeof("Deflate") - 1}, NULL);
	PancakeConfigurationAddSetting(group, (String) {"Level", sizeof("Level") - 1}, CONFIG_TYPE_INT, &PancakeHTTPDeflateConfiguration.level, sizeof(Int32), (config_value_t) 0, NULL);

	// Deflate -> vHost configuration
	PancakeConfigurationAddGroupToGroup(vHostGroup, group);

	return 1;
}

static void PancakeHTTPDeflateOnOutputEnd(PancakeHTTPRequest *request) {
	deflateEnd(request->outputFilterData);
	PancakeFree(request->outputFilterData);
}

static UByte PancakeHTTPDeflateChunk(PancakeSocket *sock, String *chunk) {
	PancakeHTTPRequest *request = (PancakeHTTPRequest*) sock->data;
	z_streamp stream;
	String output;
	UByte out[chunk->length];

	// Is there already an existing deflate stream for this request?
	if(!request->outputFilterData) {
		UByte *d;

		// Check whether client accepts deflate encoding
		if(request->HTTPVersion == PANCAKE_HTTP_11 // Chunked TE requires HTTP 1.1
		&& PancakeHTTPDeflateConfiguration.level // Is deflate enabled?
		&& !request->contentEncoding // Is the content already encoded?
		&& request->acceptEncoding.value
		&& (d = memchr(request->acceptEncoding.value, 'd', request->acceptEncoding.length))
		&& request->acceptEncoding.value + request->acceptEncoding.length - d >= sizeof("eflate") - 1
		&& !memcmp(d + 1, "eflate", sizeof("eflate") - 1)) {
			request->outputFilterData = PancakeAllocate(sizeof(z_stream));
			stream = (z_streamp) request->outputFilterData;

			// Initialize deflate stream
			stream->zalloc = NULL;
			stream->zfree = NULL;
			stream->opaque = NULL;

			if(deflateInit2(stream, PancakeHTTPDeflateConfiguration.level, Z_DEFLATED, -15, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
				PancakeFree(stream);
				return 0;
			}

			// Set Content-Encoding and chunked Transfer-Encoding
			request->onOutputEnd = PancakeHTTPDeflateOnOutputEnd;
			request->chunkedTransfer = 1;
			request->contentEncoding = &PancakeHTTPDeflateContentEncoding;

			// Build answer headers
			PancakeHTTPBuildAnswerHeaders(sock);
		} else {
			// Client does not accept deflate coding
			return 0;
		}
	} else {
		// Use existing stream
		stream = (z_streamp) request->outputFilterData;
	}

	stream->avail_in = stream->avail_out = chunk->length;
	stream->next_in = chunk->value;
	stream->next_out = out;

	// Compress data
	deflate(stream, Z_SYNC_FLUSH);

	output.value = out;
	output.length = chunk->length - stream->avail_out;

	PancakeDebug {
		PancakeLoggerFormat(PANCAKE_LOGGER_SYSTEM, 0, "%lu bytes deflated to %lu bytes (%.2f%%)", chunk->length, output.length, (double) output.length / chunk->length * 100);
	}

	// Make chunk out of compressed data
	PancakeHTTPOutputChunk(sock, &output);

	return 1;
}

#endif
