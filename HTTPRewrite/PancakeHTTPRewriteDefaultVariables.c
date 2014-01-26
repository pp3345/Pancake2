
#include "PancakeHTTPRewriteDefaultVariables.h"

#ifdef PANCAKE_HTTP_REWRITE

#include "PancakeLogger.h"

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

static UByte PancakeHTTPRewriteGetHTTPAnswerCode(PancakeSocket *sock, PancakeHTTPRewriteVariable *var, PancakeHTTPRewriteValue *value) {
	PancakeHTTPRequest *request = (PancakeHTTPRequest*) sock->data;

	value->intv = (Int32) request->answerCode;
	return 1;
}

static UByte PancakeHTTPRewriteSetHTTPAnswerCode(PancakeSocket *sock, PancakeHTTPRewriteVariable *var, PancakeHTTPRewriteValue *value) {
	PancakeHTTPRequest *request = (PancakeHTTPRequest*) sock->data;

	if(UNEXPECTED(value->intv < 100 || value->intv > 599)) {
		PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Value for $HTTPAnswerCode is out of range (100 - 599)");
		return 0;
	}

	request->answerCode = (UInt16) value->intv;
	return 1;
}

static UByte PancakeHTTPRewriteGetHTTPClientContentLength(PancakeSocket *sock, PancakeHTTPRewriteVariable *var, PancakeHTTPRewriteValue *value) {
	PancakeHTTPRequest *request = (PancakeHTTPRequest*) sock->data;

	value->intv = (Int32) request->clientContentLength;
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
	PancakeHTTPRewriteRegisterVariable(StaticString("$HTTPAnswerCode"), PANCAKE_HTTP_REWRITE_INT, 0, NULL, PancakeHTTPRewriteGetHTTPAnswerCode, PancakeHTTPRewriteSetHTTPAnswerCode);
	PancakeHTTPRewriteRegisterVariable(StaticString("$HTTPClientContentLength"), PANCAKE_HTTP_REWRITE_INT, PANCAKE_HTTP_REWRITE_READ_ONLY, NULL, PancakeHTTPRewriteGetHTTPClientContentLength, NULL);

	PancakeHTTPRewriteRegisterCallback(StaticString("ThrowException"), PancakeHTTPRewriteThrowException);
}

#endif
