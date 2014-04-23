
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

static UByte PancakeOpenSSLServerConfiguration(UByte step, config_setting_t *setting, PancakeConfigurationScope **scope) {
	PancakeOpenSSLServerSocket *sock = (PancakeOpenSSLServerSocket*) setting->parent->hook;

	if(step == PANCAKE_CONFIGURATION_INIT) {
		sock = PancakeReallocate(sock, sizeof(PancakeOpenSSLServerSocket));
		sock->contexts = NULL;
		sock->defaultContext = NULL;

		// Address of PancakeSocket might have changed
		PancakeNetworkReplaceListenSocket((PancakeSocket*) setting->parent->hook, (PancakeSocket*) sock);
		setting->parent->hook = (void*) sock;
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
		// Create OpenSSL context
		ctx = SSL_CTX_new(SSLv23_server_method());

		if(ctx == NULL) {
			PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Failed to create OpenSSL context: %s", ERR_error_string(ERR_get_error(), NULL));
			return 0;
		}

		if(!sock->defaultContext) {
			sock->defaultContext = ctx;
		}

		setting->hook = (void*) ctx;

		// Enable quiet shutdown
		//SSL_CTX_set_quiet_shutdown(ctx, 1);
	} else {
		ctx = (SSL_CTX*) setting->hook;

		SSL_CTX_free(ctx);
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

			if(element->type != CONFIG_TYPE_STRING) {
				PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Domain must be a string");
				return 0;
			}

			// Check if domain already exists
			HASH_FIND(hh, sock->contexts, element->value.sval, strlen(element->value.sval), old);

			if(old != NULL) {
				PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Multiple entries for domain \"%s\"", element->value.sval);
				return 0;
			}

			// Add context domain to hash table
			new = PancakeAllocate(sizeof(PancakeOpenSSLServerContext));
			new->context = ctx;

			HASH_ADD_KEYPTR(hh, sock->contexts, element->value.sval, strlen(element->value.sval), new);

			// Free some memory
			free(element->value.sval);
			element->type = CONFIG_TYPE_NONE;
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

static void PancakeOpenSSLConfigure(PancakeConfigurationGroup *parent, UByte mode) {
	if(mode == PANCAKE_NETWORK_LAYER_MODE_SERVER) {
		PancakeConfigurationGroup *OpenSSL, *ContextGroup;
		PancakeConfigurationSetting *Contexts;

		OpenSSL = PancakeConfigurationAddGroup(parent, StaticString("OpenSSL"), PancakeOpenSSLServerConfiguration);
		Contexts = PancakeConfigurationAddSetting(OpenSSL, StaticString("Contexts"), CONFIG_TYPE_LIST, NULL, 0, (config_value_t) 0, NULL);
		ContextGroup = PancakeConfigurationListGroup(Contexts, PancakeOpenSSLServerContextConfiguration);

		PancakeConfigurationAddSetting(ContextGroup, StaticString("Domains"), CONFIG_TYPE_LIST, NULL, 0, (config_value_t) 0, PancakeOpenSSLServerContextDomainsConfiguration);
		PancakeConfigurationAddSetting(ContextGroup, StaticString("CertificateChain"), CONFIG_TYPE_STRING, NULL, 0, (config_value_t) 0, PancakeOpenSSLServerContextCertificateChainConfiguration);
		PancakeConfigurationAddSetting(ContextGroup, StaticString("PrivateKey"), CONFIG_TYPE_STRING, NULL, 0, (config_value_t) 0, PancakeOpenSSLServerContextPrivateKeyConfiguration);
		PancakeConfigurationAddSetting(ContextGroup, StaticString("Ciphers"), CONFIG_TYPE_STRING, NULL, 0, (config_value_t) 0, PancakeOpenSSLServerContextCiphersConfiguration);
	}
}

static UByte PancakeOpenSSLAcceptConnection(PancakeSocket **socket, PancakeSocket *parent) {
	PancakeOpenSSLSocket *sock = (PancakeOpenSSLSocket*) *socket;
	PancakeOpenSSLServerSocket *server = (PancakeOpenSSLServerSocket*) parent;

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
