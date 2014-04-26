#include "PancakeHTTP.h"

#if defined(PANCAKE_HTTP) && defined(PANCAKE_NETWORK_TLS)

static UByte PancakeHTTPSInitialize(PancakeSocket *sock);
static String *PancakeHTTPSNextProtocolNegotiation(PancakeSocket *sock);

static PancakeNetworkTLSApplicationProtocol PancakeHTTPS = {
		StaticString("HTTPS"),

		NULL,
		PancakeHTTPSNextProtocolNegotiation,
		NULL,
		PancakeHTTPSInitialize
};

static String http11 = {
		"\x08http/1.1",
		sizeof("\x08http/1.1") - 1
};

static UByte PancakeHTTPSInitialize(PancakeSocket *sock) {
	sock->flags |= PANCAKE_HTTPS;

	return 1;
}

static String *PancakeHTTPSNextProtocolNegotiation(PancakeSocket *sock) {
	return &http11;
}

void PancakeHTTPSRegisterProtocol() {
	PancakeNetworkTLSRegisterApplicationProtocol(&PancakeHTTPS);
}

#endif
