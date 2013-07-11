#include "PancakeLinuxPoll.h"
#include "PancakeNetwork.h"
#include "PancakeConfiguration.h"
#include "PancakeLogger.h"

#ifdef PANCAKE_LINUX_POLL

Int32 PancakeLinuxPollFD = -1;

/* Forward declarations */
static UByte PancakeLinuxPollInitialize();
static UByte PancakeLinuxPollShutdown();
static UByte PancakeLinuxPollServerInitialize();
static void PancakeLinuxPollWait();
static inline void PancakeLinuxPollAddReadSocket(PancakeSocket *socket);
static inline void PancakeLinuxPollAddWriteSocket(PancakeSocket *socket);
static inline void PancakeLinuxPollAddReadWriteSocket(PancakeSocket *socket);

static inline void PancakeLinuxPollRemoveSocket(PancakeSocket *socket);

PancakeModule PancakeLinuxPoll = {
		"LinuxPoll",
		PancakeLinuxPollInitialize,
		NULL,
		PancakeLinuxPollShutdown,
		0
};


PancakeServerArchitecture PancakeLinuxPollServer = {
		(String) {"LinuxPoll", sizeof("LinuxPoll") - 1},

		PancakeLinuxPollWait,

		PancakeLinuxPollAddReadSocket,
		PancakeLinuxPollAddWriteSocket,
		PancakeLinuxPollAddReadWriteSocket,

		PancakeLinuxPollRemoveSocket,

		PancakeLinuxPollServerInitialize,

		NULL
};

static UByte PancakeLinuxPollInitialize() {
	PancakeRegisterServerArchitecture(&PancakeLinuxPollServer);

	return 1;
}

static UByte PancakeLinuxPollShutdown() {
	if(PancakeLinuxPollFD != -1) {
		close(PancakeLinuxPollFD);
	}

	return 1;
}

static UByte PancakeLinuxPollServerInitialize() {
	// Just test epoll here, to make sure everything will work
	// The actual epoll instance must be created in the workers as it can't be shared

	// Initialize with size 32 on old kernels
	PancakeLinuxPollFD = epoll_create(32);

	if(PancakeLinuxPollFD == -1) {
		PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Can't create epoll instance: %s", strerror(errno));
		return 0;
	}

	close(PancakeLinuxPollFD);
	PancakeLinuxPollFD = -1;

	return 1;
}

static inline void PancakeLinuxPollAddReadSocket(PancakeSocket *socket) {
	if(socket->flags & PANCAKE_LINUX_POLL_SOCKET) {
		if(!(socket->flags & PANCAKE_LINUX_POLL_IN)) {
			struct epoll_event event;

			socket->flags |= PANCAKE_LINUX_POLL_IN;
			event.events = EPOLLIN | EPOLLRDHUP | (socket->flags & EPOLLOUT);
			event.data.ptr = (void*) socket;

			epoll_ctl(PancakeLinuxPollFD, EPOLL_CTL_MOD, socket->fd, &event);
		}
	} else {
		struct epoll_event event;

		socket->flags |= PANCAKE_LINUX_POLL_IN | PANCAKE_LINUX_POLL_SOCKET;
		event.events = EPOLLIN | EPOLLRDHUP;
		event.data.ptr = (void*) socket;

		epoll_ctl(PancakeLinuxPollFD, EPOLL_CTL_ADD, socket->fd, &event);
	}
}

static inline void PancakeLinuxPollAddWriteSocket(PancakeSocket *socket) {
	if(socket->flags & PANCAKE_LINUX_POLL_SOCKET) {
		if(!(socket->flags & PANCAKE_LINUX_POLL_OUT)) {
			struct epoll_event event;

			socket->flags |= PANCAKE_LINUX_POLL_OUT;
			event.events = EPOLLOUT | EPOLLRDHUP | (socket->flags & EPOLLIN);
			event.data.ptr = (void*) socket;

			epoll_ctl(PancakeLinuxPollFD, EPOLL_CTL_MOD, socket->fd, &event);
		}
	} else {
		struct epoll_event event;

		socket->flags |= PANCAKE_LINUX_POLL_OUT | PANCAKE_LINUX_POLL_SOCKET;
		event.events = EPOLLOUT | EPOLLRDHUP;
		event.data.ptr = (void*) socket;

		epoll_ctl(PancakeLinuxPollFD, EPOLL_CTL_ADD, socket->fd, &event);
	}
}

static inline void PancakeLinuxPollAddReadWriteSocket(PancakeSocket *socket) {
	if(socket->flags & PANCAKE_LINUX_POLL_SOCKET) {
		if((socket->flags & (PANCAKE_LINUX_POLL_OUT | PANCAKE_LINUX_POLL_IN)) != (PANCAKE_LINUX_POLL_OUT | PANCAKE_LINUX_POLL_IN)) {
			struct epoll_event event;

			socket->flags |= PANCAKE_LINUX_POLL_OUT | PANCAKE_LINUX_POLL_IN;
			event.events = EPOLLOUT | EPOLLRDHUP | EPOLLIN;
			event.data.ptr = (void*) socket;

			epoll_ctl(PancakeLinuxPollFD, EPOLL_CTL_MOD, socket->fd, &event);
		}
	} else {
		struct epoll_event event;

		socket->flags |= PANCAKE_LINUX_POLL_OUT | PANCAKE_LINUX_POLL_IN | PANCAKE_LINUX_POLL_SOCKET;
		event.events = EPOLLOUT | EPOLLIN | EPOLLRDHUP;
		event.data.ptr = (void*) socket;

		epoll_ctl(PancakeLinuxPollFD, EPOLL_CTL_ADD, socket->fd, &event);
	}
}

static inline void PancakeLinuxPollRemoveSocket(PancakeSocket *socket) {
	if(socket->flags & PANCAKE_LINUX_POLL_SOCKET) {
		epoll_ctl(PancakeLinuxPollFD, EPOLL_CTL_DEL, socket->fd, NULL);
	}
}

static void PancakeLinuxPollWait() {
	// Initialize with size 32 on old kernels
	PancakeLinuxPollFD = epoll_create(32);

	if(PancakeLinuxPollFD == -1) {
		PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Can't create epoll instance: %s", strerror(errno));
		return;
	}

	// Activate listen sockets
	PancakeNetworkActivateListenSockets();

	while(1) {
		struct epoll_event events[32];
		Int32 numEvents, i;

		if((numEvents = epoll_wait(PancakeLinuxPollFD, events, 32, -1)) == -1) {
			if(PancakeDoShutdown) {
				return;
			}

			if(errno == EINTR) {
				continue;
			}

			PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "epoll_wait failed: %s", strerror(errno));
			return;
		}

		for(i = 0; i < numEvents; i++) {
			PancakeSocket *sock = (PancakeSocket*) events[i].data.ptr;

			if((events[i].events & EPOLLHUP) || (events[i].events & EPOLLRDHUP)) {
				sock->onRemoteHangup(sock);
				continue;
			}

			if(events[i].events & EPOLLIN) {
				sock->onRead(sock);
			}

			if(events[i].events & EPOLLOUT) {
				sock->onWrite(sock);
			}
		}

		if(PancakeDoShutdown) {
			return;
		}
	}
}

#endif
