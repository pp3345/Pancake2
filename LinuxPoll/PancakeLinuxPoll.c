#include "PancakeLinuxPoll.h"
#include "PancakeNetwork.h"
#include "PancakeConfiguration.h"
#include "PancakeLogger.h"

#ifdef PANCAKE_LINUX_POLL

Int32 PancakeLinuxPollFD = -1;

/* Forward declarations */
static UByte PancakeLinuxPollInitialize();
static UByte PancakeLinuxPollServerInitialize();
static void PancakeLinuxPollWait();
static inline void PancakeLinuxPollAddReadSocket(PancakeSocket *socket);
static inline void PancakeLinuxPollAddWriteSocket(PancakeSocket *socket);
static inline void PancakeLinuxPollAddReadWriteSocket(PancakeSocket *socket);

PancakeModule PancakeLinuxPoll = {
		"LinuxPoll",
		PancakeLinuxPollInitialize,
		NULL,
		NULL,
		0
};


PancakeServerArchitecture PancakeLinuxPollServer = {
		(String) {"LinuxPoll", sizeof("LinuxPoll") - 1},

		PancakeLinuxPollWait,

		PancakeLinuxPollAddReadSocket,
		PancakeLinuxPollAddWriteSocket,
		PancakeLinuxPollAddReadWriteSocket,

		PancakeLinuxPollServerInitialize,

		NULL
};

static UByte PancakeLinuxPollInitialize() {
	PancakeRegisterServerArchitecture(&PancakeLinuxPollServer);

	return 1;
}

static UByte PancakeLinuxPollServerInitialize() {
	// Initialize with size 32 on old kernels
	PancakeLinuxPollFD = epoll_create(32);

	if(PancakeLinuxPollFD == -1) {
		PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Can't create epoll instance: %s", strerror(errno));
		return 0;
	}

	return 1;
}

static inline void PancakeLinuxPollAddReadSocket(PancakeSocket *socket) {
	if(socket->flags & EPOLLRDHUP) {
		if(!(socket->flags & EPOLLIN)) {
			struct epoll_event event;

			socket->flags |= EPOLLIN;
			event.events = EPOLLIN | EPOLLRDHUP | (socket->flags & EPOLLOUT);
			event.data.ptr = (void*) socket;

			epoll_ctl(PancakeLinuxPollFD, EPOLL_CTL_MOD, socket->fd, &event);
		}
	} else {
		struct epoll_event event;

		socket->flags |= EPOLLIN | EPOLLRDHUP;
		event.events = EPOLLIN | EPOLLRDHUP;
		event.data.ptr = (void*) socket;

		epoll_ctl(PancakeLinuxPollFD, EPOLL_CTL_ADD, socket->fd, &event);
	}
}

static inline void PancakeLinuxPollAddWriteSocket(PancakeSocket *socket) {
	if(socket->flags & EPOLLRDHUP) {
		if(!(socket->flags & EPOLLOUT)) {
			struct epoll_event event;

			socket->flags |= EPOLLOUT;
			event.events = EPOLLOUT | EPOLLRDHUP | (socket->flags & EPOLLIN);
			event.data.ptr = (void*) socket;

			epoll_ctl(PancakeLinuxPollFD, EPOLL_CTL_MOD, socket->fd, &event);
		}
	} else {
		struct epoll_event event;

		socket->flags |= EPOLLOUT | EPOLLRDHUP;
		event.events = EPOLLOUT | EPOLLRDHUP;
		event.data.ptr = (void*) socket;

		epoll_ctl(PancakeLinuxPollFD, EPOLL_CTL_ADD, socket->fd, &event);
	}
}

static inline void PancakeLinuxPollAddReadWriteSocket(PancakeSocket *socket) {
	if(socket->flags & EPOLLRDHUP) {
		if((socket->flags & (EPOLLOUT | EPOLLIN)) != (EPOLLOUT | EPOLLIN)) {
			struct epoll_event event;

			socket->flags |= EPOLLOUT | EPOLLIN;
			event.events = EPOLLOUT | EPOLLRDHUP | EPOLLIN;
			event.data.ptr = (void*) socket;

			epoll_ctl(PancakeLinuxPollFD, EPOLL_CTL_MOD, socket->fd, &event);
		}
	} else {
		struct epoll_event event;

		socket->flags |= EPOLLOUT | EPOLLIN | EPOLLRDHUP;
		event.events = EPOLLOUT | EPOLLIN | EPOLLRDHUP;
		event.data.ptr = (void*) socket;

		epoll_ctl(PancakeLinuxPollFD, EPOLL_CTL_ADD, socket->fd, &event);
	}
}

static void PancakeLinuxPollWait() {
}

#endif
