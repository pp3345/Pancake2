
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

static void PancakeHTTPStaticWrite(PancakeSocket *sock) {
	if(!sock->writeBuffer.length) {
		PancakeHTTPRequest *request = (PancakeHTTPRequest*) sock->data;
		UByte buf[8192];
		String output;

		if(feof((FILE*) request->contentServeData)) {
			fclose(request->contentServeData);
			PancakeHTTPOnRemoteHangup(sock);
			return;
		}

		output.value = buf;
		output.length = fread(buf, 1, 8192, request->contentServeData);

		PancakeHTTPOutput(sock, &output);
	}

	PancakeNetworkWrite(sock);
}

static void PancakeHTTPStaticOnRemoteHangup(PancakeSocket *sock) {
	PancakeHTTPRequest *request = (PancakeHTTPRequest*) sock->data;

	fclose(request->contentServeData);
	PancakeHTTPOnRemoteHangup(sock);
}

static UByte PancakeHTTPServeStatic(PancakeSocket *sock) {
	PancakeHTTPRequest *request = (PancakeHTTPRequest*) sock->data;

	if(PancakeHTTPRunAccessChecks(sock) && S_ISREG(request->fileStat.st_mode)) {
		// File is OK, serve it
		UByte fullPath[PancakeHTTPConfiguration.documentRoot->length + request->path.length + 1];

		memcpy(fullPath, PancakeHTTPConfiguration.documentRoot->value, PancakeHTTPConfiguration.documentRoot->length);
		memcpy(fullPath + PancakeHTTPConfiguration.documentRoot->length, request->path.value, request->path.length);

		// null-terminate string
		fullPath[PancakeHTTPConfiguration.documentRoot->length + request->path.length] = '\0';

		request->contentLength = request->fileStat.st_size;
		request->answerType = PancakeMIMELookupTypeByPath(&request->path);
		request->answerCode = 200;

		// Open file
		request->contentServeData = (void*) fopen(fullPath, "r");

		if(request->contentServeData == NULL) {
			return 0;
		}

		PancakeHTTPBuildAnswerHeaders(sock);
		PancakeNetworkAddWriteSocket(sock);

		sock->onWrite = PancakeHTTPStaticWrite;
		sock->onRemoteHangup = PancakeHTTPStaticOnRemoteHangup;
		return 1;
	}

	return 0;
}

#endif
