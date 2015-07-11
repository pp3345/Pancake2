#ifndef _PANCAKE_OPENSSL_H
#define _PANCAKE_OPENSSL_H

#include "../Pancake.h"
#include "../PancakeNetwork.h"
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/conf.h>
#include <openssl/engine.h>
#include <openssl/pem.h>

extern PancakeModule PancakeOpenSSL;
UByte PancakeOpenSSLInitialize();
UByte PancakeOpenSSLShutdown();

typedef struct _PancakeOpenSSLServerContext {
	SSL_CTX *context;

	UT_hash_handle hh;
} PancakeOpenSSLServerContext;

typedef struct _PancakeOpenSSLServerSocket {
	PancakeSocket socket;

	SSL_CTX *defaultContext;
	PancakeOpenSSLServerContext *contexts;
} PancakeOpenSSLServerSocket;

typedef struct _PancakeOpenSSLSocket {
	PancakeSocket socket;

	SSL *session;
	PancakeNetworkEventHandler previousHandler;
} PancakeOpenSSLSocket;

#endif
