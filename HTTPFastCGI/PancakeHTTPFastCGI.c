
#include "PancakeHTTPFastCGI.h"

#ifdef PANCAKE_HTTP_FASTCGI

#include "PancakeLogger.h"

#ifdef PANCAKE_HTTP_REWRITE
#include "HTTPRewrite/PancakeHTTPRewrite.h"
#endif

/* Forward declarations */
static UByte PancakeHTTPFastCGIInitialize();
static UByte PancakeHTTPFastCGIServe();
static void PancakeHTTPFastCGIOnRemoteHangup(PancakeSocket *sock);
static void PancakeHTTPFastCGIOnRead(PancakeSocket *sock);
static void PancakeHTTPFastCGIOnWrite(PancakeSocket *sock);

PancakeModule PancakeHTTPFastCGIModule = {
		"HTTPFastCGI",

		PancakeHTTPFastCGIInitialize,
		NULL,
		NULL,

		0
};

static PancakeHTTPContentServeBackend PancakeHTTPFastCGI = {
		"FastCGI",
		PancakeHTTPFastCGIServe,

		NULL
};

static PancakeFastCGIConfigurationStructure FastCGIConfiguration = {
		NULL
};

static PancakeFastCGIClient *PancakeFastCGIClients = NULL;

static UByte PancakeHTTPFastCGIConfiguration(UByte step, config_setting_t *setting, PancakeConfigurationScope **scope) {
	PancakeFastCGIClient *client;

	if(step == PANCAKE_CONFIGURATION_INIT) {
		// Allocate and initalize structure
		setting->hook = PancakeAllocate(sizeof(PancakeFastCGIClient));
		client = (PancakeFastCGIClient*) setting->hook;
		client->multiplex = -1;
		client->connectionCache = NULL;
		client->highestRequestID = 0;
		client->keepAlive = 0;

		memset(&client->requests, 0, PANCAKE_FASTCGI_MAX_REQUEST_ID);
		memset(&client->sockets, 0, PANCAKE_FASTCGI_MAX_REQUEST_ID);

		PancakeNetworkClientInterfaceConfiguration((PancakeNetworkClientInterface*) client);
	} else {
		client = (PancakeFastCGIClient*) setting->hook;

		PancakeDebug {
			PancakeLoggerFormat(PANCAKE_LOGGER_SYSTEM, 0, "Highest request ID used with FastCGI client %s: %i", client->name.value, client->highestRequestID);
		}

		PancakeFree(client);
	}

	return 1;
}

static UByte PancakeHTTPFastCGINameConfiguration(UByte step, config_setting_t *setting, PancakeConfigurationScope **scope) {
	PancakeFastCGIClient *client = (PancakeFastCGIClient*) setting->parent->hook;

	if(step == PANCAKE_CONFIGURATION_INIT) {
		client->name.value = setting->value.sval;
		client->name.length = strlen(setting->value.sval);

		HASH_ADD_KEYPTR(hh, PancakeFastCGIClients, client->name.value, client->name.length, client);
	} else {
		HASH_DEL(PancakeFastCGIClients, client);
	}

	return 1;
}

static UByte PancakeHTTPFastCGIClientConfiguration(UByte step, config_setting_t *setting, PancakeConfigurationScope **scope) {
	if(step == PANCAKE_CONFIGURATION_INIT) {
		UInt32 length = strlen(setting->value.sval);

#ifdef PANCAKE_HTTP_REWRITE
		PancakeHTTPRewriteConfigurationHook(step, setting, scope);
#endif

		if(length) {
			PancakeFastCGIClient *client = NULL;

			HASH_FIND(hh, PancakeFastCGIClients, setting->value.sval, length, client);

			if(client == NULL) {
				PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Unknown FastCGI client configuration %s", setting->value.sval);
				return 0;
			}

			free(setting->value.sval);

			setting->value.sval = (char*) client;
		} else {
			free(setting->value.sval);
			setting->value.sval = NULL;
		}

		setting->type = CONFIG_TYPE_SPECIAL;
	} else {
		// Make library happy
		setting->type = CONFIG_TYPE_NONE;
	}

	return 1;
}

static UByte PancakeHTTPFastCGIClientInterfaceConfiguration(UByte step, config_setting_t *setting, PancakeConfigurationScope **scope) {
	if(step == PANCAKE_CONFIGURATION_INIT) {
		setting->hook = setting->parent->hook;
	} else {
		// Make library happy
		setting->hook = NULL;
	}

	return 1;
}

static UByte PancakeHTTPFastCGIKeepAliveConfiguration(UByte step, config_setting_t *setting, PancakeConfigurationScope **scope) {
	PancakeFastCGIClient *client = (PancakeFastCGIClient*) setting->parent->hook;

	if(step == PANCAKE_CONFIGURATION_INIT) {
		PancakeAssert(!setting->value.ival || setting->value.ival == 1);

		client->keepAlive = setting->value.ival;
	}

	return 1;
}

static UByte PancakeHTTPFastCGIInitialize() {
	PancakeConfigurationSetting *FastCGIClients, *FastCGIClient, *VirtualHosts;
	PancakeConfigurationGroup *FastCGIGroup, *HTTP;

	if(!PancakeHTTP.initialized) {
		// Defer if HTTP module is not initialized yet
		return 2;
	}

#ifdef PANCAKE_HTTP_REWRITE
	if(!PancakeHTTPRewriteModule.initialized) {
		// Defer if HTTPRewrite is not initialized yet
		return 2;
	}
#endif

	// Register FastCGI content backend
	PancakeHTTPRegisterContentServeBackend(&PancakeHTTPFastCGI);

	FastCGIClients = PancakeConfigurationAddSetting(NULL, (String) {"FastCGIClients", sizeof("FastCGIClients") - 1}, CONFIG_TYPE_LIST, NULL, 0, (config_value_t) 0, NULL);
	FastCGIGroup = PancakeConfigurationListGroup(FastCGIClients, PancakeHTTPFastCGIConfiguration);
	PancakeConfigurationAddSetting(FastCGIGroup, (String) {"Name", sizeof("Name") - 1}, CONFIG_TYPE_STRING, NULL, 0, (config_value_t) 0, PancakeHTTPFastCGINameConfiguration);
	PancakeConfigurationAddSetting(FastCGIGroup, (String) {"KeepAlive", sizeof("KeepAlive") - 1}, CONFIG_TYPE_BOOL, NULL, 0, (config_value_t) 0, PancakeHTTPFastCGIKeepAliveConfiguration);
	PancakeNetworkRegisterClientInterfaceGroup(FastCGIGroup, PancakeHTTPFastCGIClientInterfaceConfiguration);

	HTTP = PancakeConfigurationLookupGroup(NULL, (String) {"HTTP", 4});
	FastCGIClient = PancakeConfigurationAddSetting(HTTP, (String) {"FastCGIClient", sizeof("FastCGIClient") - 1}, CONFIG_TYPE_STRING, &FastCGIConfiguration.client, sizeof(PancakeFastCGIClient*), (config_value_t) 0, PancakeHTTPFastCGIClientConfiguration);
	VirtualHosts = PancakeConfigurationLookupSetting(HTTP, (String) {"VirtualHosts", sizeof("VirtualHosts") - 1});
	PancakeConfigurationAddSettingToGroup(VirtualHosts->listGroup, FastCGIClient);

#ifdef PANCAKE_HTTP_REWRITE
	PancakeConfigurationAddSettingToGroup(PancakeHTTPRewriteGroup, FastCGIClient);
#endif

	return 1;
}

static void PancakeHTTPFastCGIWriteContentBody(PancakeSocket *socket, PancakeSocket *clientSocket, UInt16 requestID) {
	PancakeHTTPRequest *request = (PancakeHTTPRequest*) clientSocket->data;
	UInt32 length = clientSocket->readBuffer.length - 4 - request->headerEnd;
	UByte haveMoreData = 0;
	UByte *offset;

	// Max length of FCGI record content data is 65535 bytes
	if(length > 65535) {
		length = 65535;
		haveMoreData = 1;
	} else if(length == 0) {
		return;
	}

	clientSocket->readBuffer.length -= length;

	// Must be smaller than available content length
	if(UNEXPECTED(length > request->clientContentLength)) {
		return;
	}

	if(socket->writeBuffer.size < socket->writeBuffer.length + 16 + length) {
		socket->writeBuffer.size = socket->writeBuffer.length + 16 + length;
		socket->writeBuffer.value = PancakeReallocate(socket->writeBuffer.value, socket->writeBuffer.size);
	}

	offset = socket->writeBuffer.value + socket->writeBuffer.length;
	request->clientContentLength -= length;

	// Build FCGI header
	offset[0] = '\x1';
	offset[1] = '\x5';
#if PANCAKE_FASTCGI_MAX_REQUEST_ID > 255
	offset[2] = requestID >> 8;
#else
	offset[2] = 0;
#endif
	offset[3] = (UByte) requestID;
	offset[4] = length >> 8;
	offset[5] = (UByte) length;
	offset[6] = '\0';
	offset[7] = '\0';

	offset += 8;

	// Copy STDIN data to FCGI socket
	memcpy(offset, clientSocket->readBuffer.value + request->headerEnd + 4, length);

	// Add empty record to mark end of data
	if(!request->clientContentLength) {
		offset += length;
		offset[0] = '\x1';
		offset[1] = '\x5';
#if PANCAKE_FASTCGI_MAX_REQUEST_ID > 255
		offset[2] = requestID >> 8;
#else
		offset[2] = 0;
#endif
		offset[3] = (UByte) requestID;
		offset[4] = '\0';
		offset[5] = '\0';
		offset[6] = '\0';
		offset[7] = '\0';

		socket->writeBuffer.length += 8;
	}

	// Set buffer length
	socket->writeBuffer.length += 8 + length;

	// Try to write
	PancakeHTTPFastCGIOnWrite(socket);

	// Set to write mode if necessary
	if(socket->writeBuffer.length) {
		PancakeNetworkAddWriteSocket(socket);
	}

	// Send more data if available
	if(haveMoreData) {
		memmove(clientSocket->readBuffer.value + request->headerEnd + 4, clientSocket->readBuffer.value + request->headerEnd + 65539, clientSocket->readBuffer.length - 4 - request->headerEnd);
		PancakeHTTPFastCGIWriteContentBody(socket, clientSocket, requestID);
	}
}

static inline void FastCGIEncodeParameter(PancakeSocket *sock, String *name, String *value) {
	UByte *offset;
	UInt32 length = name->length + value->length + (name->length < 128 ? 1 : 4) + (value->length < 128 ? 1 : 4);

	// Resize buffer if required
	if(sock->writeBuffer.size < sock->writeBuffer.length + length) {
		sock->writeBuffer.size = sock->writeBuffer.length + length + 96;
		sock->writeBuffer.value = PancakeReallocate(sock->writeBuffer.value, sock->writeBuffer.size);
	}

	// Get offset to free space in buffer
	offset = sock->writeBuffer.value + sock->writeBuffer.length;

	// Encode name length
	if(EXPECTED(name->length < 128)) {
		*offset = (UByte) name->length;
		offset++;
	} else {
		offset[0] = (name->length >> 24) | 128;
		offset[1] = name->length >> 16;
		offset[2] = name->length >> 8;
		offset[3] = (UByte) name->length;
		offset += 4;
	}

	// Encode value length
	if(value->length < 128) {
		*offset = (UByte) value->length;
		offset++;
	} else {
		offset[0] = (value->length >> 24) | 128;
		offset[1] = value->length >> 16;
		offset[2] = value->length >> 8;
		offset[3] = (UByte) value->length;
		offset += 4;
	}

	// Copy name and value
	memcpy(offset, name->value, name->length);
	memcpy(offset + name->length, value->value, value->length);
	sock->writeBuffer.length += length;
}

static inline UByte FastCGIDecodeParameter(PancakeSocket *sock, UInt32 *noffset, String *name, String *value) {
	UByte *offset = sock->readBuffer.value + *noffset;

	if(UNEXPECTED(*noffset > sock->readBuffer.length - 2)) {
		// Bad record or client implementation error
		return 0;
	}

	// Fetch name length
	if(*offset & 128) {
		if(UNEXPECTED(offset > sock->readBuffer.value + sock->readBuffer.length - 5)) {
			// Bad record or client implementation error
			return 0;
		}

		name->length = ((offset[0] ^ 128) << 24) +
					 (offset[1] << 16) +
					 (offset[2] << 8) +
					 (offset[3]);

		offset += 4;
	} else {
		name->length = *offset;
		offset++;
	}

	// Fetch value length
	if(*offset & 128) {
		if(UNEXPECTED(sock->readBuffer.value + sock->readBuffer.length - offset < 4)) {
			// Bad record or client implementation error
			return 0;
		}

		value->length = ((offset[0] ^ 128) << 24) +
					 (offset[1] << 16) +
					 (offset[2] << 8) +
					 (offset[3]);

		offset += 4;
	} else {
		value->length = *offset;
		offset++;
	}

	// Check parameter validity
	if(UNEXPECTED(sock->readBuffer.value + sock->readBuffer.length - offset < name->length + value->length)) {
		// Bad record or client implementation error
		return 0;
	}

	name->value = offset;
	value->value = offset + name->length;

	*noffset = value->value - sock->readBuffer.value;

	return 1;
}

static void FastCGIReadRecord(PancakeSocket *sock) {
	UInt16 contentLength, requestID;
	UInt8 paddingLength;
	PancakeHTTPRequest *request;
	PancakeFastCGIClient *client = (PancakeFastCGIClient*) sock->data;

	if(sock->readBuffer.length < 8)
		return;

	// Get data lengths
	contentLength = (sock->readBuffer.value[4] << 8) + sock->readBuffer.value[5];
	paddingLength = sock->readBuffer.value[6];

	// Check whether record was fully transmitted
	if(sock->readBuffer.length < 8 + contentLength + paddingLength)
		return;

	// Fetch request ID and Pancake request data
	requestID = (sock->readBuffer.value[2] << 8) + sock->readBuffer.value[3];

	// Check validity of requestID
	if(UNEXPECTED(requestID > PANCAKE_FASTCGI_MAX_REQUEST_ID)) {
		// Bad server
		PancakeHTTPFastCGIOnRemoteHangup(sock);
		return;
	}

	request = client->requests[requestID];

	if(UNEXPECTED(!request && (requestID || (!requestID && sock->readBuffer.value[1] != FCGI_GET_VALUES_RESULT)))) {
		// Unknown request ID
		PancakeHTTPFastCGIOnRemoteHangup(sock);
		return;
	}

	// Check whether we have running requests on socket
	if(UNEXPECTED(client == NULL && requestID)) {
		// Bad server
		PancakeHTTPFastCGIOnRemoteHangup(sock);
		return;
	}

	// Parse record
	switch(sock->readBuffer.value[1]) {
		case FCGI_GET_VALUES_RESULT: {
			UInt32 offset = 8;
			String name, value;

			// Fetch parameters
			while(offset < contentLength + 8) {
				if(UNEXPECTED(!FastCGIDecodeParameter(sock, &offset, &name, &value))) {
					// Bad parameter encoding
					PancakeHTTPFastCGIOnRemoteHangup(sock);
					return;
				}

				// Search for MPXS_CONNS
				if(name.length == sizeof("FCGI_MPXS_CONNS") - 1
				&& !memcmp(name.value, "FCGI_MPXS_CONNS", sizeof("FCGI_MPXS_CONNS") - 1)) {
					if(value.length >= 1) {
						if(value.value[0] == '1') {
							client->multiplex = 1;
						} else {
							client->multiplex = 0;
						}

						PancakeDebug {
							Byte *interface = PancakeNetworkGetInterfaceName(client->address);

							PancakeLoggerFormat(PANCAKE_LOGGER_SYSTEM, 0, "FastCGI server %s is %scapable of multiplexing", interface, client->multiplex == 0 ? "not " : "");
							PancakeFree(interface);
						}
					}

					break;
				}
			}

			// Close socket (was only used for server capability detection)
			PancakeNetworkClose(sock);
		} return;
		case FCGI_STDOUT: {
			// Check whether client hung up
			if(request->socket->flags & PANCAKE_HTTP_CLIENT_HANGUP) {
				break;
			}

			// Activate request scope group
			PancakeConfigurationActivateScopeGroup(&request->scopeGroup);

			if(request->socket->flags & PANCAKE_HTTP_HEADER_DATA_COMPLETE) {
				// We assume that the application won't send any output anyway when HEAD is used
				if(EXPECTED(request->method != PANCAKE_HTTP_HEAD)) {
					String output;

					output.value = sock->readBuffer.value + 8;
					output.length = contentLength;

					PancakeHTTPOutputChunk(request->socket, &output);

					if(request->HTTPVersion != PANCAKE_HTTP_10) {
						// No chunked transfers in HTTP 1.0

						PancakeNetworkAddWriteSocket(request->socket);
					}
				}
			} else {
				UByte *offset = sock->readBuffer.value + 8;
				UByte *start = offset;
				UInt16 bytesLeft = sock->readBuffer.length - 8;

				request->answerCode = 200;

				// Fetch headers
				while(bytesLeft > 1 && (offset = memchr(offset + 1, '\r', bytesLeft))) {
					if(offset[1] == '\n') {
						UByte *ptr = memchr(start, ':', offset - start);

						if(ptr) {
							UByte *ptr2 = ptr;

							// Lookup header value
							while(*(++ptr2) == ' ' && ptr2 != offset);

							// Parse special headers separately
							switch(ptr - start) {
								case 6:
									if((*start == 'S' || *start == 's')
									&& memcmp(start + 1, "tatus", 5)) {
										break;
									}

									goto StoreHeader;
								case 16:
									if((*start == 'C' || *start == 'c')
									&& (start[8] == 'E' || start[8] == 'e')
									&& memcmp(start + 1, "ontent-", 7)
									&& memcmp(start + 8, "ncoding", 7)) {
										request->contentEncoding = PancakeAllocate(sizeof(String));

										request->onRequestEnd = PancakeHTTPFreeContentEncoding;

										request->contentEncoding->length = offset - ptr2;
										request->contentEncoding->value = PancakeAllocate(request->contentEncoding->length);
										memcpy(request->contentEncoding->value, ptr2, request->contentEncoding->length);

										break;
									}

									goto StoreHeader;
								default:
								StoreHeader: {
									PancakeHTTPHeader *header = PancakeAllocate(sizeof(PancakeHTTPHeader));

									header->name.length = ptr - start;
									header->name.value = PancakeAllocate(header->name.length);
									memcpy(header->name.value, start, header->name.length);

									header->value.length = offset - ptr2;
									header->value.value = PancakeAllocate(header->value.length);
									memcpy(header->value.value, ptr2, header->value.length);

									LL_APPEND(request->answerHeaders, header);
								} break;
							}
						}

						if(offset[2] == '\r' && offset[3] == '\n') {
							// Do not send output when HEAD method is used
							if(EXPECTED(request->method != PANCAKE_HTTP_HEAD)) {
								String output;

								output.value = offset + 4;
								output.length = sock->readBuffer.value + 8 + contentLength - offset - 4;

								if(output.length) {
									PancakeHTTPOutputChunk(request->socket, &output);

									if(request->HTTPVersion != PANCAKE_HTTP_10) {
										// No chunked transfers in HTTP 1.0

										request->socket->onWrite = PancakeHTTPOnWrite;
										PancakeNetworkAddWriteSocket(request->socket);
									}
								}
							}

							break;
						}

						bytesLeft = (sock->readBuffer.value + sock->readBuffer.length) - offset;
						start = offset + 2;
					}
				}

				request->socket->flags |= PANCAKE_HTTP_HEADER_DATA_COMPLETE;
			}

			// Unscope
			PancakeConfigurationUnscope();
		} break;
		case FCGI_END_REQUEST: {
			// Remove request from FCGI client
			client->requests[requestID] = NULL;
			client->sockets[requestID] = NULL;

			// Cache FCGI connection
			if(client->keepAlive) {
				if(UNEXPECTED(sock->flags & PANCAKE_FASTCGI_UNCACHED_CONNECTION)) {
					if(client->multiplex != -1) {
						sock->flags ^= PANCAKE_FASTCGI_UNCACHED_CONNECTION;
					}
					PancakeNetworkCacheConnection(&client->connectionCache, sock);
				} else if(!client->multiplex) {
					PancakeNetworkCacheConnection(&client->connectionCache, sock);
				}
			}

			// Check whether client hung up
			if(request->socket->flags & PANCAKE_HTTP_CLIENT_HANGUP) {
				PancakeHTTPOnRemoteHangup(request->socket);

				break;
			}

			request->socket->onRemoteHangup = PancakeHTTPOnRemoteHangup;

			// Send headers if not done yet
			if(!request->headerSent) {
				PancakeHTTPBuildAnswerHeaders(request->socket);
			}

			request->socket->onWrite = PancakeHTTPFullWriteBuffer;
			PancakeNetworkSetWriteSocket(request->socket);
		} break;
		default: {
			PancakeDebug {
				PancakeLoggerFormat(PANCAKE_LOGGER_SYSTEM, 0, "Unknown FastCGI record %i", sock->readBuffer.value[1]);
			}
		}
	}

	// Remove record from buffer
	sock->readBuffer.length -= 8 + contentLength + paddingLength;

	if(sock->readBuffer.length) {
		memmove(sock->readBuffer.value, sock->readBuffer.value + 8 + contentLength + paddingLength, sock->readBuffer.length);

		if(sock->readBuffer.length >= 8) {
			// More data available, try to parse
			FastCGIReadRecord(sock);
		}
	}

	return;
}

static void PancakeHTTPFastCGIOnRead(PancakeSocket *sock) {
	if(PancakeNetworkRead(sock, 65798) == -1) { // 65798 bytes = max record size
		return;
	}

	FastCGIReadRecord(sock);
}

static void PancakeHTTPFastCGIOnWrite(PancakeSocket *sock) {
	PancakeNetworkWrite(sock);

	if(!sock->writeBuffer.length) {
		PancakeNetworkSetReadSocket(sock);
	}
}

static void PancakeHTTPFastCGIOnRemoteHangup(PancakeSocket *sock) {
	// Lookup running requests on socket
	UInt16 i;
	PancakeFastCGIClient *client = (PancakeFastCGIClient*) sock->data;

	PancakeAssert(client != NULL);

	// Iterate through requests
	for(i = 0; i <= client->highestRequestID; i++) {
		if(client->sockets[i] == sock) {
			// Check whether client has already hung up
			if(client->requests[i]->socket->flags & PANCAKE_HTTP_CLIENT_HANGUP) {
				PancakeHTTPOnRemoteHangup(client->requests[i]->socket);
			} else if(!client->requests[i]->headerSent) {
				// Send exception if no data was sent yet
				client->requests[i]->chunkedTransfer = 0;
				PancakeHTTPException(client->requests[i]->socket, 502);
			} else {
				// End request if data was already sent
				// Do not run onRequestEnd if data is still waiting for write (write function will end request)
				if(!client->requests[i]->socket->writeBuffer.length) {
					PancakeHTTPOnRequestEnd(client->requests[i]->socket);
				} else {
					client->requests[i]->socket->onWrite = PancakeHTTPFullWriteBuffer;
				}
			}

			client->requests[i] = NULL;
			client->sockets[i] = NULL;
		}
	}

	PancakeNetworkUncacheConnection(&client->connectionCache, sock);
	PancakeNetworkClose(sock);
}

static void PancakeHTTPFastCGIOnClientRead(PancakeSocket *sock) {
	PancakeHTTPRequest *request = (PancakeHTTPRequest*) sock->data;
	PancakeFastCGIClient *client;
	UInt16 requestID = (UInt16) (UNative) request->contentServeData;

	// Read data
	PancakeNetworkRead(sock, 65535);

	// Activate request scope group so that we can fetch the FastCGIClient
	PancakeConfigurationActivateScopeGroup(&request->scopeGroup);

	client = FastCGIConfiguration.client;

	PancakeHTTPFastCGIWriteContentBody(client->sockets[requestID], sock, requestID);

	// Remove read flag if all data was received
	if(!request->clientContentLength) {
		PancakeNetworkRemoveReadSocket(sock);
	}
}

static void PancakeHTTPFastCGIOnClientHangup(PancakeSocket *sock) {
	PancakeHTTPRequest *request = (PancakeHTTPRequest*) sock->data;
	PancakeFastCGIClient *client;
	PancakeSocket *FCGISocket;
	UInt16 requestID = (UInt16) (UNative) request->contentServeData;
	UByte FCGIAbortRequest[8] = "\1\x2\0\0\0\0\0\0";

	if(UNEXPECTED(sock->flags & PANCAKE_HTTP_CLIENT_HANGUP)) {
		PancakeDebug {
			PancakeLoggerFormat(PANCAKE_LOGGER_SYSTEM, 0, "Double FCGI client hangup");
		}

		PancakeNetworkRemoveSocket(sock);

		return;
	}

	// Activate request scope group so that we can fetch the FastCGIClient
	PancakeConfigurationActivateScopeGroup(&request->scopeGroup);

	client = FastCGIConfiguration.client;
	PancakeAssert(client != NULL);

	FCGISocket = client->sockets[requestID];
	if(FCGISocket == NULL || client->requests[requestID] != request) {
		// FCGI handling of request already finished
		PancakeHTTPOnRemoteHangup(sock);
		return;
	}

#if PANCAKE_FASTCGI_MAX_REQUEST_ID > 255
	FCGIAbortRequest[2] = requestID >> 8;
#endif
	FCGIAbortRequest[3] = (UByte) requestID;

	// Resize buffer if necessary
	if(FCGISocket->writeBuffer.size < FCGISocket->writeBuffer.length + sizeof(FCGIAbortRequest)) {
		FCGISocket->writeBuffer.size += sizeof(FCGIAbortRequest);
		FCGISocket->writeBuffer.value = PancakeReallocate(FCGISocket->writeBuffer.value, FCGISocket->writeBuffer.size);
	}

	// Copy record to buffer
	memcpy(FCGISocket->writeBuffer.value + FCGISocket->writeBuffer.length, FCGIAbortRequest, sizeof(FCGIAbortRequest));
	FCGISocket->writeBuffer.length += sizeof(FCGIAbortRequest);

	// Try to write now
	PancakeHTTPFastCGIOnWrite(FCGISocket);

	// Add socket to wait sockets if buffer is not empty
	if(FCGISocket->writeBuffer.length) {
		PancakeNetworkAddReadWriteSocket(FCGISocket);
	}

	// Set client hangup flag on client socket and remove socket from wait sockets
	sock->flags |= PANCAKE_HTTP_CLIENT_HANGUP;
	PancakeNetworkRemoveSocket(sock);

	// Unscope
	PancakeConfigurationUnscope();
}

static UByte PancakeHTTPFastCGIServe(PancakeSocket *clientSocket) {
	if(FastCGIConfiguration.client) {
		PancakeHTTPRequest *request = (PancakeHTTPRequest*) clientSocket->data;
		String queryString;

		PancakeHTTPExtractQueryString(request, &queryString);

		{ // Ugly, but necessary
		PancakeSocket *socket;
		UInt16 requestID = 0, i;
		UInt32 offset, length;
		UByte FCGIBeginRequest[16] = "\1\1\0\0\0\x8\0\0\0\1\0\0\0\0\0\0",
				FCGIParams[8] = "\1\x4\0\0\0\0\0\0",
				fullPath[PancakeHTTPConfiguration.documentRoot->length + request->path.length];
		PancakeHTTPHeader *header;

		// Get server capabilities if unknown
        if(UNEXPECTED(FastCGIConfiguration.client->keepAlive && FastCGIConfiguration.client->multiplex == -1)) {
			// Server capabilities unknown, let's ask
			PancakeSocket *vsocket = PancakeNetworkConnect(FastCGIConfiguration.client->address, NULL, 0);

			if(vsocket == NULL) {
					// Connection failed
					PancakeHTTPException(clientSocket, 503);
					return 0;
			}

			vsocket->onRead = PancakeHTTPFastCGIOnRead;
			vsocket->onWrite = PancakeHTTPFastCGIOnWrite;
			vsocket->onRemoteHangup = PancakeHTTPFastCGIOnRemoteHangup;
			vsocket->data = (void*) FastCGIConfiguration.client;

			vsocket->writeBuffer.length = vsocket->writeBuffer.size = sizeof("\x1\x9\0\0\0\x11\0\0" "\xf\0" "FCGI_MPXS_CONNS") - 1;
			vsocket->writeBuffer.value = PancakeAllocate(vsocket->writeBuffer.size);

			// FCGI_GET_VALUES
			memcpy(vsocket->writeBuffer.value, "\x1\x9\0\0\0\x11\0\0" "\xf\0" "FCGI_MPXS_CONNS", sizeof("\x1\x9\0\0\0\x11\0\0" "\xf\0" "FCGI_MPXS_CONNS") - 1);

			PancakeHTTPFastCGIOnWrite(vsocket);

			// Add write socket if necessary
			if(vsocket->writeBuffer.length) {
				PancakeNetworkAddWriteSocket(vsocket);
			}
        }

		// Lookup first free request ID
		for(i = 1; i < PANCAKE_FASTCGI_MAX_REQUEST_ID; i++) {
			if(FastCGIConfiguration.client->requests[i] == NULL) {
				requestID = i;
				break;
			}
		}

		if(!requestID) {
			// No free request ID
			PancakeHTTPException(clientSocket, 503);
			return 0;
		}

		socket = PancakeNetworkConnect(FastCGIConfiguration.client->address, &FastCGIConfiguration.client->connectionCache, FastCGIConfiguration.client->multiplex == 1 ? PANCAKE_NETWORK_CONNECTION_CACHE_KEEP : PANCAKE_NETWORK_CONNECTION_CACHE_REMOVE);

		if(socket == NULL) {
			// Connection failed
			PancakeHTTPException(clientSocket, 503);
			return 0;
		}

		// Mark connection as uncached if server capabilities are unknown right now
		if(UNEXPECTED(FastCGIConfiguration.client->keepAlive && FastCGIConfiguration.client->multiplex == -1)) {
			socket->flags |= PANCAKE_FASTCGI_UNCACHED_CONNECTION;
		}

		socket->onRead = PancakeHTTPFastCGIOnRead;
		socket->onWrite = PancakeHTTPFastCGIOnWrite;
		socket->onRemoteHangup = PancakeHTTPFastCGIOnRemoteHangup;
		socket->data = (void*) FastCGIConfiguration.client;

		// Set FCGI client hangup handler
		clientSocket->onRemoteHangup = PancakeHTTPFastCGIOnClientHangup;

		// Set chunked transfer mode
		request->chunkedTransfer = 1;

		// Check for highest request ID
		if(requestID > FastCGIConfiguration.client->highestRequestID)
			FastCGIConfiguration.client->highestRequestID = requestID;

		// Store request
		FastCGIConfiguration.client->requests[requestID] = request;
		FastCGIConfiguration.client->sockets[requestID] = socket;
		request->contentServeData = (void*) (UNative) requestID;

		// Resize buffer
		if(socket->writeBuffer.size < socket->writeBuffer.length + 512) {
			socket->writeBuffer.size = socket->writeBuffer.length + 512;
			socket->writeBuffer.value = PancakeReallocate(socket->writeBuffer.value, socket->writeBuffer.size);
		}

		// FCGI_BEGIN_REQUEST
		// Version 1; Type 1; RequestID (2 bytes); ContentLength 8 (2 bytes); PaddingLength 0 (2 bytes); Reserved; Role 1 (2 bytes); Flag FCGI_KEEP_CONN; Reserved (5 bytes)
#if PANCAKE_FASTCGI_MAX_REQUEST_ID > 255
		FCGIBeginRequest[2] = requestID >> 8;
#endif
		FCGIBeginRequest[3] = (UByte) requestID;

		// Set keep-alive flag
		if(FastCGIConfiguration.client->keepAlive) {
			FCGIBeginRequest[10] = 1;
		}

		// Copy record to buffer
		memcpy(socket->writeBuffer.value + socket->writeBuffer.length, FCGIBeginRequest, sizeof(FCGIBeginRequest));
		socket->writeBuffer.length += sizeof(FCGIBeginRequest);

		offset = socket->writeBuffer.length;
		socket->writeBuffer.length += sizeof(FCGIParams);

		// Get full path to file
		memcpy(fullPath, PancakeHTTPConfiguration.documentRoot->value, PancakeHTTPConfiguration.documentRoot->length);
		memcpy(fullPath + PancakeHTTPConfiguration.documentRoot->length, request->path.value, request->path.length);

		// Write FCGI parameters
		FastCGIEncodeParameter(socket, &((String) {"SCRIPT_FILENAME", sizeof("SCRIPT_FILENAME") - 1}), &((String) {fullPath, sizeof(fullPath)}));
		FastCGIEncodeParameter(socket, &StaticString("DOCUMENT_ROOT"), PancakeHTTPConfiguration.documentRoot);
		FastCGIEncodeParameter(socket, &((String) {"SCRIPT_NAME", sizeof("SCRIPT_NAME") - 1}), &request->path);
		FastCGIEncodeParameter(socket, &((String) {"REQUEST_URI", sizeof("REQUEST_URI") - 1}), &request->requestAddress);
		FastCGIEncodeParameter(socket, &((String) {"SERVER_NAME", sizeof("SERVER_NAME") - 1}), &request->host);
		FastCGIEncodeParameter(socket, &((String) {"GATEWAY_INTERFACE", sizeof("GATEWAY_INTERFACE") - 1}), &((String) {"CGI/1.1", sizeof("CGI/1.1") - 1}));
		FastCGIEncodeParameter(socket, &((String) {"SERVER_PROTOCOL", sizeof("SERVER_PROTOCOL") - 1}), &((String) {"HTTP/1.1", sizeof("HTTP/1.1") - 1}));
		FastCGIEncodeParameter(socket, &((String) {"SERVER_SOFTWARE", sizeof("SERVER_SOFTWARE") - 1}), &((String) {PANCAKE_FASTCGI_SERVER_SOFTWARE, sizeof(PANCAKE_FASTCGI_SERVER_SOFTWARE) - 1}));

		switch(clientSocket->remoteAddress.sa_family) {
			case AF_INET: {
				Byte IPv4[INET_ADDRSTRLEN + 1];
				String IPv4String;

				inet_ntop(AF_INET, &((struct sockaddr_in*) &clientSocket->remoteAddress)->sin_addr, IPv4, INET_ADDRSTRLEN);

				IPv4String.value = IPv4;
				IPv4String.length = strlen(IPv4);
				FastCGIEncodeParameter(socket, &((String) {"REMOTE_ADDR", sizeof("REMOTE_ADDR") - 1}), &IPv4String);
			} break;
			case AF_INET6: {
				Byte IPv6[INET6_ADDRSTRLEN + 1];
				String IPv6String;

				inet_ntop(AF_INET, &((struct sockaddr_in6*) &clientSocket->remoteAddress)->sin6_addr, IPv6, INET_ADDRSTRLEN);

				IPv6String.value = IPv6;
				IPv6String.length = strlen(IPv6);
				FastCGIEncodeParameter(socket, &((String) {"REMOTE_ADDR", sizeof("REMOTE_ADDR") - 1}), &IPv6String);
			} break;
		}

		switch(request->method) {
			default:
			case PANCAKE_HTTP_GET:
				FastCGIEncodeParameter(socket, &((String) {"REQUEST_METHOD", sizeof("REQUEST_METHOD") - 1}), &((String) {"GET", 3}));
				break;
			case PANCAKE_HTTP_POST:
				FastCGIEncodeParameter(socket, &((String) {"REQUEST_METHOD", sizeof("REQUEST_METHOD") - 1}), &((String) {"POST", 4}));
				break;
			case PANCAKE_HTTP_HEAD:
				FastCGIEncodeParameter(socket, &((String) {"REQUEST_METHOD", sizeof("REQUEST_METHOD") - 1}), &((String) {"HEAD", 4}));
				break;
		}

		if(queryString.length) {
			FastCGIEncodeParameter(socket, &((String) {"QUERY_STRING", sizeof("QUERY_STRING") -1 }), &queryString);
		}

		// Write HTTP headers
		FastCGIEncodeParameter(socket, &((String) {"HTTP_HOST", sizeof("HTTP_HOST") - 1}), &request->host);

		// Content-Length
		if(request->clientContentLength) {
			UByte s[sizeof("4294967296")]; // 32-bit max value
			String contentLength;

			contentLength.value = s;
			itoa(request->clientContentLength, s, 10);
			contentLength.length = strlen(s);

			FastCGIEncodeParameter(socket, &((String) {"HTTP_CONTENT_LENGTH", sizeof("HTTP_CONTENT_LENGTH") - 1}), &contentLength);
			FastCGIEncodeParameter(socket, &((String) {"CONTENT_LENGTH", sizeof("CONTENT_LENGTH") - 1}), &contentLength);
		}

		// If-Modified-Since
		if(request->ifModifiedSince.value) {
			FastCGIEncodeParameter(socket, &((String) {"HTTP_IF_MODIFIED_SINCE", sizeof("HTTP_IF_MODIFIED_SINCE") - 1}), &request->ifModifiedSince);
		}

		// Accept-Encoding
		if(request->acceptEncoding.length) {
			String acceptEncoding;

			acceptEncoding.length = request->acceptEncoding.length;
			acceptEncoding.value = clientSocket->readBuffer.value + request->acceptEncoding.offset;

			FastCGIEncodeParameter(socket, &((String) {"HTTP_ACCEPT_ENCODING", sizeof("HTTP_ACCEPT_ENCODING") - 1}), &acceptEncoding);
		}

		// Other headers
		LL_FOREACH(request->headers, header) {
			UByte parameter[sizeof("HTTP_") - 1 + header->name.length];
			UByte *coffset;

			memcpy(parameter, "HTTP_", sizeof("HTTP_") - 1);
			memcpy(parameter + sizeof("HTTP_") - 1, header->name.value, header->name.length);

			// Handle Content-Type header
			if(header->name.length == sizeof("content-type") - 1 && !memcmp(header->name.value, "content-type", sizeof("content-type") -1)) {
				FastCGIEncodeParameter(socket, &((String) {"CONTENT_TYPE", sizeof("CONTENT_TYPE") - 1}), &header->value);
			}

			// Make parameter name uppercase
			for(coffset = parameter + sizeof("HTTP_") - 1; coffset < parameter + sizeof(parameter); coffset++) {
				*coffset = toupper(*coffset);

				if(*coffset == '-')
					*coffset = '_';
			}

			FastCGIEncodeParameter(socket, &((String) {parameter, sizeof(parameter)}), &header->value);
		}

		// Get length of FCGIParams body
		length = socket->writeBuffer.length - offset - 8;

		// FCGIParams header
		socket->writeBuffer.value[offset] = '\1'; // Version
		socket->writeBuffer.value[offset + 1] = '\x4'; // FCGI_PARAMS
#if PANCAKE_FASTCGI_MAX_REQUEST_ID > 255
		socket->writeBuffer.value[offset + 2] = requestID >> 8;
#else
		socket->writeBuffer.value[offset + 2] = 0;
#endif
		socket->writeBuffer.value[offset + 3] = (UByte) requestID;
		socket->writeBuffer.value[offset + 4] = length >> 8; // FCGIParams body length
		socket->writeBuffer.value[offset + 5] = (UByte) length;
		socket->writeBuffer.value[offset + 6] = '\0'; // PaddingLength
		socket->writeBuffer.value[offset + 7] = '\0'; // Reserved

		// Resize buffer for another FCGIParams record if required
		if(socket->writeBuffer.size < socket->writeBuffer.length + 8) {
			socket->writeBuffer.size = socket->writeBuffer.length + 32;
			socket->writeBuffer.value = PancakeReallocate(socket->writeBuffer.value, socket->writeBuffer.size);
		}

		// Another empty FCGIParams
#if PANCAKE_FASTCGI_MAX_REQUEST_ID > 255
		FCGIParams[2] = requestID >> 8;
#endif
		FCGIParams[3] = (UByte) requestID;
		memcpy(socket->writeBuffer.value + socket->writeBuffer.length, FCGIParams, 8);
		socket->writeBuffer.length += 8;

		// Try to write now
		PancakeHTTPFastCGIOnWrite(socket);

		// Add socket to server architecture if necessary
		if(socket->writeBuffer.length) {
			PancakeNetworkAddWriteSocket(socket);
		}

		// Write STDIN
		if(request->headerEnd < clientSocket->readBuffer.length - 4) {
			PancakeHTTPFastCGIWriteContentBody(socket, clientSocket, requestID);
		}

		// Transmit client data
		if(request->clientContentLength) {
			clientSocket->onRead = PancakeHTTPFastCGIOnClientRead;
			PancakeNetworkSetReadSocket(clientSocket);
		}

		return 1;
	}}

	return 0;
}

#endif
