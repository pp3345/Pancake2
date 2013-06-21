#include "PancakeNetwork.h"
#include "PancakeConfiguration.h"
#include "PancakeLogger.h"

static PancakeServerArchitecture *architectures = NULL;
static PancakeSocket **listenSockets = NULL;
static UInt16 numListenSockets = 0;

UByte PancakeNetworkActivate() {
	UInt16 i;

	if(!numListenSockets) {
		PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "No network interfaces configured");
		return 0;
	}

	for(i = 0; i < numListenSockets; i++) {
		PancakeSocket *sock = listenSockets[i];

		// Start listening on socket
		if(listen(sock->fd, (Int32) sock->backlog) == -1) {
			Byte *name = PancakeNetworkGetInterfaceName(sock->localAddress);

			PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Can't listen on %s: %s", name, strerror(errno));

			PancakeFree(name);
			return 0;
		}

		// Add socket to read socket list
		PancakeNetworkAddReadSocket(sock);
	}

	PancakeFree(listenSockets);

	return 1;
}

PANCAKE_API Byte *PancakeNetworkGetInterfaceName(struct sockaddr *addr) {
	Byte *name;

	switch(addr->sa_family) {
		case AF_INET: {
			Byte textAddress[INET_ADDRSTRLEN + 1];

			inet_ntop(AF_INET, &((struct sockaddr_in*) addr)->sin_addr, textAddress, INET_ADDRSTRLEN);

			// IPv4:<address>:<port>
			name = PancakeAllocate(sizeof("IPv4::12345") + INET_ADDRSTRLEN);
			sprintf(name, "IPv4:%s:%i", textAddress, (Int32) ntohs(((struct sockaddr_in*) addr)->sin_port));
		} break;
		case AF_INET6: {
			Byte textAddress[INET6_ADDRSTRLEN + 1];

			inet_ntop(AF_INET6, &((struct sockaddr_in6*) addr)->sin6_addr, textAddress, INET6_ADDRSTRLEN);

			// IPv6:[<address>]:<port>
			name = PancakeAllocate(sizeof("IPv6:[]:12345") + INET6_ADDRSTRLEN);
			sprintf(name, "IPv6:[%s]:%i", textAddress, (Int32) ntohs(((struct sockaddr_in6*) addr)->sin6_port));
		} break;
		case AF_UNIX: {
			// UNIX:<address>
			name = PancakeAllocate(sizeof("UNIX:") + sizeof(((struct sockaddr_un*) addr)->sun_path));
			sprintf(name, "UNIX:%s", ((struct sockaddr_un*) addr)->sun_path);
		} break;
	}

	return name;
}

PANCAKE_API void PancakeRegisterServerArchitecture(PancakeServerArchitecture *arch) {
	HASH_ADD_KEYPTR(hh, architectures, arch->name.value, arch->name.length, arch);
}

void PancakeNetworkUnload() {
	HASH_CLEAR(hh, architectures);
}

PANCAKE_API UByte PancakeNetworkInterfaceConfiguration(UByte step, config_setting_t *setting, PancakeConfigurationScope **scope) {
	switch(step) {
		case PANCAKE_CONFIGURATION_INIT: {
			PancakeSocket *socket = PancakeAllocate(sizeof(PancakeSocket));

			socket->data = NULL;
			socket->fd = -1;
			socket->localAddress = PancakeAllocate(sizeof(struct sockaddr));
			socket->onRead = NULL;
			socket->onWrite = NULL;
			socket->onRemoteHangup = NULL;
			socket->backlog = 1;
			socket->flags = 0;

			socket->localAddress->sa_family = 0;
			memset(socket->localAddress->sa_data, 0, sizeof(socket->localAddress->sa_data));

			setting->hook = (void*) socket;
		} break;
		case PANCAKE_CONFIGURATION_DTOR: {
			PancakeSocket *socket = (PancakeSocket*) setting->hook;

			if(socket->fd != -1) {
				close(socket->fd);
			}

			PancakeFree(socket->localAddress);
			PancakeFree(socket);

			// Make library happy
			setting->hook = NULL;
		} break;
	}

	return 1;
}

static UByte PancakeNetworkInterfaceNetworkConfiguration(UByte step, config_setting_t *setting, PancakeConfigurationScope **scope) {
	if(step == PANCAKE_CONFIGURATION_INIT) {
		PancakeSocket *sock = (PancakeSocket*) setting->parent->hook;

		// Check family
		if(!strcmp(setting->value.sval, "ip4")) {
			struct sockaddr_in *addr = (struct sockaddr_in*) sock->localAddress;

			sock->localAddress->sa_family = AF_INET;
			addr->sin_family = AF_INET;
		} else if(!strcmp(setting->value.sval, "ip6")) {
			struct sockaddr_in6 *addr = (struct sockaddr_in6*) sock->localAddress;

			sock->localAddress->sa_family = AF_INET6;
			addr->sin6_family = AF_INET6;
		} else if(!strcmp(setting->value.sval, "unix")) {
			struct sockaddr_un *addr = (struct sockaddr_un*) sock->localAddress;

			sock->localAddress->sa_family = AF_UNIX;
			addr->sun_family = AF_UNIX;
		} else {
			PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Invalid network family %s", setting->value.sval);
			return 0;
		}

		sock->fd = socket(sock->localAddress->sa_family, SOCK_STREAM, sock->localAddress->sa_family == AF_UNIX ? 0 : SOL_TCP);

		if(sock->fd == -1) {
			PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Can't create socket: %s", strerror(errno));
			return 0;
		}
	}

	return 1;
}

static UByte PancakeNetworkInterfaceTryBind(PancakeSocket *socket) {
	int retval;

	// Try binding to interface
	switch(socket->localAddress->sa_family) {
		case AF_INET:
			retval = bind(socket->fd, (struct sockaddr_in*) socket->localAddress, sizeof(struct sockaddr_in));
			break;
		case AF_INET6:
			retval = bind(socket->fd, (struct sockaddr_in6*) socket->localAddress, sizeof(struct sockaddr_in6));
			break;
		case AF_UNIX:
			retval = bind(socket->fd, (struct sockaddr_un*) socket->localAddress, SUN_LEN((struct sockaddr_un*) socket->localAddress));
			break;
	}

	if(retval == -1) {
		Byte *name = PancakeNetworkGetInterfaceName(socket->localAddress);

		PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Can't bind to interface %s: %s", name, strerror(errno));
		PancakeFree(name);

		return 0;
	}

	// Add socket for later activation
	numListenSockets++;
	listenSockets = PancakeReallocate(listenSockets, numListenSockets * sizeof(PancakeSocket*));
	listenSockets[numListenSockets - 1] = socket;

	return 1;
}

static UByte PancakeNetworkInterfaceAddressConfiguration(UByte step, config_setting_t *setting, PancakeConfigurationScope **scope) {
	PancakeSocket *socket;

	switch(step) {
		case PANCAKE_CONFIGURATION_INIT: {
			socket = (PancakeSocket*) setting->parent->hook;

			if(!socket->localAddress->sa_family) {
				PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Network family must be set before address");
				return 0;
			}

			switch(socket->localAddress->sa_family) {
				case AF_INET: {
					struct sockaddr_in *addr = (struct sockaddr_in*) socket->localAddress;
					int retval = inet_pton(AF_INET, setting->value.sval, &addr->sin_addr);

					switch(retval) {
						case -1:
							PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Network address can not be parsed: %s", strerror(errno));
							return 0;
						case 0:
							PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Invalid IPv4 address: %s", setting->value.sval);
							return 0;
					}

					// Try binding now if address data is complete
					if(addr->sin_port && !PancakeNetworkInterfaceTryBind(socket)) {
						return 0;
					}
				} break;
				case AF_INET6: {
					struct sockaddr_in6 *addr = (struct sockaddr_in6*) socket->localAddress;
					int retval = inet_pton(AF_INET6, setting->value.sval, &addr->sin6_addr);

					switch(retval) {
						case -1:
							PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Network address can not be parsed: %s", strerror(errno));
							return 0;
						case 0:
							PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Invalid IPv6 address: %s", setting->value.sval);
							return 0;
					}

					// Try binding now if address data is complete
					if(addr->sin6_port && !PancakeNetworkInterfaceTryBind(socket)) {
						return 0;
					}
				} break;
				case AF_UNIX: {
					struct sockaddr_un *addr = (struct sockaddr_un*) socket->localAddress;

					if(strlen(setting->value.sval) > sizeof(addr->sun_path) - 1) {
						PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "UNIX path %s is longer than the allowed limit of %i characters", setting->value.sval, sizeof(addr->sun_path) - 1);
						return 0;
					}

					memcpy(addr->sun_path, setting->value.sval, strlen(setting->value.sval) + 1);

					if(!PancakeNetworkInterfaceTryBind(socket)) {
						return 0;
					}
				} break;
			}
		} break;
	}

	return 1;
}

static UByte PancakeNetworkInterfacePortConfiguration(UByte step, config_setting_t *setting, PancakeConfigurationScope **scope) {
	PancakeSocket *socket;

	switch(step) {
		case PANCAKE_CONFIGURATION_INIT: {
			socket = (PancakeSocket*) setting->parent->hook;

			if(!socket->localAddress->sa_family) {
				PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Network family must be set before port");
				return 0;
			}

			// TCP supports only ports from 1 - 65535
			if(setting->value.ival < 1 || setting->value.ival > 65535) {
				PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Value out of range");
				return 0;
			}

			switch(socket->localAddress->sa_family) {
				case AF_INET: {
					struct sockaddr_in *addr = (struct sockaddr_in*) socket->localAddress;

					addr->sin_port = htons(setting->value.ival);

					if(addr->sin_addr.s_addr && !PancakeNetworkInterfaceTryBind(socket)) {
						return 0;
					}
				} break;
				case AF_INET6: {
					struct sockaddr_in6 *addr = (struct sockaddr_in6*) socket->localAddress;

					addr->sin6_port = htons(setting->value.ival);

					if(addr->sin6_addr.s6_addr && !PancakeNetworkInterfaceTryBind(socket)) {
						return 0;
					}
				} break;
				case AF_UNIX: {
					PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Can't set port on UNIX sockets");
					return 0;
				} break;
			}
		} break;
	}

	return 1;
}

static UByte PancakeNetworkInterfaceBacklogConfiguration(UByte step, config_setting_t *setting, PancakeConfigurationScope **scope) {
	if(step == PANCAKE_CONFIGURATION_INIT) {
		PancakeSocket *sock = (PancakeSocket*) setting->parent->hook;

		sock->backlog = setting->value.ival;
	}

	return 1;
}

PANCAKE_API PancakeConfigurationSetting *PancakeNetworkRegisterListenInterfaceGroup(PancakeConfigurationGroup *parent, PancakeConfigurationHook hook) {
	PancakeConfigurationSetting *setting;
	PancakeConfigurationGroup *group;

	setting = PancakeConfigurationAddSetting(parent, (String) {"Interfaces", sizeof("Interfaces") - 1}, CONFIG_TYPE_LIST, NULL, 0, (config_value_t) 0, NULL);
	group = PancakeConfigurationListGroup(setting, hook);
	PancakeConfigurationAddSetting(group, (String) {"Network", sizeof("Network") - 1}, CONFIG_TYPE_STRING, NULL, 0, (config_value_t) "", PancakeNetworkInterfaceNetworkConfiguration);
	PancakeConfigurationAddSetting(group, (String) {"Address", sizeof("Address") - 1}, CONFIG_TYPE_STRING, NULL, 0, (config_value_t) "", PancakeNetworkInterfaceAddressConfiguration);
	PancakeConfigurationAddSetting(group, (String) {"Port", sizeof("Port") - 1}, CONFIG_TYPE_INT, NULL, 0, (config_value_t) 0, PancakeNetworkInterfacePortConfiguration);
	PancakeConfigurationAddSetting(group, (String) {"Backlog", sizeof("Backlog") - 1}, CONFIG_TYPE_INT, NULL, 0, (config_value_t) 0, PancakeNetworkInterfaceBacklogConfiguration);

	return setting;
}

UByte PancakeConfigurationServerArchitecture(UByte step, config_setting_t *setting, PancakeConfigurationScope **scope) {
	PancakeServerArchitecture *arch;

	switch(step) {
		case PANCAKE_CONFIGURATION_INIT:
			PancakeAssert(setting->type == CONFIG_TYPE_STRING);

			HASH_FIND(hh, architectures, setting->value.sval, strlen(setting->value.sval), arch);

			// Fail if server architecture does not exist
			if(arch == NULL) {
				PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Unknown server architecture %s", setting->value.sval);
				return 0;
			}

			free(setting->value.sval);

			// Set special type
			setting->type = CONFIG_TYPE_SERVER_ARCHITECTURE;
			setting->value.sval = (char*) arch;

			break;
		case PANCAKE_CONFIGURATION_DTOR:
			// Reset type to make library happy
			if(setting->type == CONFIG_TYPE_SERVER_ARCHITECTURE) {
				setting->type = CONFIG_TYPE_NONE;
			}

			break;
	}

	return 1;
}

PANCAKE_API inline PancakeSocket *PancakeNetworkAcceptConnection(PancakeSocket *sock) {
	Int32 fd, addrLen = sizeof(struct sockaddr);
	struct sockaddr addr;
	PancakeSocket *client;
#ifndef HAVE_ACCEPT4
	Int32 flags;
#endif

#ifdef HAVE_ACCEPT4
	// Accelerated version for Linux
	fd = accept4(sock->fd, &addr, &addrLen, SOCK_NONBLOCK);
#else
	fd = accept(sock->fd, &addr, &addrLen);
#endif

	if(fd == -1) {
		return NULL;
	}

#ifndef HAVE_ACCEPT4
	flags = fcntl(fd, F_GETFL);
	flags |= O_NONBLOCK;
	fcntl(fd, F_SETFL, flags);
#endif

	client = PancakeAllocate(sizeof(PancakeSocket));
	client->fd = fd;
	client->localAddress = sock->localAddress;
	client->remoteAddress = addr;
	client->flags = 0;
	client->readBuffer.size = 0;
	client->readBuffer.value = NULL;

	return client;
}

PANCAKE_API inline Byte PancakeNetworkRead(PancakeSocket *sock, UInt32 maxLength) {
	UByte buf[maxLength];
	UInt32 length;

	length = read(sock->fd, buf, maxLength);

	if(length == -1) {
#if EAGAIN != EWOULDBLOCK // On some systems these values differ
		if(errno == EAGAIN || errno == EWOULDBLOCK)
#else
		if(errno == EAGAIN)
#endif
		{
			return 0;
		}

		sock->onRemoteHangup(sock);
		return -1;
	}

	if(sock->readBuffer.size < sock->readBuffer.length + length) {
		sock->readBuffer.size += length + 64;
		sock->readBuffer.value = PancakeReallocate(sock->readBuffer.value, sock->readBuffer.size);
	}

	memcpy(sock->readBuffer.value + sock->readBuffer.length, buf, length);

	sock->readBuffer.length += length;

	return length;
}

PANCAKE_API inline void PancakeNetworkClose(PancakeSocket *sock) {
	// Remove socket from list
	PancakeNetworkRemoveSocket(sock);

	// Close underlying file descriptor
	close(sock->fd);

	// Free read buffer
	if(sock->readBuffer.size) {
		PancakeFree(sock->readBuffer.value);
	}

	// Free socket
	PancakeFree(sock);
}
