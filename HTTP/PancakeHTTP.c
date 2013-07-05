#include "PancakeHTTP.h"
#include "PancakeConfiguration.h"
#include "PancakeLogger.h"

#ifdef PANCAKE_HTTP

PancakeModule PancakeHTTP = {
		"HTTP",
		PancakeHTTPInitialize,
		NULL,
		NULL,
		0
};

PancakeHTTPVirtualHostIndex *PancakeHTTPVirtualHosts = NULL;
PancakeHTTPVirtualHost *PancakeHTTPDefaultVirtualHost = NULL;
PancakeHTTPConfigurationStructure PancakeHTTPConfiguration;

static PancakeHTTPContentServeBackend *contentBackends = NULL;

/* Forward declarations */
static void PancakeHTTPInitializeConnection(PancakeSocket *sock);
static void PancakeHTTPReadHeaderData(PancakeSocket *sock);

PANCAKE_API void PancakeHTTPRegisterContentServeBackend(PancakeHTTPContentServeBackend *backend) {
	LL_APPEND(contentBackends, backend);
}

static UByte PancakeHTTPVirtualHostConfiguration(UByte step, config_setting_t *setting, PancakeConfigurationScope **scope) {
	PancakeHTTPVirtualHost *vhost;

	switch(step) {
		case PANCAKE_CONFIGURATION_INIT: {
			vhost = PancakeAllocate(sizeof(PancakeHTTPVirtualHost));
			*scope = PancakeConfigurationAddScope();
			(*scope)->data = (void*) vhost;

			vhost->configurationScope = *scope;
			vhost->contentBackends = NULL;
			vhost->numContentBackends = 0;

			setting->hook = (void*) vhost;
		} break;
		case PANCAKE_CONFIGURATION_DTOR: {
			vhost = (PancakeHTTPVirtualHost*) setting->hook;

			PancakeConfigurationDestroyScope(vhost->configurationScope);
			PancakeFree(vhost);
		} break;
	}

	return 1;
}

static UByte PancakeHTTPHostsConfiguration(UByte step, config_setting_t *setting, PancakeConfigurationScope **scope) {
	switch(step) {
		case PANCAKE_CONFIGURATION_INIT: {
			PancakeHTTPVirtualHost *vHost = (PancakeHTTPVirtualHost*) (*scope)->data;
			config_setting_t *element;
			UInt16 i = 0;

			while(element = config_setting_get_elem(setting, i++)) {
				PancakeHTTPVirtualHostIndex *index = PancakeAllocate(sizeof(PancakeHTTPVirtualHostIndex));

				index->vHost = vHost;

				HASH_ADD_KEYPTR(hh, PancakeHTTPVirtualHosts, &element->value.sval, strlen(element->value.sval), index);
			}
		} break;
		case PANCAKE_CONFIGURATION_DTOR: {
			config_setting_t *element;
			UInt16 i = 0;

			while(element = config_setting_get_elem(setting, i++)) {
				PancakeHTTPVirtualHostIndex *index;

				HASH_FIND(hh, PancakeHTTPVirtualHosts, &element->value.sval, strlen(element->value.sval), index);
				PancakeAssert(index != NULL);

				HASH_DEL(PancakeHTTPVirtualHosts, index);
				PancakeFree(index);
			}
		} break;
	}

	return 1;
}

static UByte PancakeHTTPDefaultConfiguration(UByte step, config_setting_t *setting, PancakeConfigurationScope **scope) {
	if(step == PANCAKE_CONFIGURATION_INIT && setting->value.ival == 1) {
		PancakeHTTPDefaultVirtualHost = (PancakeHTTPVirtualHost*) (*scope)->data;
	}

	return 1;
}

static UByte PancakeHTTPDocumentRootConfiguration(UByte step, config_setting_t *setting, PancakeConfigurationScope **scope) {
	String *documentRoot;

	switch(step) {
		case PANCAKE_CONFIGURATION_INIT: {
			struct stat buf;

			if(stat(setting->value.sval, &buf) == -1) {
				PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Can't stat document root %s: %s", setting->value.sval, strerror(errno));
				return 0;
			}

			if(!S_ISDIR(buf.st_mode)) {
				PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Document root %s is not a directory", setting->value.sval);
				return 0;
			}

			// Make String out of document root
			documentRoot = PancakeAllocate(sizeof(String));
			documentRoot->length = strlen(setting->value.sval);
			documentRoot->value = PancakeAllocate(documentRoot->length + 1);
			memcpy(documentRoot->value, setting->value.sval, documentRoot->length + 1);

			free(setting->value.sval);
			setting->type = CONFIG_TYPE_SPECIAL;
			setting->value.sval = (char*) documentRoot;
		} break;
		case PANCAKE_CONFIGURATION_DTOR: {
			// Free memory
			documentRoot = (String*) setting->value.sval;
			PancakeFree(documentRoot->value);
			PancakeFree(documentRoot);

			// Make library happy
			setting->type = CONFIG_TYPE_NONE;
		} break;
	}

	return 1;
}

static UByte PancakeHTTPNetworkInterfaceConfiguration(UByte step, config_setting_t *setting, PancakeConfigurationScope **scope) {
	if(PancakeNetworkInterfaceConfiguration(step, setting, scope) && step == PANCAKE_CONFIGURATION_INIT) {
		PancakeSocket *sock = (PancakeSocket*) setting->hook;

		sock->onRead = PancakeHTTPInitializeConnection;

		return 1;
	}

	return 0;
}

static UByte PancakeHTTPContentServeBackendConfiguration(UByte step, config_setting_t *setting, PancakeConfigurationScope **scope) {
	config_setting_t *element;
	UInt16 i = 0;
	PancakeHTTPVirtualHost *vHost = (PancakeHTTPVirtualHost*) setting->parent->hook;

	if(step == PANCAKE_CONFIGURATION_INIT) {
		while(element = config_setting_get_elem(setting, i++)) {
			PancakeHTTPContentServeBackend *backend = NULL, *tmp;

			LL_FOREACH(contentBackends, tmp) {
				if(!strcmp(tmp->name, element->value.sval)) {
					backend = tmp;
					break;
				}
			}

			if(backend == NULL) {
				PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Unknown HTTP content serve backend: %s", element->value.sval);
				return 0;
			}

			vHost->numContentBackends++;
			vHost->contentBackends = PancakeReallocate(vHost->contentBackends, vHost->numContentBackends * sizeof(PancakeHTTPContentServeBackend*));
			vHost->contentBackends[vHost->numContentBackends - 1] = backend->handler;
		}
	} else {
		if(vHost->contentBackends) {
			PancakeFree(vHost->contentBackends);
		}
	}

	return 1;
}

UByte PancakeHTTPInitialize() {
	PancakeConfigurationGroup *group, *child;
	PancakeConfigurationSetting *setting;

	group = PancakeConfigurationAddGroup(NULL, (String) {"HTTP", sizeof("HTTP") - 1}, NULL);
	PancakeNetworkRegisterListenInterfaceGroup(group, PancakeHTTPNetworkInterfaceConfiguration);

	setting = PancakeConfigurationAddSetting(group, (String) {"VirtualHosts", sizeof("VirtualHosts") - 1}, CONFIG_TYPE_LIST, NULL, 0, (config_value_t) 0, NULL);
	group = PancakeConfigurationListGroup(setting, PancakeHTTPVirtualHostConfiguration);
	PancakeConfigurationAddSetting(group, (String) {"Hosts", sizeof("Hosts") - 1}, CONFIG_TYPE_LIST, NULL, 0, (config_value_t) 0, PancakeHTTPHostsConfiguration);
	PancakeConfigurationAddSetting(group, (String) {"Default", sizeof("Default") - 1}, CONFIG_TYPE_INT, NULL, 0, (config_value_t) 0, PancakeHTTPDefaultConfiguration);
	PancakeConfigurationAddSetting(group, (String) {"DocumentRoot", sizeof("DocumentRoot") - 1}, CONFIG_TYPE_STRING, &PancakeHTTPConfiguration.documentRoot, sizeof(String*), (config_value_t) "", PancakeHTTPDocumentRootConfiguration);
	PancakeConfigurationAddSetting(group, (String) {"ContentServeBackends", sizeof("ContentServeBackends") - 1}, CONFIG_TYPE_LIST, NULL, 0, (config_value_t) 0, PancakeHTTPContentServeBackendConfiguration);
	PancakeConfigurationAddSetting(group, (String) {"OutputFilters", sizeof("OutputFilters") - 1}, CONFIG_TYPE_LIST, NULL, 0, (config_value_t) 0, NULL);

	child = PancakeConfigurationLookupGroup(NULL, (String) {"Logging", sizeof("Logging") - 1});
	PancakeConfigurationAddGroupToGroup(group, child);

	return 1;
}

static void PancakeHTTPInitializeConnection(PancakeSocket *sock) {
	PancakeSocket *client = PancakeNetworkAcceptConnection(sock);
	PancakeHTTPRequest *request;

	if(client == NULL) {
		return;
	}

	request = PancakeAllocate(sizeof(PancakeHTTPRequest));
	request->method = 0;
	request->headers = NULL;
	request->requestAddress.value = NULL;
	request->host.value = NULL;
	request->path.value = NULL;

	PancakeConfigurationInitializeScopeGroup(&request->scopeGroup);

	client->onRead = PancakeHTTPReadHeaderData;
	client->onRemoteHangup = PancakeHTTPOnRemoteHangup;
	client->data = (void*) request;

	PancakeNetworkAddReadSocket(client);
}

static void PancakeHTTPReadHeaderData(PancakeSocket *sock) {
	// Read data from socket
	if(PancakeNetworkRead(sock, 1536) == -1) {
		return;
	}

	// Parse HTTP
	if(sock->readBuffer.length >= 5) {
		PancakeHTTPRequest *request = (PancakeHTTPRequest*) sock->data;
		UByte *offset, *headerEnd, *ptr, *ptr2, *ptr3;
		UInt16 i;

		if(!request->method) {
			if(sock->readBuffer.value[0] == 'G') {
				if(sock->readBuffer.value[1] != 'E'
				|| sock->readBuffer.value[2] != 'T'
				|| sock->readBuffer.value[3] != ' ') {
					PancakeHTTPOnRemoteHangup(sock);
					return;
				}

				request->method = PANCAKE_HTTP_GET;
			} else if(sock->readBuffer.value[0] == 'P') {
				if(sock->readBuffer.value[1] != 'O'
				|| sock->readBuffer.value[2] != 'S'
				|| sock->readBuffer.value[3] != 'T'
				|| sock->readBuffer.value[4] != ' ') {
					PancakeHTTPOnRemoteHangup(sock);
					return;
				}

				request->method = PANCAKE_HTTP_POST;
			} else {
				// Probably non-HTTP data, disconnect the client
				PancakeHTTPOnRemoteHangup(sock);
				return;
			}
		}

		if(sock->readBuffer.length >= 10240) {
			// Header too large
			PancakeHTTPOnRemoteHangup(sock);
			return;
		}

		if(request->method == PANCAKE_HTTP_POST) {
			// Lookup end of header data since the end of the buffer is not necessarily \r\n\r\n
		} else if(sock->readBuffer.value[sock->readBuffer.length - 1] == '\n'
			&& sock->readBuffer.value[sock->readBuffer.length - 2] == '\r'
			&& sock->readBuffer.value[sock->readBuffer.length - 3] == '\n'
			&& sock->readBuffer.value[sock->readBuffer.length - 4] == '\r') {
			offset = sock->readBuffer.value + 4; // 4 = "GET "
			headerEnd = sock->readBuffer.value + sock->readBuffer.length - 4;
		} else {
			return;
		}

		// Lookup end of request URI
		ptr = memchr(offset, ' ', headerEnd - offset);
		if(!ptr || ptr == offset) {
			// Malformed header
			PancakeHTTPOnRemoteHangup(sock);
			return;
		}

		// Copy request URI
		request->requestAddress.length = ptr - offset;
		request->requestAddress.value = PancakeAllocate(request->requestAddress.length);
		memcpy(request->requestAddress.value, ptr, request->requestAddress.length);

		// Resolve request URI
		if(*offset != '/') {
			if(headerEnd > offset + sizeof("http://") - 1 && memcmp(offset, "http://", sizeof("http://") - 1) == 0) {
				// http://abc.net[/aaa]
				offset += sizeof("http://") - 1;
				request->host.value = offset;

				if(ptr2 = memchr(offset, '/', ptr - offset)) {
					// http://abc.net/aaa
					request->host.length = ptr2 - offset;
					request->path.value = ptr2;
					request->path.length = ptr - ptr2;
				} else {
					// http://abc.net
					request->host.length = ptr - offset;
				}
			} else {
				// aaa
				// offset - 1 MUST be a space, therefore we can simply overwrite it
				*(offset - 1) = '/';
				request->path.value = offset - 1;
				request->path.length = ptr - offset + 1;
			}
		} else {
			// /aaa
			request->path.value = offset;
			request->path.length = ptr - offset;
		}

		offset = ptr + 1;

		// Fetch HTTP version
		if(offset + sizeof("HTTP/1.1") - 1 > headerEnd || memcmp(offset, "HTTP/1.", sizeof("HTTP/1.") - 1) != 0) {
			// Malformed header
			PancakeHTTPOnRemoteHangup(sock);
			return;
		}

		offset += sizeof("HTTP/1.") - 1;

		if(*offset == '1') {
			// HTTP/1.1
			request->HTTPVersion = PANCAKE_HTTP_11;
		} else if(*offset == '0') {
			// HTTP/1.0
			request->HTTPVersion = PANCAKE_HTTP_10;
		} else {
			// Unknown HTTP version
			PancakeHTTPOnRemoteHangup(sock);
			return;
		}

		offset++;

		if(offset != headerEnd) {
			if(*offset != '\r' || *(offset + 1) != '\n') {
				// Malformed header
				PancakeHTTPOnRemoteHangup(sock);
				return;
			}

			offset += 2;

			// Parse header lines
			while(1) {
				PancakeHTTPHeader *header;

				// memchr() can't fail since we have a \r\n\r\n at the end of the header for sure
				ptr = memchr(offset, '\r', headerEnd - offset + 1);
				PancakeAssert(ptr != NULL);
				if(*(ptr + 1) != '\n') {
					// Malformed header
					PancakeHTTPOnRemoteHangup(sock);
					return;
				}

				ptr2 = memchr(offset, ':', ptr - offset);

				if(!ptr2 || ptr2 == offset) {
					// Malformed header
					PancakeHTTPOnRemoteHangup(sock);
					return;
				}

				// Make header name lowercase
				ptr3 = offset;
				while(ptr3 != ptr2) {
					*ptr3 = tolower(*ptr3);
					ptr3++;
				}

				// Get pointer to value
				ptr3 = ptr2 + 1;

				// RFC 2616 section 4.2 states that the colon may be followed by any amount of spaces
				while(isspace(*ptr3) && ptr3 < ptr) {
					ptr3++;
				}

				// ptr2 - offset == length of header name
				switch(ptr2 - offset) {
					case 4:
						if(!memcmp(offset, "host", 4)) {
							request->host.value = ptr3;
							request->host.length = ptr - ptr3;
						}
						break;
				}

				header = PancakeAllocate(sizeof(PancakeHTTPHeader));
				header->name.value = offset;
				header->name.length = ptr2 - offset;
				header->value.value = ptr3;
				header->value.length = ptr - ptr3;

				// Add header to list
				LL_APPEND(request->headers, header);

				if(ptr == headerEnd) {
					// Finished parsing headers
					break;
				}

				offset = ptr + 2;
			}
		}

		// Fetch virtual host
		if(!request->host.value) {
			request->vHost = PancakeHTTPDefaultVirtualHost;
		} else {
			PancakeHTTPVirtualHostIndex *index;

			HASH_FIND(hh, PancakeHTTPVirtualHosts, request->host.value, request->host.length, index);

			if(index == NULL) {
				request->vHost = PancakeHTTPDefaultVirtualHost;
			} else {
				request->vHost = index->vHost;
			}
		}

		// Initialize scope group
		PancakeConfigurationScopeGroupAddScope(&request->scopeGroup, request->vHost->configurationScope);
		PancakeConfigurationActivateScope(request->vHost->configurationScope);

		// Initialize some values
		request->statDone = 0;
		request->contentLength = 0;
		request->answerType = NULL;
		request->chunkedTransfer = 0;

		// Serve content
		for(i = 0; i < request->vHost->numContentBackends; i++) {
			if(request->vHost->contentBackends[i](sock)) {
				PancakeConfigurationUnscope();
				return;
			}
		}

		// No content available
		PancakeConfigurationUnscope();
		PancakeHTTPOnRemoteHangup(sock);
	}
}

static inline void PancakeHTTPCleanRequestData(PancakeHTTPRequest *request) {
	PancakeHTTPHeader *header, *tmp;

	if(request->requestAddress.value) {
		PancakeFree(request->requestAddress.value);
	}

	LL_FOREACH_SAFE(request->headers, header, tmp) {
		PancakeFree(header);
	}
}

PANCAKE_API inline void PancakeHTTPOnRemoteHangup(PancakeSocket *sock) {
	PancakeHTTPRequest *request = (PancakeHTTPRequest*) sock->data;

	PancakeHTTPCleanRequestData(request);

	PancakeConfigurationDestroyScopeGroup(&request->scopeGroup);

	PancakeFree(sock->data);
	PancakeNetworkClose(sock);
}

PANCAKE_API UByte PancakeHTTPRunAccessChecks(PancakeSocket *sock) {
	PancakeHTTPRequest *request = (PancakeHTTPRequest*) sock->data;

	if(!request->statDone) {
		UByte fullPath[PancakeHTTPConfiguration.documentRoot->length + request->path.length + 1];

		memcpy(fullPath, PancakeHTTPConfiguration.documentRoot->value, PancakeHTTPConfiguration.documentRoot->length);
		memcpy(fullPath + PancakeHTTPConfiguration.documentRoot->length, request->path.value, request->path.length);

		// null-terminate string
		fullPath[PancakeHTTPConfiguration.documentRoot->length + request->path.length] = '\0';

		if(stat(fullPath, &request->fileStat) == -1) {
			return 0;
		}
	}

	if(!S_ISREG(request->fileStat.st_mode) && !S_ISDIR(request->fileStat.st_mode)) {
		return 0;
	}

	return 1;
}

PANCAKE_API void PancakeHTTPOutput(PancakeSocket *sock, String *output) {
	if(sock->writeBuffer.size < sock->writeBuffer.length + output->length) {
		sock->writeBuffer.size += output->length + 2048;
		sock->writeBuffer.value = PancakeReallocate(sock->writeBuffer.value, sock->writeBuffer.size);
	}

	memcpy(sock->writeBuffer.value + sock->writeBuffer.length, output->value, output->length);
	sock->writeBuffer.length += output->length;
}

PANCAKE_API void PancakeHTTPBuildAnswerHeaders(PancakeSocket *sock) {
	PancakeHTTPRequest *request = (PancakeHTTPRequest*) sock->data;
	UByte *offset;

	sock->writeBuffer.size += 8192;
	sock->writeBuffer.value = PancakeReallocate(sock->writeBuffer.value, sock->writeBuffer.size);

	// HTTP/1.x
	sock->writeBuffer.value[0] = 'H';
	sock->writeBuffer.value[1] = 'T';
	sock->writeBuffer.value[2] = 'T';
	sock->writeBuffer.value[3] = 'P';
	sock->writeBuffer.value[4] = '/';
	sock->writeBuffer.value[5] = '1';
	sock->writeBuffer.value[6] = '.';
	sock->writeBuffer.value[7] = request->HTTPVersion == PANCAKE_HTTP_11 ? '1' : '0';
	sock->writeBuffer.value[8] = ' ';

	// Answer code
	PancakeAssert(request->answerCode >= 100 && request->answerCode <= 599);
	itoa(request->answerCode, &sock->writeBuffer.value[9], 10);

	// \r\n - use offsets from here on to allow for dynamic-length answer code descriptions
	offset = sock->writeBuffer.value + 12;
	offset[0] = '\r';
	offset[1] = '\n';
	offset += 2;

	// Content-Length
	if(request->contentLength) {
		memcpy(offset, "Content-Length", sizeof("Content-Length") - 1);
		offset += sizeof("Content-Length") - 1;
		offset[0] = ':';
		offset[1] = ' ';
		offset += 2;
		offset += sprintf(offset, "%i", request->contentLength);

		// \r\n
		offset[0] = '\r';
		offset[1] = '\n';
		offset += 2;
	}

	// Content-Type
	if(request->answerType) {
		memcpy(offset, "Content-Type", sizeof("Content-Type") - 1);
		offset += sizeof("Content-Type") - 1;
		offset[0] = ':';
		offset[1] = ' ';
		offset += 2;
		memcpy(offset, request->answerType->type.value, request->answerType->type.length);
		offset += request->answerType->type.length;

		// \r\n
		offset[0] = '\r';
		offset[1] = '\n';
		offset += 2;
	}

	// \r\n
	offset[0] = '\r';
	offset[1] = '\n';

	sock->writeBuffer.length = offset - sock->writeBuffer.value + 2;
}
#endif
