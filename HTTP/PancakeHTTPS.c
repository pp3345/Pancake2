#include "PancakeHTTP.h"

#if defined(PANCAKE_HTTP) && defined(PANCAKE_NETWORK_TLS)

static UByte PancakeHTTPSInitialize(PancakeSocket *sock);

static PancakeNetworkTLSApplicationProtocol PancakeHTTPS = {
		StaticString("HTTPS"),

		NULL,
		NULL,
		NULL,
		PancakeHTTPSInitialize
};

static UByte PancakeHTTPSInitialize(PancakeSocket *sock) {
	sock->flags |= PANCAKE_HTTPS;

	return 1;
}

void PancakeHTTPSRegisterProtocol() {
	PancakeNetworkTLSRegisterApplicationProtocol(&PancakeHTTPS);
}

#endif
