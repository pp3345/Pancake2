
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

void PancakeHTTPRewriteRegisterDefaultVariables() {
	PancakeHTTPRewriteRegisterVariable(StaticString("$HTTPKeepAlive"), PANCAKE_HTTP_REWRITE_BOOL, 0, NULL, PancakeHTTPRewriteGetHTTPKeepAlive, PancakeHTTPRewriteSetHTTPKeepAlive);
}

#endif
