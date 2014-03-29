#include "PancakeLinuxPoll.h"

#ifdef PANCAKE_LINUX_POLL

#include "PancakeNetwork.h"
#include "PancakeConfiguration.h"
#include "PancakeLogger.h"
#include "PancakeScheduler.h"

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
static inline void PancakeLinuxPollRemoveReadSocket(PancakeSocket *socket);
static inline void PancakeLinuxPollRemoveWriteSocket(PancakeSocket *socket);

static inline void PancakeLinuxPollSetReadSocket(PancakeSocket *socket);
static inline void PancakeLinuxPollSetWriteSocket(PancakeSocket *socket);
static inline void PancakeLinuxPollSetSocket(PancakeSocket *socket);

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

		PancakeLinuxPollRemoveReadSocket,
		PancakeLinuxPollRemoveWriteSocket,
		PancakeLinuxPollRemoveSocket,

		PancakeLinuxPollSetReadSocket,
		PancakeLinuxPollSetWriteSocket,
		PancakeLinuxPollSetSocket,

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
			event.events = EPOLLIN | EPOLLRDHUP | (socket->flags & PANCAKE_LINUX_POLL_OUT ? EPOLLOUT : 0);
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
			event.events = EPOLLOUT | EPOLLRDHUP | (socket->flags & PANCAKE_LINUX_POLL_IN ? EPOLLIN : 0);
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
		socket->flags ^= PANCAKE_LINUX_POLL_SOCKET;
	}
}

static inline void PancakeLinuxPollRemoveReadSocket(PancakeSocket *socket) {
	if(socket->flags & PANCAKE_LINUX_POLL_IN) {
		struct epoll_event event;

		event.events = EPOLLRDHUP | (socket->flags & PANCAKE_LINUX_POLL_OUT ? EPOLLOUT : 0);
		event.data.ptr = (void*) socket;

		epoll_ctl(PancakeLinuxPollFD, EPOLL_CTL_MOD, socket->fd, &event);
		socket->flags ^= PANCAKE_LINUX_POLL_IN;
	}
}

static inline void PancakeLinuxPollRemoveWriteSocket(PancakeSocket *socket) {
	if(socket->flags & PANCAKE_LINUX_POLL_OUT) {
		struct epoll_event event;

		event.events = EPOLLRDHUP | (socket->flags & PANCAKE_LINUX_POLL_IN ? EPOLLIN : 0);
		event.data.ptr = (void*) socket;

		epoll_ctl(PancakeLinuxPollFD, EPOLL_CTL_MOD, socket->fd, &event);
		socket->flags ^= PANCAKE_LINUX_POLL_OUT;
	}
}

static inline void PancakeLinuxPollSetReadSocket(PancakeSocket *socket) {
	struct epoll_event event;

	event.events = EPOLLRDHUP | EPOLLIN;
	event.data.ptr = (void*) socket;

	socket->flags |= PANCAKE_LINUX_POLL_IN;

	if(socket->flags & PANCAKE_LINUX_POLL_SOCKET) {
		epoll_ctl(PancakeLinuxPollFD, EPOLL_CTL_MOD, socket->fd, &event);

		if(socket->flags & PANCAKE_LINUX_POLL_OUT) {
			socket->flags ^= PANCAKE_LINUX_POLL_OUT;
		}
	} else {
		socket->flags |= PANCAKE_LINUX_POLL_SOCKET;

		epoll_ctl(PancakeLinuxPollFD, EPOLL_CTL_ADD, socket->fd, &event);
	}
}

static inline void PancakeLinuxPollSetWriteSocket(PancakeSocket *socket) {
	struct epoll_event event;

	event.events = EPOLLRDHUP | EPOLLOUT;
	event.data.ptr = (void*) socket;

	socket->flags |= PANCAKE_LINUX_POLL_OUT;

	if(socket->flags & PANCAKE_LINUX_POLL_SOCKET) {
		epoll_ctl(PancakeLinuxPollFD, EPOLL_CTL_MOD, socket->fd, &event);

		if(socket->flags & PANCAKE_LINUX_POLL_IN) {
			socket->flags ^= PANCAKE_LINUX_POLL_IN;
		}
	} else {
		socket->flags |= PANCAKE_LINUX_POLL_SOCKET;

		epoll_ctl(PancakeLinuxPollFD, EPOLL_CTL_ADD, socket->fd, &event);
	}
}

static inline void PancakeLinuxPollSetSocket(PancakeSocket *socket) {
	struct epoll_event event;

	event.events = EPOLLRDHUP;
	event.data.ptr = (void*) socket;

	if(socket->flags & PANCAKE_LINUX_POLL_SOCKET) {
		epoll_ctl(PancakeLinuxPollFD, EPOLL_CTL_MOD, socket->fd, &event);

		if(socket->flags & PANCAKE_LINUX_POLL_IN) {
			socket->flags ^= PANCAKE_LINUX_POLL_IN;
		}

		if(socket->flags & PANCAKE_LINUX_POLL_OUT) {
			socket->flags ^= PANCAKE_LINUX_POLL_OUT;
		}
	} else {
		socket->flags |= PANCAKE_LINUX_POLL_SOCKET;

		epoll_ctl(PancakeLinuxPollFD, EPOLL_CTL_ADD, socket->fd, &event);
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

		numEvents = epoll_wait(PancakeLinuxPollFD, events, 32, (PancakeSchedulerGetNextExecutionTime() - time(NULL)) * 1000);

		if(UNEXPECTED(numEvents == -1)) {
			if(PancakeDoShutdown) {
				return;
			}

			if(errno == EINTR) {
				continue;
			}

			PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "epoll_wait failed: %s", strerror(errno));
			return;
		}

		// Network events first
		for(i = 0; i < numEvents; i++) {
			PancakeSocket *sock = (PancakeSocket*) events[i].data.ptr;

			if((events[i].events & EPOLLHUP) && !(events[i].events & EPOLLRDHUP)) {
				sock->onRemoteHangup(sock);
				PancakeCheckHeap();
				continue;
			}

			if(events[i].events & EPOLLIN) {
				sock->onRead(sock);
				PancakeCheckHeap();
			}

			if(events[i].events & EPOLLOUT) {
				sock->onWrite(sock);
				PancakeCheckHeap();
			}

			if(events[i].events & EPOLLRDHUP) {
				sock->onRemoteHangup(sock);
				PancakeCheckHeap();
			}
		}

		// Scheduler events second
		PancakeSchedulerRun();

		if(UNEXPECTED(PancakeDoShutdown)) {
			return;
		}
	}
}

#endif
