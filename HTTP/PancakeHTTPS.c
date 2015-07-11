#include "PancakeHTTP.h"

#ifdef PANCAKE_NETWORK_TLS

STATIC UByte PancakeHTTPSInitialize(PancakeSocket *sock);
STATIC String *PancakeHTTPSNextProtocolNegotiation(PancakeSocket *sock);

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

STATIC UByte PancakeHTTPSInitialize(PancakeSocket *sock) {
	sock->flags |= PANCAKE_HTTPS;

	return 1;
}

STATIC String *PancakeHTTPSNextProtocolNegotiation(PancakeSocket *sock) {
	return &http11;
}

void PancakeHTTPSRegisterProtocol() {
	PancakeNetworkTLSRegisterApplicationProtocol(&PancakeHTTPS);
}

#endif
