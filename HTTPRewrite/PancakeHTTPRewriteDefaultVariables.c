
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

static UByte PancakeHTTPRewriteGetHTTPVersion(PancakeSocket *sock, PancakeHTTPRewriteVariable *var, PancakeHTTPRewriteValue *value) {
	PancakeHTTPRequest *request = (PancakeHTTPRequest*) sock->data;

	PancakeAssert(request->HTTPVersion == PANCAKE_HTTP_10 || request->HTTPVersion == PANCAKE_HTTP_11);

	value->intv = request->HTTPVersion == PANCAKE_HTTP_10 ? 10 : 11;
	return 1;
}

static UByte PancakeHTTPRewriteSetHTTPVersion(PancakeSocket *sock, PancakeHTTPRewriteVariable *var, PancakeHTTPRewriteValue *value) {
	PancakeHTTPRequest *request = (PancakeHTTPRequest*) sock->data;

	if(UNEXPECTED(value->intv < 10 || value->intv > 11)) {
		PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Bad value for $HTTPVersion: must be 10 or 11");
		return 0;
	}

	if(UNEXPECTED(value->intv == 11 && request->HTTPVersion < PANCAKE_HTTP_11)) {
		PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Can't set $HTTPVersion to higher value than supported by client");
		return 0;
	}

	request->HTTPVersion = value->intv == 10 ? PANCAKE_HTTP_10 : PANCAKE_HTTP_11;

	return 1;
}

static UByte PancakeHTTPRewriteGetDocumentURI(PancakeSocket *sock, PancakeHTTPRewriteVariable *var, PancakeHTTPRewriteValue *value) {
	PancakeHTTPRequest *request = (PancakeHTTPRequest*) sock->data;

	value->stringv.value = request->path.value;
	value->stringv.length = request->path.length;

	return 1;
}

static UByte PancakeHTTPRewriteSetDocumentURI(PancakeSocket *sock, PancakeHTTPRewriteVariable *var, PancakeHTTPRewriteValue *value) {
	PancakeHTTPRequest *request = (PancakeHTTPRequest*) sock->data;

	request->path.value = value->stringv.value;
	request->path.length = value->stringv.length;

	return 1;
}

static UByte PancakeHTTPRewriteGetHTTPMethod(PancakeSocket *sock, PancakeHTTPRewriteVariable *var, PancakeHTTPRewriteValue *value) {
	PancakeHTTPRequest *request = (PancakeHTTPRequest*) sock->data;

	value->stringv = PancakeHTTPMethods[request->method - 1];

	return 1;
}

static UByte PancakeHTTPRewriteGetMIMEType(PancakeSocket *sock, PancakeHTTPRewriteVariable *var, PancakeHTTPRewriteValue *value) {
	PancakeHTTPRequest *request = (PancakeHTTPRequest*) sock->data;
	PancakeMIMEType *type;

	type = PancakeMIMELookupTypeByPath(&request->path);

	value->stringv = type->type;

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
	PancakeHTTPRewriteRegisterVariable(StaticString("$HTTPVersion"), PANCAKE_HTTP_REWRITE_INT, 0, NULL, PancakeHTTPRewriteGetHTTPVersion, PancakeHTTPRewriteSetHTTPVersion);
	PancakeHTTPRewriteRegisterVariable(StaticString("$HTTPMethod"), PANCAKE_HTTP_REWRITE_STRING, PANCAKE_HTTP_REWRITE_READ_ONLY, NULL, PancakeHTTPRewriteGetHTTPMethod, NULL);
	PancakeHTTPRewriteRegisterVariable(StaticString("$DocumentURI"), PANCAKE_HTTP_REWRITE_STRING, 0, NULL, PancakeHTTPRewriteGetDocumentURI, PancakeHTTPRewriteSetDocumentURI);
	PancakeHTTPRewriteRegisterVariable(StaticString("$MIMEType"), PANCAKE_HTTP_REWRITE_STRING, PANCAKE_HTTP_REWRITE_READ_ONLY, NULL, PancakeHTTPRewriteGetMIMEType, NULL);

	PancakeHTTPRewriteRegisterCallback(StaticString("ThrowException"), PancakeHTTPRewriteThrowException);
}

#endif
