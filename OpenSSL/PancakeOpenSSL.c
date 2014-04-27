
#include "PancakeOpenSSL.h"

#ifdef PANCAKE_OPENSSL

#include "PancakeLogger.h"

PancakeModule PancakeOpenSSL = {
	"OpenSSL",

	PancakeOpenSSLInitialize,
	NULL,
	PancakeOpenSSLShutdown,

	0
};

static void PancakeOpenSSLConfigure(PancakeConfigurationGroup *parent, UByte mode);
#ifdef TLSEXT_TYPE_next_proto_neg
static Int32 PancakeOpenSSLNextProtocolNegotiation(SSL *ssl, const UByte **output, UInt32 *outputLength, void *arg);
#endif
#ifdef SSL_CTRL_SET_TLSEXT_HOSTNAME
static Int32 PancakeOpenSSLServerNameIndication(SSL *ssl, int *ad, void *arg);
#endif
static UByte PancakeOpenSSLAcceptConnection(PancakeSocket **socket, PancakeSocket *parent);
static Int32 PancakeOpenSSLRead(PancakeSocket *socket, UInt32 maxLength, UByte *buf);
static Int32 PancakeOpenSSLWrite(PancakeSocket *socket);
static void PancakeOpenSSLClose(PancakeSocket *socket);

static PancakeNetworkLayer PancakeOpenSSLNetworkLayer = {
	StaticString("OpenSSL"),

	PancakeOpenSSLConfigure,
	PancakeOpenSSLAcceptConnection,
	PancakeOpenSSLRead,
	PancakeOpenSSLWrite,
	PancakeOpenSSLClose,

	NULL
};

UByte PancakeOpenSSLInitialize() {
	PancakeNetworkRegisterNetworkLayer(&PancakeOpenSSLNetworkLayer);

	// Initialize OpenSSL library
	SSL_library_init();
	SSL_load_error_strings();
	OpenSSL_add_all_algorithms();

	return 1;
}

UByte PancakeOpenSSLShutdown() {
	CONF_modules_free();
	ERR_remove_state(0);
	ENGINE_cleanup();
	CONF_modules_unload(1);
	ERR_free_strings();
	EVP_cleanup();
	CRYPTO_cleanup_all_ex_data();
	sk_SSL_COMP_free(SSL_COMP_get_compression_methods());

	return 1;
}

static UByte PancakeOpenSSLApplicationProtocolConfiguration(UByte step, config_setting_t *setting, PancakeConfigurationScope **scope) {
	PancakeNetworkTLSApplicationProtocol *protocol;
	String name;
	PancakeOpenSSLServerSocket *sock = (PancakeOpenSSLServerSocket*) setting->parent->parent->hook;

	// Fetch module
	name.value = setting->value.sval;
	name.length = strlen(setting->value.sval);

	protocol = PancakeNetworkTLSGetApplicationProtocol(&name);

	if(protocol == NULL) {
		PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Unknown TLS application protocol");
		return 0;
	}

	setting->parent->hook = (void*) protocol;

	if(sock->defaultContext) {
		PancakeOpenSSLServerContext *context, *tmp;

		SSL_CTX_set_ex_data(sock->defaultContext, 0, (void*) protocol);

		// Contexts are already configured, add application protocol data
		HASH_ITER(hh, sock->contexts, context, tmp) {
			SSL_CTX_set_ex_data(context->context, 0, (void*) protocol);
		}
	}

	return 1;
}

static UByte PancakeOpenSSLServerConfiguration(UByte step, config_setting_t *setting, PancakeConfigurationScope **scope) {
	PancakeOpenSSLServerSocket *sock = (PancakeOpenSSLServerSocket*) setting->parent->hook;

	if(step == PANCAKE_CONFIGURATION_INIT) {
		sock = PancakeReallocate(sock, sizeof(PancakeOpenSSLServerSocket));
		sock->contexts = NULL;
		sock->defaultContext = NULL;

		// Address of PancakeSocket might have changed
		PancakeNetworkReplaceListenSocket((PancakeSocket*) setting->parent->hook, (PancakeSocket*) sock);
		setting->parent->hook = (void*) sock;
		setting->hook = NULL; // Will be used for application protocol setting
	} else {
		PancakeOpenSSLServerContext *domain, *tmp;

		// Free domain context references
		HASH_ITER(hh, sock->contexts, domain, tmp) {
			HASH_DEL(sock->contexts, domain);
			PancakeFree(domain);
		}
	}

	return 1;
}

static UByte PancakeOpenSSLServerContextConfiguration(UByte step, config_setting_t *setting, PancakeConfigurationScope **scope) {
	SSL_CTX *ctx;
	PancakeOpenSSLServerSocket *sock = (PancakeOpenSSLServerSocket*) setting->parent->parent->parent->hook;

	if(step == PANCAKE_CONFIGURATION_INIT) {
		// Taken from openssl/apps/dh1024.pem
		static unsigned char dh1024_p[] = {
			0xF4,0x88,0xFD,0x58,0x4E,0x49,0xDB,0xCD,0x20,0xB4,0x9D,0xE4,
			0x91,0x07,0x36,0x6B,0x33,0x6C,0x38,0x0D,0x45,0x1D,0x0F,0x7C,
			0x88,0xB3,0x1C,0x7C,0x5B,0x2D,0x8E,0xF6,0xF3,0xC9,0x23,0xC0,
			0x43,0xF0,0xA5,0x5B,0x18,0x8D,0x8E,0xBB,0x55,0x8C,0xB8,0x5D,
			0x38,0xD3,0x34,0xFD,0x7C,0x17,0x57,0x43,0xA3,0x1D,0x18,0x6C,
			0xDE,0x33,0x21,0x2C,0xB5,0x2A,0xFF,0x3C,0xE1,0xB1,0x29,0x40,
			0x18,0x11,0x8D,0x7C,0x84,0xA7,0x0A,0x72,0xD6,0x86,0xC4,0x03,
			0x19,0xC8,0x07,0x29,0x7A,0xCA,0x95,0x0C,0xD9,0x96,0x9F,0xAB,
			0xD0,0x0A,0x50,0x9B,0x02,0x46,0xD3,0x08,0x3D,0x66,0xA4,0x5D,
			0x41,0x9F,0x9C,0x7C,0xBD,0x89,0x4B,0x22,0x19,0x26,0xBA,0xAB,
			0xA2,0x5E,0xC3,0x55,0xE9,0x2F,0x78,0xC7
		};
		static unsigned char dh1024_g[] = {0x02};
		DH *params;
		EC_KEY *curve;

		// Create OpenSSL context
		ctx = SSL_CTX_new(SSLv23_server_method());

		if(ctx == NULL) {
			PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Failed to create OpenSSL context: %s", ERR_error_string(ERR_get_error(), NULL));
			return 0;
		}

		if(!sock->defaultContext) {
			sock->defaultContext = ctx;
		} else {
#ifdef SSL_CTRL_SET_TLSEXT_HOSTNAME
			if(!SSL_CTX_set_tlsext_servername_callback(sock->defaultContext, PancakeOpenSSLServerNameIndication)) {
				PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Server Name Indication is not supported by your OpenSSL build");
				return 0;
			}

			SSL_CTX_set_ex_data(sock->defaultContext, 1, (void*) sock);
#else
			PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Server Name Indication is not supported by your OpenSSL build");
			return 0;
#endif
		}

		// NPN
#ifdef TLSEXT_TYPE_next_proto_neg
		SSL_CTX_set_next_protos_advertised_cb(ctx, PancakeOpenSSLNextProtocolNegotiation, NULL);
#endif

		// Set default DH parameters
		params = DH_new();
		params->p = BN_bin2bn(dh1024_p, sizeof(dh1024_p), NULL);
		params->g = BN_bin2bn(dh1024_g, sizeof(dh1024_g), NULL);

		if(params->p == NULL || params->g == NULL) {
			DH_free(params);
			PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Failed to set DH parameters: %s", ERR_error_string(ERR_get_error(), NULL));
			return 0;
		}

		SSL_CTX_set_tmp_dh(ctx, params);

		DH_free(params);

		// Set curve for ECDHE
		curve = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);

		if(curve == NULL) {
			PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Failed to get prime256v1 elliptic curve: %s", ERR_error_string(ERR_get_error(), NULL));
			return 0;
		}

		SSL_CTX_set_tmp_ecdh(ctx, curve);
		EC_KEY_free(curve);

		setting->hook = (void*) ctx;

		// Application protocol setting
		if(setting->parent->parent->hook) {
			SSL_CTX_set_ex_data(ctx, 0, (void*) setting->parent->parent->hook);
		}

		// Enable quiet shutdown
		//SSL_CTX_set_quiet_shutdown(ctx, 1);
	} else {
		ctx = (SSL_CTX*) setting->hook;

		SSL_CTX_free(ctx);
	}

	return 1;
}

static UByte PancakeOpenSSLServerContextServerCipherPreferenceConfiguration(UByte step, config_setting_t *setting, PancakeConfigurationScope **scope) {
	SSL_CTX *ctx = (SSL_CTX*) setting->parent->hook;

	if(step == PANCAKE_CONFIGURATION_INIT && setting->value.ival == 1) {
		// Enable server cipher preference
		SSL_CTX_set_options(ctx, SSL_OP_CIPHER_SERVER_PREFERENCE);
	}

	return 1;
}

static UByte PancakeOpenSSLServerContextDomainsConfiguration(UByte step, config_setting_t *setting, PancakeConfigurationScope **scope) {
	SSL_CTX *ctx = (SSL_CTX*) setting->parent->hook;
	PancakeOpenSSLServerSocket *sock = (PancakeOpenSSLServerSocket*) setting->parent->parent->parent->parent->hook;

	if(step == PANCAKE_CONFIGURATION_INIT) {
		config_setting_t *element;
		UInt16 i = 0;

		while(element = config_setting_get_elem(setting, i++)) {
			PancakeOpenSSLServerContext *old = NULL, *new;
			UByte length;

			if(element->type != CONFIG_TYPE_STRING) {
				PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Domain must be a string");
				return 0;
			}

			length = strlen(element->value.sval);

			// Check if domain already exists
			HASH_FIND(hh, sock->contexts, element->value.sval, length, old);

			if(old != NULL) {
				PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Multiple entries for domain \"%s\"", element->value.sval);
				return 0;
			}

			// Add context domain to hash table
			new = PancakeAllocate(sizeof(PancakeOpenSSLServerContext));
			new->context = ctx;

			HASH_ADD_KEYPTR(hh, sock->contexts, element->value.sval, length, new);
		}
	}

	return 1;
}

static UByte PancakeOpenSSLServerContextCertificateChainConfiguration(UByte step, config_setting_t *setting, PancakeConfigurationScope **scope) {
	SSL_CTX *ctx = (SSL_CTX*) setting->parent->hook;

	if(step == PANCAKE_CONFIGURATION_INIT) {
		if(SSL_CTX_use_certificate_chain_file(ctx, setting->value.sval) != 1) {
			PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Failed to set certificate chain file \"%s\": %s", setting->value.sval, ERR_error_string(ERR_get_error(), NULL));
			return 0;
		}

		// Free some memory
		free(setting->value.sval);
		setting->type = CONFIG_TYPE_NONE;
	}

	return 1;
}

static UByte PancakeOpenSSLServerContextPrivateKeyConfiguration(UByte step, config_setting_t *setting, PancakeConfigurationScope **scope) {
	SSL_CTX *ctx = (SSL_CTX*) setting->parent->hook;

	if(step == PANCAKE_CONFIGURATION_INIT) {
		if(SSL_CTX_use_PrivateKey_file(ctx, setting->value.sval, X509_FILETYPE_PEM) != 1) {
			PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Failed to set private key file \"%s\": %s", setting->value.sval, ERR_error_string(ERR_get_error(), NULL));
			return 0;
		}

		// Free some memory
		free(setting->value.sval);
		setting->type = CONFIG_TYPE_NONE;
	}

	return 1;
}

static UByte PancakeOpenSSLServerContextCiphersConfiguration(UByte step, config_setting_t *setting, PancakeConfigurationScope **scope) {
	SSL_CTX *ctx = (SSL_CTX*) setting->parent->hook;

	if(step == PANCAKE_CONFIGURATION_INIT) {
		if(SSL_CTX_set_cipher_list(ctx, setting->value.sval) != 1) {
			PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Failed to set cipher list: %s", ERR_error_string(ERR_get_error(), NULL));
			return 0;
		}

		// Free some memory
		free(setting->value.sval);
		setting->type = CONFIG_TYPE_NONE;
	}

	return 1;
}

static UByte PancakeOpenSSLServerContextDiffieHellmanParameterConfiguration(UByte step, config_setting_t *setting, PancakeConfigurationScope **scope) {
	SSL_CTX *ctx = (SSL_CTX*) setting->parent->hook;

	if(step == PANCAKE_CONFIGURATION_INIT) {
		FILE *file;
		DH *params;

		file = fopen(setting->value.sval, "r");

		if(file == NULL) {
			PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Failed to open %s: %s", setting->value.sval, strerror(errno));
			return 0;
		}

		params = PEM_read_DHparams(file, NULL, NULL, NULL);

		if(params == NULL) {
			fclose(file);
			PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Failed to read DH parameters from %s: %s", setting->value.sval, ERR_error_string(ERR_get_error(), NULL));
			return 0;
		}

		fclose(file);

		if(!SSL_CTX_set_tmp_dh(ctx, params)) {
			PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Failed to set DH parameters from %s: %s", setting->value.sval, ERR_error_string(ERR_get_error(), NULL));
			return 0;
		}

		DH_free(params);
	}

	return 1;
}

static UByte PancakeOpenSSLServerContextEllipticCurveConfiguration(UByte step, config_setting_t *setting, PancakeConfigurationScope **scope) {
	SSL_CTX *ctx = (SSL_CTX*) setting->parent->hook;

	if(step == PANCAKE_CONFIGURATION_INIT) {
		Int32 nid;
		EC_KEY *curve;

		nid = OBJ_sn2nid(setting->value.sval);

		if(!nid) {
			PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Unknown elliptic curve \"%s\"", setting->value.sval);
			return 0;
		}

		curve = EC_KEY_new_by_curve_name(nid);

		if(curve == NULL) {
			PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Failed to get \"%s\" elliptic curve: %s", setting->value.sval, ERR_error_string(ERR_get_error(), NULL));
			return 0;
		}

		SSL_CTX_set_tmp_ecdh(ctx, curve);
		EC_KEY_free(curve);

		// Free some memory
		free(setting->value.sval);
		setting->type = CONFIG_TYPE_NONE;
	}

	return 1;
}

static void PancakeOpenSSLConfigure(PancakeConfigurationGroup *parent, UByte mode) {
	if(mode == PANCAKE_NETWORK_LAYER_MODE_SERVER) {
		PancakeConfigurationGroup *OpenSSL, *ContextGroup;
		PancakeConfigurationSetting *Contexts;

		OpenSSL = PancakeConfigurationAddGroup(parent, StaticString("OpenSSL"), PancakeOpenSSLServerConfiguration);
		PancakeConfigurationAddSetting(OpenSSL, StaticString("ApplicationProtocol"), CONFIG_TYPE_STRING, NULL, 0, (config_value_t) 0, PancakeOpenSSLApplicationProtocolConfiguration);
		Contexts = PancakeConfigurationAddSetting(OpenSSL, StaticString("Contexts"), CONFIG_TYPE_LIST, NULL, 0, (config_value_t) 0, NULL);
		ContextGroup = PancakeConfigurationListGroup(Contexts, PancakeOpenSSLServerContextConfiguration);

		PancakeConfigurationAddSetting(ContextGroup, StaticString("Domains"), CONFIG_TYPE_LIST, NULL, 0, (config_value_t) 0, PancakeOpenSSLServerContextDomainsConfiguration);
		PancakeConfigurationAddSetting(ContextGroup, StaticString("CertificateChain"), CONFIG_TYPE_STRING, NULL, 0, (config_value_t) 0, PancakeOpenSSLServerContextCertificateChainConfiguration);
		PancakeConfigurationAddSetting(ContextGroup, StaticString("PrivateKey"), CONFIG_TYPE_STRING, NULL, 0, (config_value_t) 0, PancakeOpenSSLServerContextPrivateKeyConfiguration);
		PancakeConfigurationAddSetting(ContextGroup, StaticString("Ciphers"), CONFIG_TYPE_STRING, NULL, 0, (config_value_t) 0, PancakeOpenSSLServerContextCiphersConfiguration);
		PancakeConfigurationAddSetting(ContextGroup, StaticString("PreferServerCiphers"), CONFIG_TYPE_BOOL, NULL, 0, (config_value_t) 0, PancakeOpenSSLServerContextServerCipherPreferenceConfiguration);
		PancakeConfigurationAddSetting(ContextGroup, StaticString("DiffieHellmanParameters"), CONFIG_TYPE_STRING, NULL, 0, (config_value_t) 0, PancakeOpenSSLServerContextDiffieHellmanParameterConfiguration);
		PancakeConfigurationAddSetting(ContextGroup, StaticString("EllipticCurve"), CONFIG_TYPE_STRING, NULL, 0, (config_value_t) 0, PancakeOpenSSLServerContextEllipticCurveConfiguration);
	}
}

#ifdef SSL_CTRL_SET_TLSEXT_HOSTNAME
static Int32 PancakeOpenSSLServerNameIndication(SSL *ssl, int *ad, void *arg) {
	const UByte *name = SSL_get_servername(ssl, TLSEXT_NAMETYPE_host_name);
	PancakeOpenSSLServerSocket *sock;
	PancakeOpenSSLServerContext *context;
	UByte length;

	if(name == NULL || !(*name)) {
		// No name given
		return SSL_TLSEXT_ERR_NOACK;
	}

	sock = (PancakeOpenSSLServerSocket*) SSL_CTX_get_ex_data(ssl->ctx, 1);
	length = strlen(name);

	HASH_FIND(hh, sock->contexts, name, length, context);

	if(context == NULL) {
		// Unknown domain
		return SSL_TLSEXT_ERR_NOACK;
	}

	SSL_set_SSL_CTX(ssl, context->context);

	return SSL_TLSEXT_ERR_OK;
}
#endif

#ifdef TLSEXT_TYPE_next_proto_neg
static Int32 PancakeOpenSSLNextProtocolNegotiation(SSL *ssl, const UByte **output, UInt32 *outputLength, void *arg) {
	PancakeSocket *sock;
	PancakeNetworkTLSApplicationProtocol *protocol;
	String *result;

	protocol = (PancakeNetworkTLSApplicationProtocol*) SSL_CTX_get_ex_data(ssl->ctx, 0);

	if(!protocol || !protocol->NPN) {
		// No application protocol configured or application protocol does not support NPN
		return SSL_TLSEXT_ERR_OK;
	}

	sock = (PancakeSocket*) SSL_get_ex_data(ssl, 0);

	result = protocol->NPN(sock);

	*output = result->value;
	*outputLength = result->length;

	return SSL_TLSEXT_ERR_OK;
}
#endif

static UByte PancakeOpenSSLAcceptConnection(PancakeSocket **socket, PancakeSocket *parent) {
	PancakeOpenSSLSocket *sock = (PancakeOpenSSLSocket*) *socket;
	PancakeOpenSSLServerSocket *server = (PancakeOpenSSLServerSocket*) parent;
	PancakeNetworkTLSApplicationProtocol *protocol;

	sock = PancakeReallocate(sock, sizeof(PancakeOpenSSLSocket));
	sock->session = SSL_new(server->defaultContext);
	sock->previousHandler = NULL;

	if(UNEXPECTED(sock->session == NULL)) {
		return 0;
	}

	if(UNEXPECTED(!SSL_set_fd(sock->session, sock->socket.fd))) {
		SSL_free(sock->session);
		return 0;
	}

	SSL_set_accept_state(sock->session);

	*socket = (PancakeSocket*) sock;

	protocol = (PancakeNetworkTLSApplicationProtocol*) SSL_CTX_get_ex_data(server->defaultContext, 0);

	if(protocol != NULL) {
		if(protocol->initialize && UNEXPECTED(!protocol->initialize((PancakeSocket*) sock))) {
			SSL_free(sock->session);
			return 0;
		}

		SSL_set_ex_data(sock->session, 0, (PancakeSocket*) sock);
	}

	return 1;
}

static Int32 PancakeOpenSSLRead(PancakeSocket *socket, UInt32 maxLength, UByte *buf) {
	PancakeOpenSSLSocket *sock = (PancakeOpenSSLSocket*) socket;
	Int32 length;

	length = SSL_read(sock->session, buf, maxLength);

	if(length > 0) {
		if(sock->previousHandler) {
			socket->onWrite = sock->previousHandler;
		}

		return length;
	} else { // <= 0
		switch(SSL_get_error(sock->session, length)) {
			case SSL_ERROR_ZERO_RETURN:
				// SSL connection shut down, will handle close in onRemoteHangup event
			case SSL_ERROR_WANT_READ:
				return 0;
			case SSL_ERROR_SYSCALL:
				if(!errno) {
					// No data was available to read
					return 0;
				}
				return -1;
			case SSL_ERROR_WANT_WRITE:
				PancakeNetworkAddWriteSocket(socket);

				sock->previousHandler = socket->onWrite;
				socket->onWrite = socket->onRead;
				return 0;
			default:
				SSL_free(sock->session);
				sock->session = NULL;

				return -1;
		}
	}
}

static Int32 PancakeOpenSSLWrite(PancakeSocket *socket) {
	PancakeOpenSSLSocket *sock = (PancakeOpenSSLSocket*) socket;
	Int32 length;

	length = SSL_write(sock->session, socket->writeBuffer.value, socket->writeBuffer.length);

	if(length > 0) {
		if(sock->previousHandler) {
			socket->onRead = sock->previousHandler;
		}

		return length;
	} else { // <= 0
		switch(SSL_get_error(sock->session, length)) {
			case SSL_ERROR_WANT_READ:
				PancakeNetworkAddReadSocket(socket);
				sock->previousHandler = socket->onRead;
				socket->onRead = socket->onWrite;

				return 0;
			case SSL_ERROR_ZERO_RETURN:
				// SSL connection shut down, will handle close in onRemoteHangup event
			case SSL_ERROR_WANT_WRITE:
				return 0;
			case SSL_ERROR_SYSCALL:
				if(!errno) {
					// No data was available to read
					return 0;
				}
				return -1;
			default:
				SSL_free(sock->session);
				sock->session = NULL;

				return -1;
		}
	}
}

static void PancakeOpenSSLClose(PancakeSocket *socket) {
	PancakeOpenSSLSocket *sock = (PancakeOpenSSLSocket*) socket;

	if(sock->session) {
		SSL_shutdown(sock->session);
		SSL_free(sock->session);
	}
}

#endif
