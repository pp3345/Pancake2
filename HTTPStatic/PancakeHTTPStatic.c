
#include "PancakeHTTPStatic.h"
#include "MIME/PancakeMIME.h"

#if defined(PANCAKE_HTTP_STATIC) && defined(PANCAKE_HTTP)

/* Forward declarations */
static UByte PancakeHTTPServeStatic(PancakeSocket *sock);
static UByte PancakeHTTPStaticInitialize();

PancakeModule PancakeHTTPStatic = {
	"HTTPStatic",

	PancakeHTTPStaticInitialize,
	NULL,
	NULL,

	0
};

static PancakeHTTPContentServeBackend PancakeHTTPStaticContent = {
	"Static",
	PancakeHTTPServeStatic
};

static UByte PancakeHTTPStaticInitialize() {
	if(!PancakeHTTP.intialized) {
		return 2;
	}

	PancakeHTTPRegisterContentServeBackend(&PancakeHTTPStaticContent);

	return 1;
}

static inline void PancakeHTTPStaticOnRequestEnd(PancakeHTTPRequest *request) {
	fclose(request->contentServeData);
}

static void PancakeHTTPStaticWrite(PancakeSocket *sock) {
	PancakeHTTPRequest *request = (PancakeHTTPRequest*) sock->data;

	if(sock->writeBuffer.size < PancakeMainConfiguration.networkBuffering) {
		sock->writeBuffer.size = PancakeMainConfiguration.networkBuffering;
		sock->writeBuffer.value = PancakeReallocate(sock->writeBuffer.value, PancakeMainConfiguration.networkBuffering);
	}

	if(sock->writeBuffer.length < PancakeMainConfiguration.networkBuffering) {
		UByte buf[sock->writeBuffer.size - sock->writeBuffer.length];
		String output;

		output.value = buf;
		output.length = fread(buf, 1, sock->writeBuffer.size - sock->writeBuffer.length, request->contentServeData);

		PancakeHTTPOutput(sock, &output);
	}

	if(feof((FILE*) request->contentServeData)) {
		if(sock->writeBuffer.length) {
			sock->onWrite = PancakeHTTPFullWriteBuffer;
			PancakeHTTPFullWriteBuffer(sock);
		} else {
			PancakeHTTPOnRequestEnd(sock);
		}

		return;
	}

	PancakeNetworkWrite(sock);
}

static UByte PancakeHTTPServeStatic(PancakeSocket *sock) {
	PancakeHTTPRequest *request = (PancakeHTTPRequest*) sock->data;

	if(PancakeHTTPRunAccessChecks(sock) && S_ISREG(request->fileStat.st_mode)) {
		// File is OK, serve it
		UByte fullPath[PancakeHTTPConfiguration.documentRoot->length + request->path.length + 1];

		if(request->ifModifiedSince) {
			UByte buf[29];

			PancakeRFC1123Date(request->fileStat.st_mtim.tv_sec, buf);
			if(!memcmp(request->ifModifiedSince, buf, 29)) {
				// File not modified
				request->answerCode = 304;

				PancakeHTTPBuildAnswerHeaders(sock);
				PancakeNetworkAddWriteSocket(sock);

				sock->onWrite = PancakeHTTPFullWriteBuffer;

				// Try to write now
				PancakeHTTPFullWriteBuffer(sock);
				return 1;
			}
		}

		memcpy(fullPath, PancakeHTTPConfiguration.documentRoot->value, PancakeHTTPConfiguration.documentRoot->length);
		memcpy(fullPath + PancakeHTTPConfiguration.documentRoot->length, request->path.value, request->path.length);

		// null-terminate string
		fullPath[PancakeHTTPConfiguration.documentRoot->length + request->path.length] = '\0';

		request->contentLength = request->fileStat.st_size;
		request->answerType = PancakeMIMELookupTypeByPath(&request->path);
		request->lastModified = request->fileStat.st_mtim.tv_sec;
		request->answerCode = 200;

		// Open file
		request->contentServeData = (void*) fopen(fullPath, "r");

		if(request->contentServeData == NULL) {
			PancakeHTTPException(sock, 403);
			return 0;
		}

		PancakeHTTPBuildAnswerHeaders(sock);
		PancakeNetworkAddWriteSocket(sock);

		sock->onWrite = PancakeHTTPStaticWrite;
		request->onRequestEnd = PancakeHTTPStaticOnRequestEnd;

		// Try to write now
		PancakeHTTPStaticWrite(sock);

		return 1;
	}

	return 0;
}

#endif
