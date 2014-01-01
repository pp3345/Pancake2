
#include "PancakeHTTPRewriteDefaultVariables.h"

#ifdef PANCAKE_HTTP_REWRITE

static UByte PancakeHTTPRewriteGetHTTPKeepAlive(PancakeSocket *sock, PancakeHTTPRewriteVariable *var, PancakeHTTPRewriteValue *value) {
	PancakeHTTPRequest *request = (PancakeHTTPRequest*) sock->data;

	value->boolv = request->keepAlive;
	return 1;
}

static UByte PancakeHTTPRewriteSetHTTPKeepAlive(PancakeSocket *sock, PancakeHTTPRewriteVariable *var, PancakeHTTPRewriteValue *value) {
	PancakeHTTPRequest *request = (PancakeHTTPRequest*) sock->data;

	request->keepAlive = value->boolv;
	return 1;
}

static Byte PancakeHTTPRewriteThrowException(PancakeSocket *sock) {
	PancakeHTTPRequest *request = (PancakeHTTPRequest*) sock->data;

	PancakeHTTPException(sock, request->answerCode >= 100 && request->answerCode <= 599 ? request->answerCode : 500);

	/* Stop parser */
	return -1;
}

void PancakeHTTPRewriteRegisterDefaultVariables() {
	PancakeHTTPRewriteRegisterVariable(StaticString("$HTTPKeepAlive"), PANCAKE_HTTP_REWRITE_BOOL, 0, NULL, PancakeHTTPRewriteGetHTTPKeepAlive, PancakeHTTPRewriteSetHTTPKeepAlive);

	PancakeHTTPRewriteRegisterCallback(StaticString("ThrowException"), PancakeHTTPRewriteThrowException);
}

#endif
